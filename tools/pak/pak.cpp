#include <CarpgLibCore.h>
#include <Pak.h>

struct Paker
{
	enum Cmd
	{
		None,
		Help,
		Encrypt,
		FullEncrypt,
		NoCompress,
		NoSubdir,
		Output,
		Key,
		Browse,
		Unpack
	};

	std::map<string, Cmd> cmds =
	{
		{"?", Help}, {"h", Help}, {"help", Help},
		{"e", Encrypt}, {"encrypt", Encrypt},
		{"fe", FullEncrypt}, {"fullencrypt", FullEncrypt},
		{"nc", NoCompress}, {"nocompress", NoCompress},
		{"ns", NoSubdir}, {"nosubdir", NoSubdir},
		{"o", Output}, {"output", Output},
		{"k", Key}, {"key", Key},
		{"b", Browse}, {"browse", Browse},
		{"u", Unpack }, {"unpack", Unpack }
	};

	PakWriter pakw;
	string crypt_key, decrypt_key, output;
	bool compress, encrypt, full_encrypt, subdir, done_anything;

	Paker()
	{
		output = "data.pak";
		compress = true;
		encrypt = false;
		full_encrypt = false;
		subdir = true;
		done_anything = false;
	}

	void Add(cstring path, bool subdir)
	{
		printf("Scanning: %s ...\n", path);
		io::FindFiles(path, [=](const io::FileInfo& info)
		{
			string new_path = io::CombinePath(path, info.filename);
			if(info.is_dir)
			{
				if(subdir)
				{
					new_path += "/*";
					Add(new_path.c_str(), true);
				}
			}
			else
				pakw.AddFile(new_path.c_str());
			return true;
		});
	}

	void BrowsePak(cstring path)
	{
		done_anything = true;

		if(!pakw.Read(path))
		{
			printf("Failed to read '%s'.", path);
			return;
		}

		vector<PakWriter::File>& files = pakw.GetFiles();
		printf("Browsing files: %u\n", files.size());
		uint total_size = 0u, total_compressed_size = 0u;
		for(PakWriter::File& file : files)
		{
			if(file.compressedSize == file.size)
				printf("  %s - size %s, offset %u\n", file.name.c_str(), GetSize(file.size), file.dataOffset);
			else
				printf("  %s - size %s, compressed %s, offset %u\n", file.name.c_str(), GetSize(file.size), GetSize(file.compressedSize), file.dataOffset);
			total_size += file.size;
			total_compressed_size += file.compressedSize;
		}
		printf("Size: %s, compressed %s (%d%%)\n", GetSize(total_size), GetSize(total_compressed_size),
			(int)floor(double(total_compressed_size) * 100 / total_size));
		printf("Done.\n");
	}

	void DisplayHelp()
	{
		printf("CaRpg paker v2. Switches:\n"
			"-?/h/help - help\n"
			"-e/encrypt pswd - encrypt file entries with password\n"
			"-fe/fullencrypt pswd - full encrypt with password\n"
			"-nc/nocompress - don't compress\n"
			"-o/output filename - output filename (default \"data.pak\")\n"
			"-ns/nosubdir - don't process subdirectories\n"
			"-k/key pswd - encryption key\n"
			"-b/browse filename - display list of files\n"
			"-u/unpack filename - unpack files from pak\n"
			"Parameters without '-' are treated as files/directories.\n");
		done_anything = true;
	}

	Cmd GetCommand(cstring arg)
	{
		auto it = cmds.find(arg);
		if(it == cmds.end())
			return None;
		else
			return it->second;
	}

	cstring GetSize(uint size)
	{
		double dsize = (double)size;
		int mag = 0;
		while(dsize >= 500)
		{
			dsize /= 1024;
			++mag;
		}
		if(mag == 0)
			return Format("%u B", size);

		if(dsize > 250)
			dsize = floor(dsize / 10) * 10;
		else if(dsize > 100)
			dsize = floor(dsize);
		else if(dsize > 10)
			dsize = floor(dsize * 10) / 10;
		else
			dsize = floor(dsize * 100) / 100;
		switch(mag)
		{
		case 1:
			return Format("%g KB", dsize);
		case 2:
			return Format("%g MB", dsize);
		case 3:
		default:
			return Format("%g GB", dsize);
		}
	}

	Pak* OpenPak(cstring path)
	{
		// open file
		printf("Opening pak '%s'.\n", path);
		HANDLE file = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if(file == INVALID_HANDLE_VALUE)
		{
			printf("ERROR: Failed to open (%d).\n", GetLastError());
			return nullptr;
		}

		// read header
		Pak::Header header;
		if(!ReadFile(file, &header, sizeof(header), &tmp, nullptr))
		{
			printf("ERROR: Unexpected end of file at header.\n");
			CloseHandle(file);
			return nullptr;
		}
		if(!header.HaveValidSign())
		{
			printf("ERROR: Invalid file signature.\n");
			CloseHandle(file);
			return nullptr;
		}
		if(header.version != Pak::CURRENT_VERSION)
		{
			printf("ERROR: Unsupported version %u (current version is %u).\n", header.version, Pak::CURRENT_VERSION);
			CloseHandle(file);
			return nullptr;
		}

		// read file list
		Pak* pak = new Pak;
		pak->file = file;
		pak->table = new byte[header.file_entry_table_size];
		if(!ReadFile(file, pak->table, header.file_entry_table_size, &tmp, nullptr))
		{
			printf("ERROR: Unexpected end of file at file entry table.\n");
			delete pak;
			return nullptr;
		}

		// decrypt
		if(header.flags & Pak::F_ENCRYPTION)
		{
			if(decrypt_key.empty())
			{
				printf("ERROR: Missing decryption key, use -key to enter it.\n");
				delete pak;
				return nullptr;
			}

			Crypt((char*)pak->table, header.file_entry_table_size, decrypt_key.c_str(), decrypt_key.length());
		}
		pak->encrypted = (header.flags & Pak::F_FULL_ENCRYPTION) != 0;
		pak->file_count = header.file_count;

		// verify file list, set name
		DWORD total_size = GetFileSize(file, nullptr);
		for(uint i = 0; i < pak->file_count; ++i)
		{
			Pak::File& f = pak->file_table[i];
			f.filename = (cstring)pak->table + f.filename_offset;
			if(f.offset + f.compressed_size > total_size)
			{
				printf("ERROR: Broken file entry (index %u, name %s).\n", i, f.filename);
				delete pak;
				return nullptr;
			}
		}

		return pak;
	}

	void ProcessCommandLine(int argc, char** argv)
	{
		// process parameters
		for(int i = 1; i < argc; ++i)
		{
			if(argv[i][0] == '-')
			{
				cstring arg = argv[i] + 1;
				Cmd cmd = GetCommand(arg);

				switch(cmd)
				{
				case Help:
					DisplayHelp();
					break;
				case Encrypt:
					if(i < argc)
					{
						printf("Using encryption.\n");
						crypt_key = argv[++i];
						encrypt = true;
						full_encrypt = false;
					}
					else
						printf("ERROR: Missing encryption password.\n");
					break;
				case FullEncrypt:
					if(i < argc)
					{
						printf("Using full encryption.\n");
						crypt_key = argv[++i];
						encrypt = true;
						full_encrypt = true;
					}
					else
						printf("ERROR: Missing encryption password.\n");
					break;
				case NoCompress:
					printf("Not using compression.\n");
					compress = false;
					break;
				case NoSubdir:
					printf("Disabled processing subdirectories.\n");
					subdir = false;
					break;
				case Output:
					if(i < argc)
					{
						++i;
						printf("Using output filename '%s'.\n", argv[i]);
						output = argv[i];
					}
					else
						printf("ERROR: Missing output filename.\n");
					break;
				case Key:
					if(i < argc)
					{
						printf("Using decrypt key,\n");
						decrypt_key = argv[++i];
					}
					else
						printf("ERROR: Missing decryption password.\n");
					break;
				case Browse:
					if(i < argc)
						BrowsePak(argv[++i]);
					else
						printf("ERROR: Missing pak filename.\n");
					break;
				case Unpack:
					if(i < argc)
						UnpackPak(argv[++i]);
					else
						printf("ERROR: Missing pak filename.\n");
					break;
				default:
					printf("ERROR: Invalid switch '%s'.\n", argv[i]);
					break;
				}
			}
			else
				Add(argv[i], files, subdir);
		}
	}

	int Run(int argc, char** argv)
	{
		ProcessCommandLine(argc, argv);

		if(files.empty())
		{
			if(!done_anything)
				printf("No input files. Use '%s -?' to get help.\n", argv[0]);
			return 0;
		}

		return SavePak() ? 0 : 1;
	}

	bool SavePak()
	{
		// open pak
		printf("Creating pak file...\n");
		HANDLE pak = CreateFile(output.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if(pak == INVALID_HANDLE_VALUE)
		{
			DWORD result = GetLastError();
			printf("ERROR: Failed to open '%s' (%u).\n", output.c_str(), result);
			return false;
		}

		// calculate data size & offset
		const uint header_size = 16;
		const uint entries_offset = header_size;
		const uint entries_size = files.size() * 16;
		const uint names_offset = entries_offset + entries_size;
		uint names_size = 0;
		uint offset = names_offset - header_size;
		for(File& f : files)
		{
			f.name_offset = offset;
			uint len = 1 + f.name.length();
			offset += len;
			names_size += len;
		}
		const uint table_size = entries_size + names_size;
		const uint data_offset = entries_offset + table_size;

		// write header
		printf("Writing header...\n");
		DWORD tmp;
		char sign[4] = { 'P', 'A', 'K', Pak::CURRENT_VERSION };
		WriteFile(pak, sign, sizeof(sign), &tmp, NULL);
		int flags = 0;
		if(encrypt)
			flags |= Pak::F_ENCRYPTION;
		if(full_encrypt)
			flags |= Pak::F_FULL_ENCRYPTION;
		WriteFile(pak, &flags, sizeof(flags), &tmp, NULL);
		uint count = files.size();
		WriteFile(pak, &count, sizeof(count), &tmp, NULL);
		WriteFile(pak, &table_size, sizeof(table_size), &tmp, NULL);

		// write data
		printf("Writing files data...\n");
		vector<byte> buf;
		vector<byte> cbuf;
		offset = data_offset;
		SetFilePointer(pak, offset, NULL, FILE_BEGIN);
		for(File& f : files)
		{
			f.offset = offset;

			// read file to buf
			HANDLE file = CreateFile(f.path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if(file == INVALID_HANDLE_VALUE)
			{
				DWORD result = GetLastError();
				printf("ERROR: Failed to open file '%s' (%u).", f.path.c_str(), result);
				f.size = 0;
				f.compressed_size = 0;
				continue;
			}
			buf.resize(f.size);
			ReadFile(file, &buf[0], f.size, &tmp, NULL);
			CloseHandle(file);

			// compress if required
			byte* b = NULL;
			uint size;
			if(compress)
			{
				uLong cbuf_size = compressBound(f.size);
				cbuf.resize(cbuf_size);
				::compress(&cbuf[0], &cbuf_size, &buf[0], f.size);
				if(cbuf_size < f.size)
				{
					b = &cbuf[0];
					f.compressed_size = cbuf_size;
					size = cbuf_size;
				}
			}
			if(!b)
			{
				b = &buf[0];
				size = f.size;
				f.compressed_size = f.size;
			}
			if(full_encrypt)
				Crypt((char*)b, f.compressed_size, crypt_key.c_str(), crypt_key.length());

			// write
			WriteFile(pak, b, size, &tmp, NULL);
			offset += size;
		}

		if(!encrypt)
		{
			printf("Writing file entries...\n");

			// file entries
			SetFilePointer(pak, entries_offset, NULL, FILE_BEGIN);
			for(File& f : files)
			{
				WriteFile(pak, &f.name_offset, sizeof(f.name_offset), &tmp, NULL);
				WriteFile(pak, &f.size, sizeof(f.size), &tmp, NULL);
				WriteFile(pak, &f.compressed_size, sizeof(f.compressed_size), &tmp, NULL);
				WriteFile(pak, &f.offset, sizeof(f.offset), &tmp, NULL);
			}

			// file names
			byte zero = 0;
			for(File& f : files)
			{
				WriteFile(pak, f.name.c_str(), f.name.length(), &tmp, NULL);
				WriteFile(pak, &zero, 1, &tmp, NULL);
			}
		}
		else
		{
			printf("Writing encrypted file entries...\n");

			buf.resize(table_size);
			byte* b = buf.data();

			// file entries
			for(File& f : files)
			{
				memcpy(b, &f.name_offset, sizeof(f.name_offset));
				b += 4;
				memcpy(b, &f.size, sizeof(f.size));
				b += 4;
				memcpy(b, &f.compressed_size, sizeof(f.compressed_size));
				b += 4;
				memcpy(b, &f.offset, sizeof(f.offset));
				b += 4;
			}

			// file names
			byte zero = 0;
			for(File& f : files)
			{
				memcpy(b, f.name.c_str(), f.name.length());
				b += f.name.length();
				*b = 0;
				++b;
			}

			Crypt((char*)buf.data(), table_size, crypt_key.c_str(), crypt_key.length());

			SetFilePointer(pak, entries_offset, NULL, FILE_BEGIN);
			WriteFile(pak, buf.data(), buf.size(), &tmp, NULL);
		}

		CloseHandle(pak);
		printf("Done.\n");

		return true;
	}

	void UnpackPak(cstring path)
	{
		done_anything = true;

		Pak* pak = OpenPak(path);
		if(!pak)
			return;

		vector<byte> buf, buf2;

		printf("Unpacking files: %u\n", pak->file_count);
		for(uint i = 0; i < pak->file_count; ++i)
		{
			// read
			Pak::File& f = pak->file_table[i];
			buf.resize(f.compressed_size);
			SetFilePointer(pak->file, f.offset, nullptr, FILE_BEGIN);
			if(!ReadFile(pak->file, buf.data(), f.compressed_size, &tmp, nullptr))
			{
				printf("  ERROR: Failed to read file (name %s, offset %u, size %u).\n", f.filename, f.offset, f.compressed_size);
				continue;
			}

			// decrypt
			if(pak->encrypted)
				Crypt((char*)buf.data(), buf.size(), decrypt_key.c_str(), decrypt_key.length());

			// decompress
			vector<byte>* result;
			if(f.compressed_size == f.size)
				result = &buf;
			else
			{
				uint real_size = f.size;
				buf2.resize(real_size);
				uncompress((Bytef*)buf2.data(), (uLongf*)&real_size, (const Bytef*)buf.data(), f.compressed_size);
				result = &buf2;
			}

			// save
			HANDLE out_file = CreateFile(f.filename, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
			if(out_file == INVALID_HANDLE_VALUE)
			{
				printf("  ERROR: Failed to create output file '%s'.\n", f.filename);
				continue;
			}
			WriteFile(out_file, result->data(), result->size(), &tmp, nullptr);
			CloseHandle(out_file);
		}
		printf("Done.\n");

		delete pak;
	}
};

int main(int argc, char** argv)
{
	Paker paker;
	return paker.Run(argc, argv);
}

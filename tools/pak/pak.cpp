#define CARPGLIB_USE_ZLIB
#include <CarpgLibCore.h>
#include <File.h>
#include <Crc.h>

struct Pak
{
	static const byte CURRENT_VERSION = 1;

	struct Header
	{
		char sign[3];
		byte version;
		int flags;
		uint fileCount, fileEntryTableSize;

		bool HaveValidSign()
		{
			return sign[0] == 'P' && sign[1] == 'A' && sign[2] == 'K';
		}
	};

	struct File
	{
		union
		{
			cstring filename;
			uint filenameOffset;
		};
		uint size;
		uint compressedSize;
		uint offset;
	};

	enum Flags
	{
		F_ENCRYPTION = 1 << 0,
		F_FULL_ENCRYPTION = 1 << 1
	};

	FileReader file;
	union
	{
		byte* table;
		File* fileTable;
	};
	uint fileCount;
	bool encrypted;

	~Pak()
	{
		delete[] table;
	}
};

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
		Unpack,
		Path,
		Crc
	};

	struct File
	{
		string path, name;
		uint size, compressedSize, offset, nameOffset;
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
		{"u", Unpack }, {"unpack", Unpack },
		{"path", Path}
	};

	vector<File> files;
	string cryptKey, decryptKey, output;
	bool compress, encrypt, fullEncrypt, subdir, doneAnything, fullPath;

	Paker()
	{
		output = "data.pak";
		compress = true;
		encrypt = false;
		fullEncrypt = false;
		subdir = true;
		doneAnything = false;
		fullPath = false;
	}

	void Add(cstring path, vector<File>& files, bool subdir, uint pathOffset = 0)
	{
		printf("Scanning: %s ...\n", path);
		if(pathOffset == 0)
			pathOffset = strlen(path) + 1;

		// start find
		io::FindFiles(path, [&](const io::FileInfo& fileInfo)
		{
			if(strcmp(fileInfo.filename, ".") != 0 && strcmp(fileInfo.filename, "..") != 0)
			{
				string newPath = io::CombinePath(path, fileInfo.filename);
				if(fileInfo.isDir)
				{
					// directory
					if(subdir)
					{
						newPath += "/*";
						Add(newPath.c_str(), files, true, pathOffset);
					}
				}
				else
				{
					File f;
					f.path = newPath;
					if(fullPath)
						f.name = newPath.substr(pathOffset);
					else
						f.name = fileInfo.filename;
					f.size = fileInfo.size;
					f.nameOffset = 0;
					files.push_back(f);
				}
			}
			return true;
		});
	}

	void BrowsePak(cstring path)
	{
		doneAnything = true;

		Pak* pak = OpenPak(path);
		if(!pak)
			return;

		printf("Browsing files: %u\n", pak->fileCount);
		uint totalSize = 0u, totalCompressedSize = 0u;
		for(uint i = 0; i < pak->fileCount; ++i)
		{
			Pak::File& f = pak->fileTable[i];
			if(f.compressedSize == f.size)
				printf("  %s - size %s, offset %u\n", f.filename, GetSize(f.size), f.offset);
			else
				printf("  %s - size %s, compressed %s, offset %u\n", f.filename, GetSize(f.size), GetSize(f.compressedSize), f.offset);
			totalSize += f.size;
			totalCompressedSize += f.compressedSize;
		}
		printf("Size: %s, compressed %s (%d%%)\n", GetSize(totalSize), GetSize(totalCompressedSize), (int)floor(double(totalCompressedSize) * 100 / totalSize));
		printf("Done.\n");

		delete pak;
	}

	void DisplayHelp()
	{
		printf("CaRpg paker v1. Switches:\n"
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
		doneAnything = true;
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
		FileReader f(path);
		if(!f)
		{
			printf("ERROR: Failed to open.\n");
			return nullptr;
		}

		// read header
		Pak::Header header;
		f >> header;
		if(!f)
		{
			printf("ERROR: Unexpected end of file at header.\n");
			return nullptr;
		}
		if(!header.HaveValidSign())
		{
			printf("ERROR: Invalid file signature.\n");
			return nullptr;
		}
		if(header.version != Pak::CURRENT_VERSION)
		{
			printf("ERROR: Unsupported version %u (current version is %u).\n", header.version, Pak::CURRENT_VERSION);
			return nullptr;
		}

		// read file list
		Pak* pak = new Pak;
		pak->table = new byte[header.fileEntryTableSize];
		f.Read(pak->table, header.fileEntryTableSize);
		if(!f)
		{
			printf("ERROR: Unexpected end of file at file entry table.\n");
			delete pak;
			return nullptr;
		}

		// decrypt
		if(header.flags & Pak::F_ENCRYPTION)
		{
			if(decryptKey.empty())
			{
				printf("ERROR: Missing decryption key, use -key to enter it.\n");
				delete pak;
				return nullptr;
			}

			io::Crypt((char*)pak->table, header.fileEntryTableSize, decryptKey.c_str(), decryptKey.length());
		}
		pak->encrypted = (header.flags & Pak::F_FULL_ENCRYPTION) != 0;
		pak->fileCount = header.fileCount;

		// verify file list, set name
		uint totalSize = f.GetSize();
		for(uint i = 0; i < pak->fileCount; ++i)
		{
			Pak::File& f = pak->fileTable[i];
			f.filename = (cstring)pak->table + f.filenameOffset;
			if(f.offset + f.compressedSize > totalSize)
			{
				printf("ERROR: Broken file entry (index %u, name %s).\n", i, f.filename);
				delete pak;
				return nullptr;
			}
		}

		pak->file = f;
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
						cryptKey = argv[++i];
						encrypt = true;
						fullEncrypt = false;
					}
					else
						printf("ERROR: Missing encryption password.\n");
					break;
				case FullEncrypt:
					if(i < argc)
					{
						printf("Using full encryption.\n");
						cryptKey = argv[++i];
						encrypt = true;
						fullEncrypt = true;
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
						decryptKey = argv[++i];
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
				case Path:
					fullPath = true;
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
			if(!doneAnything)
				printf("No input files. Use '%s -?' to get help.\n", argv[0]);
			return 0;
		}

		if(SavePak())
		{
			printf(Format("Crc: %u\nDone.\n", Crc::Calculate(output)));
			return 0;
		}
		else
			return 1;
	}

	bool SavePak()
	{
		// open pak
		printf("Creating pak file...\n");
		FileWriter pak(output);
		if(!pak)
		{
			printf("ERROR: Failed to open '%s'.\n", output.c_str());
			return false;
		}

		// calculate data size & offset
		const uint headerSize = 16;
		const uint entriesOffset = headerSize;
		const uint entriesSize = files.size() * 16;
		const uint namesOffset = entriesOffset + entriesSize;
		uint namesSize = 0;
		uint offset = namesOffset - headerSize;
		for(File& f : files)
		{
			f.nameOffset = offset;
			uint len = 1 + f.name.length();
			offset += len;
			namesSize += len;
		}
		const uint tableSize = entriesSize + namesSize;
		const uint dataOffset = entriesOffset + tableSize;

		// write header
		printf("Writing header...\n");
		char sign[4] = { 'P', 'A', 'K', Pak::CURRENT_VERSION };
		pak.Write(sign, sizeof(sign));
		int flags = 0;
		if(encrypt)
			flags |= Pak::F_ENCRYPTION;
		if(fullEncrypt)
			flags |= Pak::F_FULL_ENCRYPTION;
		pak << flags;
		pak << files.size();
		pak << tableSize;

		// write data
		printf("Writing files data...\n");
		offset = dataOffset;
		pak.SetPos(offset);
		for(File& f : files)
		{
			f.offset = offset;

			// read file to buf
			FileReader file(f.path);
			Buffer* buf = file.ReadToBuffer(f.size);
			if(!file)
			{
				printf("ERROR: Failed to open file '%s'.", f.path.c_str());
				f.size = 0;
				f.compressedSize = 0;
				continue;
			}

			// compress & encrypt
			f.size = buf->Size();
			buf = buf->TryCompress();
			f.compressedSize = buf->Size();
			if(fullEncrypt)
				io::Crypt((char*)buf->Data(), f.compressedSize, cryptKey.c_str(), cryptKey.length());

			// write
			pak.Write(buf->Data(), buf->Size());
			buf->Free();
			offset += f.compressedSize;
		}

		if(!encrypt)
		{
			printf("Writing file entries...\n");

			// file entries
			pak.SetPos(entriesOffset);
			for(File& f : files)
			{
				pak << f.nameOffset;
				pak << f.size;
				pak << f.compressedSize;
				pak << f.offset;
			}

			// file names
			for(File& f : files)
				pak.WriteString0(f.name);
		}
		else
		{
			printf("Writing encrypted file entries...\n");

			Buffer* buf = Buffer::Get();
			buf->Resize(tableSize);
			byte* b = (byte*)buf->Data();

			// file entries
			for(File& f : files)
			{
				memcpy(b, &f.nameOffset, sizeof(f.nameOffset));
				b += 4;
				memcpy(b, &f.size, sizeof(f.size));
				b += 4;
				memcpy(b, &f.compressedSize, sizeof(f.compressedSize));
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

			io::Crypt((char*)buf->Data(), tableSize, cryptKey.c_str(), cryptKey.length());

			pak.SetPos(entriesOffset);
			pak.Write(buf->Data(), buf->Size());
		}

		return true;
	}

	void UnpackPak(cstring path)
	{
		doneAnything = true;

		Pak* pak = OpenPak(path);
		if(!pak)
			return;

		printf("Unpacking files: %u\n", pak->fileCount);
		for(uint i = 0; i < pak->fileCount; ++i)
		{
			// read
			Pak::File& f = pak->fileTable[i];
			pak->file.SetPos(f.offset);
			Buffer* buf = pak->file.ReadToBuffer(f.compressedSize);
			if(!pak->file)
			{
				printf("  ERROR: Failed to read file (name %s, offset %u, size %u).\n", f.filename, f.offset, f.compressedSize);
				continue;
			}

			// decrypt
			if(pak->encrypted)
				io::Crypt((char*)buf->Data(), buf->Size(), decryptKey.c_str(), decryptKey.length());

			// decompress
			if(f.compressedSize != f.size)
				buf = buf->Decompress(f.size);

			// save
			FileWriter::WriteAll(f.filename, buf);
			buf->Free();
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

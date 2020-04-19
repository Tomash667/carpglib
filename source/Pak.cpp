#include "Pch.h"
#include "Pak.h"

PakWriter::PakWriter(bool fullPath) : flags(0), ok(true), compress(true), fullPath(fullPath)
{
}

void PakWriter::AddFile(cstring path)
{
	assert(path);
	uint size = io::GetFileSize(path);
	ok = ok && size != (uint)-1;
	File file;
	file.path = path;
	file.name = fullPath ? file.path : io::FilenameFromPath(file.path);
	file.size = size;
	files.push_back(file);
}

void PakWriter::AddEmptyFile(cstring path)
{
	assert(path);
	File file;
	file.path = path;
	file.size = 0;
	files.push_back(file);
}

void PakWriter::Encrypt(cstring key, bool full)
{
	this->key = key;
	flags = full ? (Pak::FullEncrypted | Pak::Encrypted) : Pak::Encrypted;
}

bool PakWriter::Write(cstring path)
{
	// open
	FileWriter f;
	if(!f.Open(path))
		return false;

	// header
	f.WriteStringF("PAK");
	f << (byte)2;
	f << flags;
	f << files.size();
	f << 0;
	f << 0;

	// custom data
	const uint headerSize = 20;
	WriteCustomData();
	uint customDataSize = f.GetPos() - headerSize;

	// calculate data size & offset
	const uint entriesOffset = headerSize + customDataSize;
	const uint entriesSize = files.size() * 16;
	const uint namesOffset = entriesOffset + entriesSize;
	uint namesSize = 0;
	uint offset = namesOffset - entriesOffset;
	for(File& file : files)
	{
		file.nameOffset = offset;
		uint nameLen = file.name.length() + 1;
		namesSize += nameLen;
		offset += nameLen;
	}
	const uint tableSize = entriesSize + namesSize;
	const uint dataOffset = entriesOffset + tableSize;

	// patch header
	uint pos = f.GetPos();
	f.SetPos(12);
	f << tableSize;
	f << customDataSize;
	f.SetPos(pos);

	// data
	f.SetPos(dataOffset); FIXME; // propably won't work
	offset = dataOffset;
	for(File& file : files)
	{
		file.dataOffset = offset;
		if(file.size)
		{
			Buffer* buf = FileReader::ReadToBuffer(file.path);
			// compress if required
			if(compress)
				buf = buf->Compress();
			file.compressedSize = buf->Size();
			// encrypt if required
			if(IsSet(flags, Pak::FullEncrypted))
				buf->Crypt(key);
			// save
			f << buf;
		}
		else
			file.compressedSize = file.size;
		offset += file.compressedSize;
	}

	// file entries
	f.SetPos(entriesOffset);
	if(!IsSet(flags, Pak::Encrypted))
	{
		// file entries
		for(File& file : files)
		{
			f << file.nameOffset;
			f << file.size;
			f << file.compressedSize;
			f << file.dataOffset;
		}

		// file names
		for(File& file : files)
			f.WriteString0(file.name);
	}
	else
	{
		MemoryWriter mem;
		mem.Resize(tableSize);

		// file entries
		for(File& file : files)
		{
			mem << file.nameOffset;
			mem << file.size;
			mem << file.compressedSize;
			mem << file.dataOffset;
		}

		// file names
		for(File& file : files)
			mem.WriteString0(file.name);

		Buffer* buf = mem.PinBuffer();
		buf->Crypt(key);
		f << buf;
		buf->Free();
	}

	return true;
}

bool PakWriter::Read(cstring path)
{
	// open
	FileReader f;
	if(!f.Open(path))
		return nullptr;

	// read header
	Pak::Header head;
	f.Read(&head, sizeof(Pak::Header) - 4); // don't read ver 1 optional field
	if(!head.IsValid())
		return nullptr;

	// custom data
	if(head.version == 2)
	{
		f >> head.custom_data_size;
		ReadCustomData(f, head.custom_data_size);
	}

	// entries
	files.resize(head.files_count);
	if(!IsSet(head.flags, Pak::Encrypted))
	{
		for(File& file : files)
		{
			f >> file.nameOffset;
			f >> file.size;
			f >> file.compressedSize;
			f >> file.dataOffset;
		}
		const uint offset = head.files_count * 16;
		Buffer* buf = f.ReadToBuffer(head.file_entry_table_size - offset);
		for(File& file : files)
			file.name = ((char*)buf->Data()) + file.nameOffset - offset;
		buf->Free();
	}
	else
	{
		Buffer* buf = f.ReadToBuffer(head.file_entry_table_size);
		buf->Crypt(key);
		MemoryReader mem(buf);
		for(File& file : files)
		{
			mem >> file.nameOffset;
			mem >> file.size;
			mem >> file.compressedSize;
			mem >> file.dataOffset;
		}
		for(File& file : files)
			file.name = ((char*)buf->Data()) + file.nameOffset;
	}

	return true;
}

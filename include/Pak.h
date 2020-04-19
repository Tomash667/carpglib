#pragma once

#include "File.h"

//-----------------------------------------------------------------------------
// Check tools/pak/pak.txt for specification
struct Pak
{
	enum Flags
	{
		Encrypted = 0x01,
		FullEncrypted = 0x02
	};

	struct Header
	{
		char sign[3];
		byte version;
		int flags;
		uint files_count;
		uint file_entry_table_size;
		uint custom_data_size;

		bool IsValid() const
		{
			return sign[0] == 'P' && sign[1] == 'A' && sign[2] == 'K' && Any(version, 1, 2);
		}
	};

	struct File
	{
		union
		{
			cstring filename;
			uint filename_offset;
		};
		uint size;
		uint compressed_size;
		uint offset;
	};

	string path, key;
	FileReader file;
	File* files;
	Buffer* filename_buf;
	bool encrypted;
};

//-----------------------------------------------------------------------------
struct PakWriter
{
	struct File
	{
		string path, name;
		uint size;
		uint compressedSize;
		uint nameOffset;
		uint dataOffset;
	};

	PakWriter(bool fullPath = false);
	virtual ~PakWriter() {}
	void AddFile(cstring path);
	void AddEmptyFile(cstring path);
	void Encrypt(cstring key, bool full);
	void UseCompress(bool compress) { this->compress = compress; }
	bool Write(cstring path);
	bool Read(cstring path);
	virtual void WriteCustomData(FileWriter& f) { }
	virtual void ReadCustomData(FileReader& f, uint size) { f.Skip(size); }
	bool IsOk() const { return ok; }
	vector<File>& GetFiles() { return files; }

protected:
	int flags;
	vector<File> files;
	string key;
	bool ok, compress, fullPath;
};

#pragma once

#include "File.h"

//-----------------------------------------------------------------------------
// Check tools/pak/pak.txt for specification
class Pak
{
public:
	enum Flags
	{
		Encrypted = 0x01,
		FullEncrypted = 0x02
	};

	struct Header
	{
		char sign[3];
		byte version;
		uint flags;
		uint filesCount;
		uint fileEntryTableSize;
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

	string path, key;
	FileReader file;
	File* files;
	Buffer* filenameBuf;
	bool encrypted;
};

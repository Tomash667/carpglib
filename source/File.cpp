#include "Pch.h"
#include "File.h"
#include <zlib.h>
#define INCLUDE_SHELLAPI
#include "WindowsIncludes.h"

//-----------------------------------------------------------------------------
static DWORD tmp;
string StreamReader::buf;
char BUF[256];


//-----------------------------------------------------------------------------
static_assert(sizeof(uint64) == sizeof(FILETIME), "Invalid FileTime size.");

int FileTime::Compare(const FileTime& fileTime) const
{
	FILETIME ft1 = union_cast<FILETIME>(time);
	FILETIME ft2 = union_cast<FILETIME>(fileTime);
	return CompareFileTime(&ft1, &ft2);
}


//-----------------------------------------------------------------------------
FileReader::FileReader(FileHandle file) : file(file), ownHandle(false)
{
	if(file != INVALID_HANDLE_VALUE)
		size = GetFileSize(file, nullptr);
	else
		size = 0;
}

FileReader::~FileReader()
{
	if(ownHandle && file != INVALID_HANDLE_VALUE)
	{
		CloseHandle(file);
		file = INVALID_HANDLE_VALUE;
		ok = false;
	}
}

void FileReader::operator = (FileReader& f)
{
	assert(file == INVALID_HANDLE_VALUE);
	file = f.file;
	ownHandle = f.ownHandle;
	ok = f.ok;
	size = f.size;
	f.file = INVALID_HANDLE_VALUE;
}

bool FileReader::Open(Cstring filename)
{
	file = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	ownHandle = true;
	if(file != INVALID_HANDLE_VALUE)
	{
		size = GetFileSize(file, nullptr);
		ok = true;
	}
	else
	{
		size = 0;
		ok = false;
	}
	return ok;
}

void FileReader::Close()
{
	if(file != INVALID_HANDLE_VALUE)
	{
		if(ownHandle)
			CloseHandle(file);
		file = INVALID_HANDLE_VALUE;
		ok = false;
	}
}

void FileReader::Read(void* ptr, uint size)
{
	[[maybe_unused]] BOOL result = ReadFile(file, ptr, size, &tmp, nullptr);
	assert(result != FALSE);
	ok = (size == tmp);
}

void FileReader::ReadToString(string& s)
{
	DWORD size = GetFileSize(file, nullptr);
	s.resize(size);
	[[maybe_unused]] BOOL result = ReadFile(file, (char*)s.c_str(), size, &tmp, nullptr);
	assert(result != FALSE);
	assert(size == tmp);
}

Buffer* FileReader::ReadToBuffer(Cstring path)
{
	FileReader f(path);
	if(!f)
		return nullptr;
	Buffer* buffer = Buffer::Get();
	buffer->Resize(f.GetSize());
	f.Read(buffer->Data(), buffer->Size());
	return buffer;
}

Buffer* FileReader::ReadToBuffer(Cstring path, uint offset, uint size)
{
	FileReader f(path);
	if(!f)
		return nullptr;
	f.Skip(offset);
	if(!f.Ensure(size))
		return nullptr;
	Buffer* buffer = Buffer::Get();
	buffer->Resize(size);
	f.Read(buffer->Data(), size);
	return buffer;
}

void FileReader::Skip(uint bytes)
{
	ok = (ok && SetFilePointer(file, bytes, nullptr, FILE_CURRENT) != INVALID_SET_FILE_POINTER);
}

uint FileReader::GetPos() const
{
	return (uint)SetFilePointer(file, 0, nullptr, FILE_CURRENT);
}

FileTime FileReader::GetTime() const
{
	FILETIME fileTime;
	GetFileTime((HANDLE)file, nullptr, nullptr, &fileTime);
	return union_cast<FileTime>(fileTime);
}

bool FileReader::SetPos(uint pos)
{
	if(!ok || pos > size)
	{
		pos = size;
		ok = false;
		return false;
	}
	SetFilePointer((HANDLE)file, pos, nullptr, FILE_BEGIN);
	return true;
}


//-----------------------------------------------------------------------------
MemoryReader::MemoryReader(BufferHandle& bufHandle) : buf(*bufHandle.Pin())
{
	ok = true;
	pos = 0;
}

MemoryReader::MemoryReader(Buffer* buf) : buf(*buf)
{
	ok = true;
	pos = 0;
}

MemoryReader::~MemoryReader()
{
	buf.Free();
}

void MemoryReader::Read(void* ptr, uint size)
{
	if(!ok || GetPos() + size > GetSize())
		ok = false;
	else
	{
		memcpy(ptr, (byte*)buf.Data() + pos, size);
		pos += size;
	}
}

void MemoryReader::Skip(uint size)
{
	if(!ok || GetPos() + size > GetSize())
		ok = false;
	else
		pos += size;
}

bool MemoryReader::SetPos(uint pos)
{
	if(!ok || pos > GetSize())
		ok = false;
	else
		this->pos = pos;
	return ok;
}


//-----------------------------------------------------------------------------
FileWriter::~FileWriter()
{
	if(ownHandle && file != INVALID_HANDLE_VALUE)
	{
		CloseHandle(file);
		file = INVALID_HANDLE_VALUE;
	}
}

bool FileWriter::Open(cstring filename)
{
	assert(filename);
	file = CreateFile(filename, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	return (file != INVALID_HANDLE_VALUE);
}

void FileWriter::Close()
{
	if(file != INVALID_HANDLE_VALUE)
	{
		if(ownHandle)
			CloseHandle(file);
		file = INVALID_HANDLE_VALUE;
	}
}

void FileWriter::Write(const void* ptr, uint size)
{
	WriteFile(file, ptr, size, &tmp, nullptr);
	assert(size == tmp);
}

void FileWriter::Flush()
{
	FlushFileBuffers(file);
}

uint FileWriter::GetSize() const
{
	return GetFileSize(file, nullptr);
}

uint FileWriter::GetPos() const
{
	return SetFilePointer(file, 0, nullptr, FILE_CURRENT);
}

void FileWriter::operator = (FileWriter& f)
{
	assert(file == INVALID_FILE_HANDLE && f.file != INVALID_HANDLE_VALUE);
	file = f.file;
	ownHandle = f.ownHandle;
	f.file = INVALID_FILE_HANDLE;
}

void FileWriter::SetTime(FileTime fileTime)
{
	FILETIME ft = union_cast<FILETIME>(fileTime);
	SetFileTime((HANDLE)file, nullptr, nullptr, &ft);
}

bool FileWriter::SetPos(uint pos)
{
	SetFilePointer(file, pos, nullptr, FILE_BEGIN);
	return true;
}

bool FileWriter::WriteAll(Cstring filename, Buffer* buf)
{
	FileWriter f(filename);
	if(!f)
		return false;
	f.Write(buf->Data(), buf->Size());
	return true;
}


//=================================================================================================
void io::CreateDirectory(Cstring dir)
{
	CreateDirectoryA(dir, nullptr);
}

//=================================================================================================
void io::CreateDirectories(Cstring dirs)
{
	GetFullPathNameA(dirs, 256, BUF, nullptr);
	SHCreateDirectoryExA(nullptr, BUF, nullptr);
}

//=================================================================================================
bool io::DeleteDirectory(Cstring dir)
{
	MakeDoubleZeroTerminated(BUF, dir);

	SHFILEOPSTRUCT op = {
		nullptr,
		FO_DELETE,
		BUF,
		nullptr,
		FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT,
		FALSE,
		nullptr,
		nullptr
	};

	return SHFileOperation(&op) == 0;
}

//=================================================================================================
bool io::DirectoryExists(cstring dir)
{
	assert(dir);

	DWORD attrib = GetFileAttributes(dir);
	if(attrib == INVALID_FILE_ATTRIBUTES)
		return false;

	return IsSet(attrib, FILE_ATTRIBUTE_DIRECTORY);
}

//=================================================================================================
bool io::FileExists(cstring filename)
{
	assert(filename);

	DWORD attrib = GetFileAttributes(filename);
	if(attrib == INVALID_FILE_ATTRIBUTES)
		return false;

	return !IsSet(attrib, FILE_ATTRIBUTE_DIRECTORY);
}

//=================================================================================================
bool io::DeleteFile(cstring filename)
{
	assert(filename);

	return DeleteFileA(filename) != 0;
}

//=================================================================================================
FileTime io::GetFileTime(cstring filename)
{
	assert(filename);

	HANDLE file = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if(file == INVALID_HANDLE_VALUE)
	{
		FileTime fileTime;
		fileTime.time = 0;
		return fileTime;
	}

	FILETIME fileTime;
	GetFileTime(file, nullptr, nullptr, &fileTime);
	CloseHandle(file);
	return union_cast<FileTime>(fileTime);
}

//=================================================================================================
void io::MoveFile(cstring filename, cstring newFilename)
{
	if(MoveFileExA(filename, newFilename, MOVEFILE_REPLACE_EXISTING) == 0)
	{
		uint error = GetLastError();
		Error("Failed to move file '%s' to '%s' (%u).", filename, newFilename, error);
	}
}

//=================================================================================================
bool io::FindFiles(cstring pattern, delegate<bool(const FileInfo&)> func)
{
	assert(pattern);

	WIN32_FIND_DATA findData;
	HANDLE find = FindFirstFile(pattern, &findData);
	if(find == INVALID_HANDLE_VALUE)
		return false;

	do
	{
		// exclude special directories
		if(strcmp(findData.cFileName, ".") == 0 || strcmp(findData.cFileName, "..") == 0)
			continue;

		// callback
		FileInfo info =
		{
			findData.cFileName,
			findData.nFileSizeLow,
			IsSet(findData.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY)
		};
		if(!func(info))
			break;
	} while(FindNextFile(find, &findData) != 0);

	DWORD result = GetLastError();
	FindClose(find);
	return (result == ERROR_NO_MORE_FILES);
}

//=================================================================================================
void io::Execute(cstring file)
{
	assert(file);

	ShellExecute(nullptr, "open", file, nullptr, nullptr, SW_SHOWNORMAL);
}

//=================================================================================================
bool io::LoadFileToString(cstring path, string& str, uint maxSize)
{
	HANDLE file = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if(file == INVALID_HANDLE_VALUE)
		return false;

	uint fileSize = (uint)GetFileSize(file, nullptr);
	uint size = min(fileSize, maxSize);
	str.resize(size);

	ReadFile(file, (char*)str.c_str(), size, &tmp, nullptr);

	CloseHandle(file);

	return size == tmp;
}

//=================================================================================================
void io::Crypt(char* inp, uint inplen, cstring key, uint keylen)
{
	//we will consider size of sbox 256 bytes
	//(extra byte are only to prevent any mishep just in case)
	char Sbox[257], Sbox2[257];
	uint i = 0, j = 0;

	//always initialize the arrays with zero
	ZeroMemory(Sbox, sizeof(Sbox));
	ZeroMemory(Sbox2, sizeof(Sbox2));

	//initialize sbox i
	for(i = 0; i < 256U; i++)
	{
		Sbox[i] = (char)i;
	}

	//initialize the sbox2 with user key
	for(i = 0; i < 256U; i++)
	{
		if(j == keylen)
		{
			j = 0;
		}
		Sbox2[i] = key[j++];
	}

	j = 0; //Initialize j
		   //scramble sbox1 with sbox2
	for(i = 0; i < 256; i++)
	{
		j = (j + (uint)Sbox[i] + (uint)Sbox2[i]) % 256U;
		char temp = Sbox[i];
		Sbox[i] = Sbox[j];
		Sbox[j] = temp;
	}

	i = j = 0;
	for(uint x = 0; x < inplen; x++)
	{
		//increment i
		i = (i + 1U) % 256U;
		//increment j
		j = (j + (uint)Sbox[i]) % 256U;

		//Scramble SBox #1 further so encryption routine will
		//will repeat itself at great interval
		char temp = Sbox[i];
		Sbox[i] = Sbox[j];
		Sbox[j] = temp;

		//Get ready to create pseudo random  byte for encryption key
		uint t = ((uint)Sbox[i] + (uint)Sbox[j]) % 256U;

		//get the random byte
		char k = Sbox[t];

		//xor with the data and done
		inp[x] = (inp[x] ^ k);
	}
}

//=================================================================================================
cstring io::FilenameFromPath(const string& path)
{
	uint pos = path.find_last_of("/\\");
	if(pos == string::npos)
		return path.c_str();
	else
		return path.c_str() + pos + 1;
}

//=================================================================================================
cstring io::FilenameFromPath(cstring path)
{
	assert(path);
	cstring filename = FindLastOf(path, "/\\");
	if(filename)
		return filename + 1;
	else
		return path;
}

//=================================================================================================
// "file.txt" -> ""
// "dir/file.txt" -> "dir"
// "path/to/file.txt" -> "path/to"
string io::PathToDirectory(Cstring path)
{
	cstring filename = FindLastOf(path, "/\\");
	if(filename)
		return string(path, filename - path);
	else
		return "";
}

//=================================================================================================
string io::CombinePath(cstring path, cstring filename)
{
	string s;
	int pos = FindCharInString(path, "/\\");
	if(pos != -1)
	{
		s.assign(path, pos);
		s += '/';
		s += filename;
	}
	else
		s = filename;
	return s;
}

//=================================================================================================
void io::OpenUrl(Cstring url)
{
	ShellExecute(nullptr, "open", url, nullptr, nullptr, SW_SHOWNORMAL);
}

//=================================================================================================
cstring io::GetCurrentDirectory()
{
	char* str = GetFormatString();
	GetCurrentDirectoryA(MAX_PATH, str);
	return str;
}

//=================================================================================================
Buffer* io::Compress(byte* data, uint size)
{
	uint safeSize = compressBound(size);
	Buffer* buf = Buffer::Get();
	buf->Resize(safeSize);
	uint realSize = safeSize;
	compress(static_cast<Bytef*>(buf->Data()), (uLongf*)&realSize, data, size);
	buf->Resize(realSize);
	return buf;
}

//=================================================================================================
Buffer* io::TryCompress(byte* data, uint size)
{
	Buffer* buf = Compress(data, size);
	if(buf->Size() < size)
		return buf;
	buf->Free();
	return nullptr;
}

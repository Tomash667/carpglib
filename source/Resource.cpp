#include "EnginePch.h"
#include "EngineCore.h"
#include "Resource.h"
#include "Pak.h"

cstring Resource::GetPath() const
{
	if(!pak)
		return path.c_str();
	else
		return Format("%s/%s", pak->path.c_str(), filename);
}

//=================================================================================================
Buffer* Resource::GetBuffer()
{
	if(!pak)
		return FileReader::ReadToBuffer(path);

	Pak::File& file = pak->files[pak_index];
	Buffer* buf = pak->file.ReadToBuffer(file.offset, file.compressed_size);
	if(pak->encrypted)
		io::Crypt((char*)buf->Data(), buf->Size(), pak->key.c_str(), pak->key.length());
	if(file.compressed_size != file.size)
		buf = buf->Decompress(file.size);
	return buf;
}

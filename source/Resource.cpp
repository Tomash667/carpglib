#include "Pch.h"
#include "Resource.h"

#include "Pak.h"
#include "ResourceManager.h"

//=================================================================================================
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

	Pak::File& file = pak->files[pakIndex];
	Buffer* buf = pak->file.ReadToBuffer(file.offset, file.compressedSize);
	if(pak->encrypted)
		io::Crypt((char*)buf->Data(), buf->Size(), pak->key.c_str(), pak->key.length());
	if(file.compressedSize != file.size)
		buf = buf->Decompress(file.size);
	return buf;
}

//=================================================================================================
void Resource::EnsureIsLoaded()
{
	if(state != ResourceState::Loaded)
		app::resMgr->LoadInstant(this);
}

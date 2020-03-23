#pragma once

//-----------------------------------------------------------------------------
struct ManagedResource
{
	virtual ~ManagedResource() {}
	virtual void OnReset() = 0;
	virtual void OnReload() = 0;
	virtual void OnRelease() = 0;
};

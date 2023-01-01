#pragma once

namespace CrashHandler
{
	void Register(cstring title, cstring version, cstring url, int minidumpLevel, delegate<void()> callback = nullptr);
	void AddFile(cstring path, cstring name);
	void DoCrash();
}

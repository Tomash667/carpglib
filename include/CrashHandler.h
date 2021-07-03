#pragma once

namespace CrashHandler
{
	void Register(cstring title, cstring version, cstring url, int minidumpLevel);
	void DoCrash();
}

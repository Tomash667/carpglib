#pragma once

int AppEntry(cstring cmdLine);

#ifndef _MINWINDEF_
struct HINSTANCE__ { int unused; };
typedef struct HINSTANCE__* HINSTANCE;
#endif

int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char* lpCmdLine, int nShowCmd)
{
	return AppEntry(lpCmdLine);
}

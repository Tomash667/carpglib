#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#ifndef STRICT
#	define STRICT
#endif

#include <Windows.h>
#ifdef INCLUDE_COMMON_DIALOGS
#include <commdlg.h>
#endif
#ifdef INCLUDE_DEVICE_API
#include <initguid.h>
#include <mmdeviceapi.h>
#endif
#ifdef INCLUDE_SHELLAPI
#include <Shellapi.h>
#include <Shlobj.h>
#endif

#undef CreateFont
#undef CreateDirectory
#undef DialogBox
#undef DrawText
#undef MoveFile
#undef far
#undef near
#undef small
#undef IN
#undef OUT
#undef ERROR
#undef TRANSPARENT

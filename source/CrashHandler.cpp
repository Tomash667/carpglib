#include "Pch.h"
#include "CrashHandler.h"

#include <CrashRpt.h>

static delegate<void()> callback;

//=================================================================================================
cstring ExceptionTypeToString(int exctype)
{
	switch(exctype)
	{
	case CR_SEH_EXCEPTION:
		return "SEH exception";
	case CR_CPP_TERMINATE_CALL:
		return "C++ terminate() call";
	case CR_CPP_UNEXPECTED_CALL:
		return "C++ unexpected() call";
	case CR_CPP_PURE_CALL:
		return "C++ pure virtual function call";
	case CR_CPP_NEW_OPERATOR_ERROR:
		return "C++ new operator fault";
	case CR_CPP_SECURITY_ERROR:
		return "Buffer overrun error";
	case CR_CPP_INVALID_PARAMETER:
		return "Invalid parameter exception";
	case CR_CPP_SIGABRT:
		return "C++ SIGABRT signal";
	case CR_CPP_SIGFPE:
		return "C++ SIGFPE signal";
	case CR_CPP_SIGILL:
		return "C++ SIGILL signal";
	case CR_CPP_SIGINT:
		return "C++ SIGINT signal";
	case CR_CPP_SIGSEGV:
		return "C++ SIGSEGV signal";
	case CR_CPP_SIGTERM:
		return "C++ SIGTERM signal";
	default:
		return Format("unknown(%d)", exctype);
	}
}

//=================================================================================================
cstring CodeToString(DWORD err)
{
	switch(err)
	{
	case EXCEPTION_ACCESS_VIOLATION:         return "Access violation";
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:    return "Array bounds exceeded";
	case EXCEPTION_BREAKPOINT:               return "Breakpoint was encountered";
	case EXCEPTION_DATATYPE_MISALIGNMENT:    return "Datatype misalignment";
	case EXCEPTION_FLT_DENORMAL_OPERAND:     return "Float: Denormal operand";
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:       return "Float: Divide by zero";
	case EXCEPTION_FLT_INEXACT_RESULT:       return "Float: Inexact result";
	case EXCEPTION_FLT_INVALID_OPERATION:    return "Float: Invalid operation";
	case EXCEPTION_FLT_OVERFLOW:             return "Float: Overflow";
	case EXCEPTION_FLT_STACK_CHECK:          return "Float: Stack check";
	case EXCEPTION_FLT_UNDERFLOW:            return "Float: Underflow";
	case EXCEPTION_ILLEGAL_INSTRUCTION:      return "Illegal instruction";
	case EXCEPTION_IN_PAGE_ERROR:            return "Page error";
	case EXCEPTION_INT_DIVIDE_BY_ZERO:       return "Integer: Divide by zero";
	case EXCEPTION_INT_OVERFLOW:             return "Integer: Overflow";
	case EXCEPTION_INVALID_DISPOSITION:      return "Invalid disposition";
	case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "Noncontinuable exception";
	case EXCEPTION_PRIV_INSTRUCTION:         return "Private Instruction";
	case EXCEPTION_SINGLE_STEP:              return "Single step";
	case EXCEPTION_STACK_OVERFLOW:           return "Stack overflow";
	default:								 return "Unknown exception code";
	}
}

//=================================================================================================
int WINAPI OnCrash(CR_CRASH_CALLBACK_INFO* crashInfo)
{
	cstring type = ExceptionTypeToString(crashInfo->pExceptionInfo->exctype);
	if(crashInfo->pExceptionInfo->pexcptrs && crashInfo->pExceptionInfo->pexcptrs->ExceptionRecord)
	{
		PEXCEPTION_RECORD ptr = crashInfo->pExceptionInfo->pexcptrs->ExceptionRecord;
		Error("Engine: Unhandled exception caught!\nType: %s\nCode: 0x%x (%s)\nFlags: %d\nAddress: 0x%p\n\nPlease report this error.",
			type, ptr->ExceptionCode, CodeToString(ptr->ExceptionCode), ptr->ExceptionFlags, ptr->ExceptionAddress);
	}
	else
		Error("Engine: Unhandled exception caught!\nType: %s", type);

	TextLogger* textLogger = TextLogger::GetInstance();
	if(textLogger)
		textLogger->Flush();

	if(callback)
		callback();

	return CR_CB_DODEFAULT;
}

//=================================================================================================
void CrashHandler::Register(cstring title, cstring version, cstring url, int minidumpLevel, delegate<void()> callback)
{
	if(IsDebuggerPresent())
		return;

	CR_INSTALL_INFO info = { 0 };
	info.cb = sizeof(CR_INSTALL_INFO);
	info.pszAppName = title;
	info.pszAppVersion = version;
	info.pszUrl = url;
	info.uPriorities[CR_HTTP] = 1;
	info.uPriorities[CR_SMTP] = CR_NEGATIVE_PRIORITY;
	info.uPriorities[CR_SMAPI] = CR_NEGATIVE_PRIORITY;
	info.dwFlags = CR_INST_ALL_POSSIBLE_HANDLERS | CR_INST_SHOW_ADDITIONAL_INFO_FIELDS | CR_INST_NO_EMAIL_VALIDATION;
	switch(minidumpLevel)
	{
	case 0:
		info.dwFlags |= CR_INST_NO_MINIDUMP;
		break;
	default:
	case 1:
		info.uMiniDumpType = MiniDumpNormal;
		break;
	case 2:
		info.uMiniDumpType = MiniDumpWithDataSegs;
		break;
	case 3:
		info.uMiniDumpType = (MINIDUMP_TYPE)(MiniDumpWithDataSegs | MiniDumpWithFullMemory);
		break;
	}

	int r = crInstall(&info);
	assert(r == 0);

	::callback = callback;
	r = crSetCrashCallback(OnCrash, nullptr);
	assert(r == 0);

	TextLogger* textLogger = TextLogger::GetInstance();
	if(textLogger)
	{
		r = crAddFile2(textLogger->GetPath().c_str(), nullptr, "Log file", CR_AF_MAKE_FILE_COPY | CR_AF_MISSING_FILE_OK | CR_AF_ALLOW_DELETE);
		assert(r == 0);
	}

	r = crAddScreenshot2(CR_AS_MAIN_WINDOW | CR_AS_PROCESS_WINDOWS | CR_AS_USE_JPEG_FORMAT | CR_AS_ALLOW_DELETE, 50);
	assert(r == 0);
}

//=================================================================================================
void CrashHandler::AddFile(cstring path, cstring name)
{
	int r = crAddFile2(path, nullptr, name, CR_AF_MAKE_FILE_COPY | CR_AF_MISSING_FILE_OK | CR_AF_ALLOW_DELETE);
	assert(r == 0);
}

//=================================================================================================
void CrashHandler::DoCrash()
{
	Info("Crash test...");
	int* z = nullptr;
#pragma warning(push)
#pragma warning(suppress: 6011)
	*z = 13;
#pragma warning(pop)
}

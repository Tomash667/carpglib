#include "Pch.h"
#include "File.h"
#include <Windows.h>

//-----------------------------------------------------------------------------
Logger* Logger::global;
static std::set<int> onceId;
const cstring Logger::levelNames[4] = {
	"INFO ",
	"WARN ",
	"ERROR",
	"FATAL"
};

void Logger::Log(Level level, cstring msg)
{
	assert(msg);
	time_t t = time(0);
	tm tm;
	localtime_s(&tm, &t);
	Log(level, msg, tm);
}

void Logger::SetInstance(Logger* logger)
{
	assert(logger);
	if(logger == global)
		return;
	if(global)
		delete global;
	global = logger;
}

//-----------------------------------------------------------------------------
ConsoleLogger::ConsoleLogger()
{
	AllocConsole();
	FILE* file;
	freopen_s(&file, "CONIN$", "r", stdin);
	freopen_s(&file, "CONOUT$", "w", stdout);
	freopen_s(&file, "CONOUT$", "w", stderr);
}

ConsoleLogger::~ConsoleLogger()
{
	printf("*** End of log.");
}

void ConsoleLogger::Move(const Int2& consolePos, const Int2& consoleSize)
{
	HWND con = GetConsoleWindow();
	Rect rect;
	GetWindowRect(con, (RECT*)&rect);
	Int2 pos = rect.LeftTop();
	Int2 size = rect.Size();
	if(consolePos.x != -1)
		pos.x = consolePos.x;
	if(consolePos.y != -1)
		pos.y = consolePos.y;
	if(consoleSize.x != -1)
		size.x = consoleSize.x;
	if(consoleSize.y != -1)
		size.y = consoleSize.y;
	SetWindowPos(con, 0, pos.x, pos.y, size.x, size.y, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
}

void ConsoleLogger::Log(Level level, cstring text, const tm& time)
{
	printf("%02d:%02d:%02d %s - %s\n", time.tm_hour, time.tm_min, time.tm_sec, levelNames[level], text);
	fflush(stdout);
}

//-----------------------------------------------------------------------------
TextLogger::TextLogger(cstring filename) : path(filename)
{
	assert(filename);
	writer = new TextWriter(filename);
}

TextLogger::~TextLogger()
{
	*writer << "*** End of log.";
	delete writer;
}

void TextLogger::Log(Level level, cstring text, const tm& time)
{
	*writer << Format("%02d:%02d:%02d %s - %s\n", time.tm_hour, time.tm_min, time.tm_sec, levelNames[level], text);
}

void TextLogger::Flush()
{
	writer->Flush();
}

TextLogger* TextLogger::GetInstance()
{
	Logger* log = Logger::GetInstance();
	TextLogger* textLog = dynamic_cast<TextLogger*>(log);
	if(textLog)
		return textLog;
	MultiLogger* mlog = dynamic_cast<MultiLogger*>(log);
	if(mlog)
	{
		for(Logger* log : mlog->loggers)
		{
			textLog = dynamic_cast<TextLogger*>(log);
			if(textLog)
				return textLog;
		}
	}
	return nullptr;
}

//-----------------------------------------------------------------------------
MultiLogger::MultiLogger(std::initializer_list<Logger*> const& loggers)
{
	for(Logger* logger : loggers)
		this->loggers.push_back(logger);
}

MultiLogger::~MultiLogger()
{
	DeleteElements(loggers);
}

void MultiLogger::Log(Level level, cstring text, const tm& time)
{
	for(Logger* logger : loggers)
		logger->Log(level, text, time);
}

void MultiLogger::Flush()
{
	for(Logger* logger : loggers)
		logger->Flush();
}

//-----------------------------------------------------------------------------
void PreLogger::Apply(Logger* logger)
{
	assert(logger);

	for(Prelog* log : prelogs)
		logger->Log(log->level, log->str.c_str(), log->time);

	if(flush)
		logger->Flush();
	DeleteElements(prelogs);
}

void PreLogger::Clear()
{
	DeleteElements(prelogs);
}

void PreLogger::Log(Level level, cstring text, const tm& time)
{
	Prelog* p = new Prelog;
	p->str = text;
	p->level = level;
	p->time = time;

	prelogs.push_back(p);
}

void PreLogger::Flush()
{
	flush = true;
}

//-----------------------------------------------------------------------------
void WarnOnce(int id, cstring msg)
{
	if(onceId.find(id) == onceId.end())
	{
		onceId.insert(id);
		Warn(msg);
	}
}

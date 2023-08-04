#pragma once

//-----------------------------------------------------------------------------
// Base logger, don't log anything
class Logger
{
public:
	enum Level
	{
		L_INFO,
		L_WARN,
		L_ERROR,
		L_FATAL
	};

	virtual ~Logger() {}
	virtual void Log(Level level, cstring text, const tm& time) {}
	virtual void Flush() {}
	void Log(Level level, cstring text);

	void Info(cstring msg)
	{
		Log(L_INFO, msg);
	}
	template<typename... Args>
	void Info(cstring msg, const Args&... args)
	{
		Log(L_INFO, Format(msg, args...));
	}
	void Warn(cstring msg)
	{
		Log(L_WARN, msg);
	}
	template<typename... Args>
	void Warn(cstring msg, const Args&... args)
	{
		Log(L_WARN, Format(msg, args...));
	}
	void Error(cstring msg)
	{
		Log(L_ERROR, msg);
	}
	template<typename... Args>
	void Error(cstring msg, const Args&... args)
	{
		Log(L_ERROR, Format(msg, args...));
	}
	void Fatal(cstring msg)
	{
		Log(L_FATAL, msg);
	}
	template<typename... Args>
	void Fatal(cstring msg, const Args&... args)
	{
		Log(L_FATAL, Format(msg, args...));
	}

	static Logger* GetInstance() { return global; }
	static void SetInstance(Logger* logger);

protected:
	static Logger* global;
	static const cstring levelNames[4];
};

//-----------------------------------------------------------------------------
// Logger to system console window
class ConsoleLogger : public Logger
{
public:
	ConsoleLogger();
	~ConsoleLogger();

	void Move(const Int2& pos, const Int2& size);

protected:
	void Log(Level level, cstring text, const tm& time) override;
};

//-----------------------------------------------------------------------------
// Logger to text file
class TextLogger : public Logger
{
	TextWriter* writer;
	string path;

public:
	explicit TextLogger(cstring filename);
	~TextLogger();
	void Flush() override;
	const string& GetPath() const
	{
		return path;
	}

	static TextLogger* GetInstance();

protected:
	void Log(Level level, cstring text, const tm& time) override;
};

//-----------------------------------------------------------------------------
// Logger to multiple loggers
class MultiLogger : public Logger
{
public:
	vector<Logger*> loggers;

	MultiLogger() {}
	MultiLogger(std::initializer_list<Logger*> const& loggers);
	~MultiLogger();
	void Flush() override;

protected:
	void Log(Level level, cstring text, const tm& time) override;
};

//-----------------------------------------------------------------------------
// Pre logger, used before deciding where log to
class PreLogger : public Logger
{
	struct Prelog
	{
		string str;
		Level level;
		tm time;
	};

	vector<Prelog*> prelogs;
	bool flush;

public:
	PreLogger() : flush(false) {}
	~PreLogger();
	void Apply(Logger* logger);
	void Flush() override;

protected:
	void Log(Level level, cstring text, const tm& time) override;
};

//-----------------------------------------------------------------------------
// Helper global functions
inline void Info(cstring msg)
{
	Logger::GetInstance()->Log(Logger::L_INFO, msg);
}
template<typename... Args>
inline void Info(cstring msg, const Args&... args)
{
	Logger::GetInstance()->Log(Logger::L_INFO, Format(msg, args...));
}

inline void Warn(cstring msg)
{
	Logger::GetInstance()->Log(Logger::L_WARN, msg);
}
template<typename... Args>
inline void Warn(cstring msg, const Args&... args)
{
	Logger::GetInstance()->Log(Logger::L_WARN, Format(msg, args...));
}

void WarnOnce(int id, cstring msg);
template<typename... Args>
inline void WarnOnce(int id, cstring msg, const Args&... args)
{
	WarnOnce(id, Format(msg, args...));
}

inline void Error(cstring msg)
{
	Logger::GetInstance()->Log(Logger::L_ERROR, msg);
}
template<typename... Args>
inline void Error(cstring msg, const Args&... args)
{
	Logger::GetInstance()->Log(Logger::L_ERROR, Format(msg, args...));
}

inline void Fatal(cstring msg)
{
	Logger::GetInstance()->Log(Logger::L_FATAL, msg);
}
template<typename... Args>
inline void Fatal(cstring msg, const Args&... args)
{
	Logger::GetInstance()->Log(Logger::L_FATAL, Format(msg, args...));
}

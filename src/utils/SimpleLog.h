/* Copyright 2006-2011 the SumatraPDF project authors (see AUTHORS file).
   License: Simplified BSD (see COPYING) */

#ifndef SimpleLog_h
#define SimpleLog_h

#include "Vec.h"

namespace Log {

class Logger {
public:
    virtual ~Logger() { }
    virtual void Log(TCHAR *s) = 0;

    void LogFmt(TCHAR *fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        ScopedMem<TCHAR> s(Str::FmtV(fmt, args));
        Log(s);
        va_end(args);
    }

    void LogAndFree(TCHAR *s)
    {
        Log(s);
        free(s);
    }
};

class FileLogger : public Logger {
    HANDLE fh;

public:
    FileLogger(const TCHAR *fileName)
    {
        fh = CreateFile(fileName, FILE_APPEND_DATA, FILE_SHARE_READ, NULL,
                        OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    }
    FileLogger(HANDLE fh) : fh(fh) { }
    virtual ~FileLogger() { CloseHandle(fh); }

    virtual void Log(TCHAR *s)
    {
        ScopedMem<char> utf8s(Str::Conv::ToUtf8(s));
        if (utf8s && INVALID_HANDLE_VALUE != fh) {
            DWORD len;
            BOOL ok = WriteFile(fh, utf8s.Get(), Str::Len(utf8s), &len, NULL);
            if (ok)
                WriteFile(fh, "\r\n", 2, &len, NULL);
        }
    }
};

class MemoryLogger : public Logger {
    Str::Str<TCHAR> log;

public:
    virtual void Log(TCHAR *s)
    {
        if (s) {
            log.Append(s);
            log.Append(_T("\r\n"));
        }
    }

    // caller MUST NOT free the result
    // (Str::Dup data, if the logger is in use)
    TCHAR *GetData() { return log.LendData(); }
};

class DebugLogger : public Logger {
public:
    virtual void Log(TCHAR *s)
    {
        if (s) {
            OutputDebugString(s);
            OutputDebugString(_T("\r\n"));
        }
    }
};

class StderrLogger : public Logger {
public:
    virtual void Log(TCHAR *s)
    {
        if (s)
            _ftprintf(stderr, _T("%s\n"), s);
    }
};

// allows to log into several logs at the same time (thread safe)
class MultiLogger : public Logger {
    Vec<Logger *>    loggers;
    CRITICAL_SECTION cs;

public:
    MultiLogger() { InitializeCriticalSection(&cs); }
    ~MultiLogger()
    {
        EnterCriticalSection(&cs);
        DeleteVecMembers(loggers);
        LeaveCriticalSection(&cs);
        DeleteCriticalSection(&cs);
    }

    virtual void Log(TCHAR *s)
    {
        ScopedCritSec scope(&cs);
        for (size_t i = 0; i < loggers.Count(); i++)
            loggers[i]->Log(s);
    }

    void AddLogger(Logger *logger)
    {
        ScopedCritSec scope(&cs);
        loggers.Append(logger);
    }
    void RemoveLogger(Logger *logger)
    {
        ScopedCritSec scope(&cs);
        loggers.Remove(logger);
    }
    size_t CountLoggers()
    {
        ScopedCritSec scope(&cs);
        return loggers.Count();
    }
};

void Initialize();
void Destroy();

void AddLogger(Logger *);
void RemoveLogger(Logger *);

void Log(TCHAR *s);
void LogFmt(TCHAR *fmt, ...);
void LogAndFree(TCHAR *s);

} // namespace Log

#endif
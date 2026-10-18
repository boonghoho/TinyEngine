#include "log.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdarg>
#include <cstdio>
#include <mutex>
#include <share.h>
#include <string>
#include <string_view>

namespace tiny
{

namespace
{

struct LogState
{
    std::mutex Mutex;
    std::FILE* File = nullptr;

    ~LogState()
    {
        if (File)
        {
            std::fclose(File);
        }
    }
};

LogState& GetLogState()
{
    static LogState State;
    return State;
}

const char* GetLevelName(LogLevel Level)
{
    switch (Level)
    {
    case LogLevel::Info:
        return "Info";
    case LogLevel::Warning:
        return "Warning";
    case LogLevel::Error:
        return "Error";
    }

    return "Unknown";
}

const char* GetFileName(const char* Path)
{
    const char* Name = Path;
    for (const char* Character = Path; *Character; ++Character)
    {
        if (*Character == '/' || *Character == '\\')
        {
            Name = Character + 1;
        }
    }
    return Name;
}

void WriteToConsoleAndDebugger(const std::string& Text)
{
    std::fwrite(Text.data(), 1, Text.size(), stderr);
    std::fflush(stderr);

    const int WideLength = MultiByteToWideChar(CP_UTF8, 0, Text.c_str(), -1, nullptr, 0);
    if (WideLength > 0)
    {
        std::wstring WideText(static_cast<std::size_t>(WideLength), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, Text.c_str(), -1, WideText.data(), WideLength);
        OutputDebugStringW(WideText.c_str());
    }
}

}

bool InitializeLogging(const std::filesystem::path& FilePath)
{
    LogState& State = GetLogState();
    const std::lock_guard Lock(State.Mutex);

    if (State.File)
    {
        std::fclose(State.File);
        State.File = nullptr;
    }

    // 실행 중에도 로그를 읽을 수 있도록 읽기 공유를 허용한다.
    State.File = _wfsopen(FilePath.c_str(), L"wb", _SH_DENYWR);
    return State.File != nullptr;
}

void ShutdownLogging()
{
    LogState& State = GetLogState();
    const std::lock_guard Lock(State.Mutex);

    if (State.File)
    {
        std::fclose(State.File);
        State.File = nullptr;
    }
}

std::string FormatLogMessage(const char* Format, va_list Arguments)
{
    if (!Format)
    {
        return {};
    }

    va_list Copy;
    va_copy(Copy, Arguments);
    const int MessageLength = std::vsnprintf(nullptr, 0, Format, Copy);
    va_end(Copy);

    if (MessageLength < 0)
    {
        return "Failed to format message";
    }

    std::string Message(static_cast<std::size_t>(MessageLength), '\0');
    va_copy(Copy, Arguments);
    std::vsnprintf(Message.data(), Message.size() + 1, Format, Copy);
    va_end(Copy);
    return Message;
}

void WriteLog(
    LogLevel Level,
    const char* Category,
    const char* SourceFile,
    int SourceLine,
    const char* Format,
    ...)
{
    va_list Arguments;
    va_start(Arguments, Format);
    std::string Message = FormatLogMessage(Format, Arguments);
    va_end(Arguments);

    while (!Message.empty() && (Message.back() == '\n' || Message.back() == '\r'))
    {
        Message.pop_back();
    }

    LogState& State = GetLogState();
    const std::lock_guard Lock(State.Mutex);

    SYSTEMTIME Time = {};
    GetLocalTime(&Time);
    char Timestamp[32];
    std::snprintf(
        Timestamp, sizeof(Timestamp), "%04d-%02d-%02d %02d:%02d:%02d.%03d", Time.wYear, Time.wMonth, Time.wDay, Time.wHour, Time.wMinute, Time.wSecond, Time.wMilliseconds);

    const std::string Prefix = std::string("[") + Timestamp + "][" + GetLevelName(Level) + "][" + Category + "] ";
    const std::string Suffix = std::string(" (") + GetFileName(SourceFile) + ":" + std::to_string(SourceLine) + ")\n";
    std::string Record;
    std::size_t Begin = 0;

    // 빈 메시지도 한 줄 기록한다.
    while (Begin <= Message.size())
    {
        const std::size_t End = Message.find('\n', Begin);
        std::string_view Line(Message.data() + Begin, (End == std::string::npos ? Message.size() : End) - Begin);
        if (!Line.empty() && Line.back() == '\r')
        {
            Line.remove_suffix(1);
        }
        Record += Prefix;
        Record.append(Line);
        Record += Suffix;
        if (End == std::string::npos)
        {
            break;
        }
        Begin = End + 1;
    }

    WriteToConsoleAndDebugger(Record);
    if (State.File)
    {
        const bool bWriteSucceeded = std::fwrite(Record.data(), 1, Record.size(), State.File) == Record.size();
        const bool bFlushSucceeded = std::fflush(State.File) == 0;
        if (!bWriteSucceeded || !bFlushSucceeded)
        {
            std::fclose(State.File);
            State.File = nullptr;
            WriteToConsoleAndDebugger("[Warning][Log] File output failed; continuing with console/debugger output.\n");
        }
    }
}

}

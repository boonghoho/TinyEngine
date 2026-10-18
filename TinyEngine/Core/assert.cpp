#include "assert.h"
#include "log.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <intrin.h>

#include <cstdarg>
#include <cstdlib>
#include <string>

namespace tiny
{

[[noreturn]] void HandleAssertFailure(
    const char* Condition,
    const char* SourceFile,
    int SourceLine,
    const char* Format,
    ...)
{
    va_list Arguments;
    va_start(Arguments, Format);
    const std::string Message = FormatLogMessage(Format, Arguments);
    va_end(Arguments);

    WriteLog(
        LogLevel::Error, "Assert", SourceFile, SourceLine,
        "Assertion failed: %s%s%s", Condition, Message.empty() ? "" : "\n", Message.c_str());

    // WriteLog가 파일 기록과 mutex 해제를 마친 뒤 중단한다.
    if (IsDebuggerPresent())
    {
        __debugbreak();
    }

    std::abort();
}

}

#pragma once

#include <filesystem>

namespace tiny
{

enum class LogLevel
{
    Info,
    Warning,
    Error,
};

// UTF-8 로그 파일을 연다. 기존 파일 내용은 지워진다.
bool InitializeLogging(const std::filesystem::path& FilePath);

// 로그를 쓰는 스레드가 모두 종료된 뒤 호출한다.
void ShutdownLogging();

// printf 형식으로 동기 기록하며, 전달받은 포인터는 저장하지 않는다.
void WriteLog(
    LogLevel Level,
    const char* Category,
    const char* SourceFile,
    int SourceLine,
    const char* Format,
    ...);

}

#define TE_LOG_INFO(Category, ...) \
    ::tiny::WriteLog(::tiny::LogLevel::Info, Category, __FILE__, __LINE__, __VA_ARGS__)
#define TE_LOG_WARNING(Category, ...) \
    ::tiny::WriteLog(::tiny::LogLevel::Warning, Category, __FILE__, __LINE__, __VA_ARGS__)
#define TE_LOG_ERROR(Category, ...) \
    ::tiny::WriteLog(::tiny::LogLevel::Error, Category, __FILE__, __LINE__, __VA_ARGS__)

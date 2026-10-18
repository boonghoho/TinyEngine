#pragma once

namespace tiny
{

[[noreturn]] void HandleAssertFailure(
    const char* Condition,
    const char* SourceFile,
    int SourceLine,
    const char* Format = nullptr,
    ...);

}

// Release에서는 인자도 실행되지 않으므로, 상태를 바꾸는 작업은 조건 밖에서 수행한다.
#if defined(NDEBUG)
#define TE_ASSERT(Condition, ...) static_cast<void>(0)
#else
#define TE_ASSERT(Condition, ...) \
    ((Condition) ? static_cast<void>(0) : \
        ::tiny::HandleAssertFailure(#Condition, __FILE__, __LINE__ __VA_OPT__(,) __VA_ARGS__))
#endif

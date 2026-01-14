#include "stdafx.h"

#ifdef _DEBUG
static void DbgPrint(const TCHAR* fmt, ...)
{
    static TCHAR buffer[256];

    va_list args;
    va_start(args, fmt);
    _vstprintf_s(buffer, sizeof(buffer) / sizeof(TCHAR), fmt, args);
    va_end(args);

    OutputDebugString(buffer);
}
#else
inline static void DbgPrint(const TCHAR* fmt, ...) {}

namespace std
{
    struct nothrow_t {};
    extern nothrow_t constexpr nothrow = {};
    using size_t = decltype(sizeof(0));
}

// Required by `Microsoft::WRL::Details::MakeAllocator`
void* operator new(std::size_t size, const std::nothrow_t&) noexcept { return malloc(size); }

// Required by `Microsoft::WRL::Details::DelegateArgTraits`
void operator delete(void* ptr, std::size_t) noexcept { free(ptr); }
void operator delete[](void* ptr, std::size_t) noexcept { free(ptr); }
#endif
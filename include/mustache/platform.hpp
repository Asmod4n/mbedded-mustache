// The attributes that ask the compiler to inline, in the form of each compiler.
#pragma once

#if defined(_MSC_VER) && !defined(__clang__)
#define MUSTACHE_ALWAYS_INLINE [[msvc::forceinline]]
#define MUSTACHE_NEVER_INLINE [[msvc::noinline]]
#define MUSTACHE_FLATTEN [[msvc::flatten]]
#else
#define MUSTACHE_ALWAYS_INLINE [[gnu::always_inline]]
#define MUSTACHE_NEVER_INLINE [[gnu::noinline]]
#define MUSTACHE_FLATTEN [[gnu::flatten]]
#endif

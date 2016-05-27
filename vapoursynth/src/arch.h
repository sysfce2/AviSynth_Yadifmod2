/*
arch.h: Copyright (C) 2016  Oka Motofumi

Author: Oka Motofumi (chikuzen.mo at gmail dot com)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with the author; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/


#ifndef ARCHITECTURE_H
#define ARCHITECTURE_H

#if defined(_M_IX86) || defined(_M_AMD64) || defined(__i686) || defined(__x86_64)
    #define INTEL_X86_CPU
#if !defined(__GNUC__)
    #define __SSE4_1__
    #define __SSSE3__
    #define __SSE2__
#endif // __GNUC__

#endif // X86


#if defined(__AVX__)
    #if defined(__GNUC__)
        #include <x86intrin.h>
    #else
        #include <immintrin.h>
    #endif
#elif defined(__SSE4_1__)
    #include <smmintrin.h>
#elif defined(__SSSE3__)
    #include <tmmintrin.h>
#elif defined(__SSE2__)
    #include <emmintrin.h>
#endif


#if defined(__GNUC__)
    #define F_INLINE inline __attribute__((always_inline))
#else
    #define F_INLINE __forceinline
#endif


enum arch_t {
    NO_SIMD,
    USE_SSE2,
    USE_SSSE3,
    USE_SSE41,
    USE_AVX,
    USE_AVX2,
};


#if defined(INTEL_X86_CPU)
    extern bool has_sse2();
    extern bool has_ssse3();
    extern bool has_sse41();
    extern bool has_avx();
    extern bool has_avx2();
#endif


static inline arch_t get_arch(int opt)
{
#if !defined(__SSE2__)
    return NO_SIMD
#else
    if (opt == 0 || !has_sse2()) {
        return NO_SIMD;
    }
#if !defined(__SSSE3__)
    return USE_SSE2;
#else
    if (opt == 1 || !has_sse41()) {
        return USE_SSE2;
    }
#if !defined(__SSE4_1__)
    return USE_SSSE3;
#else
    if (opt == 2 || !has_sse41()) {
        return USE_SSSE3;
    }
#if !defined(__AVX__)
    return USE_SSE41;
#else
    if (opt == 3 || !has_avx()) {
        return USE_SSE41;
    }
#if !defined(__AVX2__)
    return USE_AVX;
#else
    if (opt == 4 || !has_avx2()) {
        return USE_AVX;
    }
    return USE_AVX2;
#endif // __AVX2__
#endif // __AVX__
#endif // __SSE4_1__
#endif // __SSSE3__
#endif // __SSE2__
}

#endif //ARCHITECTURE_H

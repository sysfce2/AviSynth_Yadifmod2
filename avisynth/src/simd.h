/*
    simd.h

    This file is a part of yadifmod2.

    Copyright (C) 2016 OKA Motofumi

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


#ifndef YADIF_MOD_2_SIMD_H
#define YADIF_MOD_2_SIMD_H

#if defined (__AVX2__)
    #include <immintrin.h>
#else
    #include <tmmintrin.h>
#endif
#include "common.h"


#define SFINLINE static __forceinline



template <typename T>
T zero();

template <typename T>
T load_half(const uint8_t* p);

template <typename T>
void store_half(uint8_t * p, const T & x) {}


template <>
SFINLINE __m128i zero<__m128i>()
{
    return _mm_setzero_si128();
}

template <>
SFINLINE __m128i load_half<__m128i>(const uint8_t* p)
{
    __m128i t = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(p));
    return _mm_unpacklo_epi8(t, zero<__m128i>());
}

template <>
SFINLINE void store_half<__m128i>(uint8_t* p, const __m128i& x)
{
    _mm_storel_epi64(reinterpret_cast<__m128i*>(p), _mm_packus_epi16(x, x));
}

SFINLINE __m128i and_reg(const __m128i& x, const __m128i& y)
{
    return _mm_and_si128(x, y);
}

SFINLINE __m128i or_reg(const __m128i& x, const __m128i& y)
{
    return _mm_or_si128(x, y);
}

SFINLINE __m128i andnot_reg(const __m128i& x, const __m128i& y)
{
    return _mm_andnot_si128(x, y);
}

SFINLINE __m128i xor_reg(const __m128i& x, const __m128i& y)
{
    return _mm_xor_si128(x, y);
}

SFINLINE __m128i max_i16(const __m128i& x, const __m128i& y)
{
    return _mm_max_epi16(x, y);
}

SFINLINE __m128i min_i16(const __m128i& x, const __m128i& y)
{
    return _mm_min_epi16(x, y);
}

SFINLINE __m128i sub_i16(const __m128i& x, const __m128i& y)
{
    return _mm_sub_epi16(x, y);
}

SFINLINE __m128i add_i16(const __m128i& x, const __m128i& y)
{
    return _mm_add_epi16(x, y);
}

SFINLINE __m128i div2_i16(const __m128i& x)
{
    return  _mm_srli_epi16(x, 1);
}

SFINLINE __m128i cmpeq_i16(const __m128i& x, const __m128i& y)
{
    return _mm_cmpeq_epi16(x, y);
}


SFINLINE __m128i cmpgt_i16(const __m128i& x, const __m128i& y)
{
    return _mm_cmpgt_epi16(x, y);
}

template <typename T, arch_t A>
SFINLINE T abs_diff_i16(const T& x, const T& y)
{
    return max_i16(sub_i16(x, y), sub_i16(y, x)); 
}

template <>
SFINLINE __m128i
abs_diff_i16<__m128i, USE_SSSE3>(const __m128i& x, const __m128i& y)
{
    return _mm_abs_epi16(sub_i16(x, y));
}

SFINLINE __m128i blendv(const __m128i& x, const __m128i& y, const __m128i& m)
{
    return or_reg(and_reg(m, y), andnot_reg(m, x));
}


#if defined(__AVX2__)

template <>
SFINLINE __m256i zero<__m256i>()
{
    return _mm256_setzero_si256();
}

template <>
SFINLINE __m256i load_half<__m256i>(const uint8_t* p)
{
    __m128i t = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));
    return _mm256_cvtepu8_epi16(t);
}

template <>
SFINLINE void store_half<__m256i>(uint8_t* p, const __m256i& x)
{
    __m256i t = _mm256_packus_epi16(x, _mm256_permute2x128_si256(x, x, 0x01));
    _mm_stream_si128(reinterpret_cast<__m128i*>(p),
        _mm256_extracti128_si256(t, 0));
}

SFINLINE __m256i and_reg(const __m256i& x, const __m256i& y)
{
    return _mm256_and_si256(x, y);
}

SFINLINE __m256i or_reg(const __m256i& x, const __m256i& y)
{
    return _mm256_or_si256(x, y);
}

SFINLINE __m256i andnot_reg(const __m256i& x, const __m256i& y)
{
    return _mm256_andnot_si256(x, y);
}

SFINLINE __m256i xor_reg(const __m256i& x, const __m256i& y)
{
    return _mm256_xor_si256(x, y);
}

SFINLINE __m256i max_i16(const __m256i& x, const __m256i& y)
{
    return _mm256_max_epi16(x, y);
}

SFINLINE __m256i min_i16(const __m256i& x, const __m256i& y)
{
    return _mm256_min_epi16(x, y);
}

SFINLINE __m256i sub_i16(const __m256i& x, const __m256i& y)
{
    return _mm256_sub_epi16(x, y);
}

SFINLINE __m256i add_i16(const __m256i& x, const __m256i& y)
{
    return _mm256_add_epi16(x, y);
}

SFINLINE __m256i div2_i16(const __m256i& x)
{
    return  _mm256_srli_epi16(x, 1);
}

template <>
SFINLINE __m256i
abs_diff_i16<__m256i, USE_AVX2>(const __m256i& x, const __m256i& y)
{
    return _mm256_abs_epi16(sub_i16(x, y));
}

SFINLINE __m256i cmpeq_i16(const __m256i& x, const __m256i& y)
{
    return _mm256_cmpeq_epi16(x, y);
}

SFINLINE __m256i cmpgt_i16(const __m256i& x, const __m256i& y)
{
    return _mm256_cmpgt_epi16(x, y);
}

SFINLINE __m256i blendv(const __m256i& x, const __m256i& y, const __m256i& m)
{
    return _mm256_blendv_epi8(x, y, m);
}

#endif  //__AVX2__

template <typename T>
SFINLINE T min3(const T& x, const T& y, const T& z)
{
    return min_i16(min_i16(x, y), z);
}

template <typename T>
SFINLINE T max3(const T& x, const T& y, const T& z)
{
    return max_i16(max_i16(x, y), z);
}

template <typename T>
SFINLINE T clamp_i16(const T& x, const T& min_, const T& max_)
{
    return max_i16(min_i16(x, max_), min_);
}

template <typename T>
SFINLINE T minus_i16(const T& x)
{
    return sub_i16(zero<T>(), x);
}

template <typename T>
SFINLINE T avg_i16(const T& x, const T& y)
{
    return div2_i16(add_i16(x, y));
}

#endif


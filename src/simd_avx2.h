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


#ifndef YADIFMOD2_SIMD_AVX2_H
#define YADIFMOD2_SIMD_AVX2_H

#include <immintrin.h>

#include "common.h"

#if !defined(__AVX2__)
#error "AVX2 option needed"
#endif


#ifndef __GNUC__
#define F_INLINE __forceinline
#else
#define F_INLINE __attribute__((always_inline)) inline
#endif


namespace simd {
    //-------------------------LOAD------------------------------------------------

    template <typename V, typename T, arch_t A>
    static F_INLINE V load(const uint8_t* ptr);

    template <>
    F_INLINE __m256i load<__m256i, int16_t, arch_t::USE_AVX2>(const uint8_t* ptr)
    {
        return _mm256_loadu_si256(reinterpret_cast<const __m256i*>(ptr));
    }

    template <>
    F_INLINE __m256i load<__m256i, uint8_t, arch_t::USE_AVX2>(const uint8_t* ptr)
    {
        __m128i t = _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr));
        return _mm256_cvtepu8_epi16(t);
    }

    template <>
    F_INLINE __m256i load<__m256i, uint16_t, arch_t::USE_AVX2>(const uint8_t* ptr)
    {
        __m128i t = _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr));
        return _mm256_cvtepu16_epi32(t);
    }


    //-------------------STORE---------------------------------------------------

    template <typename T>
    static F_INLINE void store(uint8_t* p, const __m256i& x)
    {
        _mm256_stream_si256(reinterpret_cast<__m256i*>(p), x);
    }

    template <>
    F_INLINE void store<uint8_t>(uint8_t* p, const __m256i& x)
    {
        __m256i t = _mm256_packus_epi16(x, _mm256_permute2x128_si256(x, x, 0x01));
        _mm_stream_si128(reinterpret_cast<__m128i*>(p), _mm256_extracti128_si256(t, 0));
    }

    template <>
    F_INLINE void store<uint16_t>(uint8_t* p, const __m256i& x)
    {
        __m256i t = _mm256_packus_epi32(x, _mm256_permute2x128_si256(x, x, 0x01));
        _mm_stream_si128(reinterpret_cast<__m128i*>(p), _mm256_extracti128_si256(t, 0));
    }


    //-------------------SETZERO-------------------------------------------------
    template <typename V>
    static F_INLINE V setzero();

    template <>
    F_INLINE __m256i setzero<__m256i>()
    {
        return _mm256_setzero_si256();
    }


    //-------------------ADD------------------------------------------------------

    template <typename T>
    static F_INLINE __m256i add(const __m256i& x, const __m256i& y)
    {
        return _mm256_add_epi16(x, y);
    }

    template <>
    F_INLINE __m256i add<uint16_t>(const __m256i& x, const __m256i& y)
    {
        return _mm256_add_epi32(x, y);
    }



    //-------------------SUB-----------------------------------------------------

    template <typename T>
    static F_INLINE __m256i sub(const __m256i& x, const __m256i& y)
    {
        return _mm256_sub_epi16(x, y);
    }

    template <>
    F_INLINE __m256i sub<uint16_t>(const __m256i& x, const __m256i& y)
    {
        return _mm256_sub_epi32(x, y);
    }


    //----------------------DIV2-------------------------------------------

    template <typename T>
    static F_INLINE __m256i div2(const __m256i& x)
    {
        return _mm256_srli_epi16(x, 1);
    }

    template <>
    F_INLINE __m256i div2<uint16_t>(const __m256i& x)
    {
        return _mm256_srli_epi32(x, 1);
    }


    //-------------------MIN----------------------------------------------------

    template <typename T>
    static F_INLINE __m256i min(const __m256i& x, const __m256i& y)
    {
        return _mm256_min_epi16(x, y);
    }

    template <>
    F_INLINE __m256i min<uint16_t>(const __m256i& x, const __m256i& y)
    {
        return _mm256_min_epi32(x, y);
    }



    //-----------------------MAX-----------------------------------------------

    template <typename T>
    static F_INLINE __m256i max(const __m256i& x, const __m256i& y)
    {
        return _mm256_max_epi16(x, y);
    }

    template <>
    F_INLINE __m256i max<uint16_t>(const __m256i& x, const __m256i& y)
    {
        return _mm256_max_epi32(x, y);
    }


    //------------------CMPGT-----------------------------------------

    template <typename T>
    static F_INLINE __m256i cmpgt(const __m256i& x, const __m256i& y)
    {
        return _mm256_cmpgt_epi16(x, y);
    }

    template <>
    F_INLINE __m256i cmpgt<uint16_t>(const __m256i& x, const __m256i& y)
    {
        return _mm256_cmpgt_epi32(x, y);
    }


    //------------------MINUS-------------------------------------------

    template <typename V, typename T>
    static F_INLINE V minus(const V& x)
    {
        return sub<T>(setzero<V>(), x);
    }


    //--------------------ABS_DIFF--------------------------------------------

    template <typename T, arch_t A>
    static F_INLINE __m256i abs_diff(const __m256i& x, const __m256i& y)
    {
        return _mm256_abs_epi16(_mm256_sub_epi16(x, y));
    }

    template <>
    F_INLINE __m256i
        abs_diff<uint16_t, arch_t::USE_AVX2>(const __m256i& x, const __m256i& y)
    {
        return _mm256_abs_epi32(_mm256_sub_epi32(x, y));
    }



    template <typename V, typename T>
    static F_INLINE V average(const V& x, const V& y)
    {
        return div2<T>(add<T>(x, y));
    }


    template <typename T>
    static F_INLINE __m256i adjust(const __m256i& x)
    {
        return add<T>(x, _mm256_cmpeq_epi32(x, x));
    }


    template <arch_t A>
    static F_INLINE __m256i
        blendv(const __m256i& x, const __m256i& y, const __m256i& m)
    {
        return _mm256_blendv_epi8(x, y, m);
    }
}

#endif //YADIFMOD2_SIMD_AVX2_H

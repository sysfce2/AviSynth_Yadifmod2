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


#ifndef YADIFMOD2_SIMD_SSE41_H
#define YADIFMOD2_SIMD_SSE41_H

#if defined (__AVX__)
#include <immintrin.h>
#else
#include <smmintrin.h>
#endif
#include "common.h"


#define F_INLINE __forceinline


namespace simd {
    //-------------------------LOAD------------------------------------------------
    template <typename V, typename T, arch_t A>
    static F_INLINE V load(const uint8_t* ptr);

    template <>
    F_INLINE __m128 load<__m128, float, arch_t::USE_SSE2>(const uint8_t* ptr)
    {
        return _mm_loadu_ps(reinterpret_cast<const float*>(ptr));
    }
    template <>
    F_INLINE __m128i load<__m128i, int16_t, arch_t::USE_SSE2>(const uint8_t* ptr)
    {
        return _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr));
    }
    template <>
    F_INLINE __m128i load<__m128i, uint8_t, arch_t::USE_SSE2>(const uint8_t* ptr)
    {
        __m128i t = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(ptr));
        return _mm_unpacklo_epi8(t, _mm_setzero_si128());
    }
    template <>
    F_INLINE __m128i load<__m128i, uint16_t, arch_t::USE_SSE2>(const uint8_t* ptr)
    {
        __m128i t = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(ptr));
        return _mm_unpacklo_epi16(t, _mm_setzero_si128());
    }

    template <>
    F_INLINE __m128i load<__m128i, int16_t, arch_t::USE_SSSE3>(const uint8_t* ptr)
    {
        return load<__m128i, int16_t, arch_t::USE_SSE2>(ptr);
    }
    template <>
    F_INLINE __m128i load<__m128i, uint8_t, arch_t::USE_SSSE3>(const uint8_t* ptr)
    {
        __m128i t = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(ptr));
        return _mm_unpacklo_epi8(t, _mm_setzero_si128());
    }
    template <>
    F_INLINE __m128i load<__m128i, uint16_t, arch_t::USE_SSSE3>(const uint8_t* ptr)
    {
        __m128i t = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(ptr));
        return _mm_unpacklo_epi16(t, _mm_setzero_si128());
    }



#if defined(__AVX__)
    template <>
    F_INLINE __m256 load<__m256, float, arch_t::USE_AVX>(const uint8_t* ptr)
    {
        return _mm256_loadu_ps(reinterpret_cast<const float*>(ptr));
    }

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

#endif //__AVX__

    //-------------------STORE---------------------------------------------------
    template <typename T>
    static F_INLINE void store(uint8_t* p, const __m128& x)
    {
        _mm_stream_ps(reinterpret_cast<float*>(p), x);
    }

    template <typename T>
    static F_INLINE void store(uint8_t* p, const __m128i& x)
    {
        _mm_stream_si128(reinterpret_cast<__m128i*>(p), x);
    }
    template <>
    F_INLINE void store<uint8_t>(uint8_t* p, const __m128i& x)
    {
        _mm_storel_epi64(reinterpret_cast<__m128i*>(p), _mm_packus_epi16(x, x));
    }
    template <>
    F_INLINE void store<uint16_t>(uint8_t* p, const __m128i& x)
    {
        _mm_storel_epi64(reinterpret_cast<__m128i*>(p), _mm_packus_epi32(x, x));
    }

#if defined(__AVX__)
    template <typename T>
    static F_INLINE void store(uint8_t* p, const __m256& x)
    {
        _mm256_stream_ps(reinterpret_cast<float*>(p), x);
    }

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
#endif // __AVX__


    //-------------------SETZERO-------------------------------------------------
    template <typename V>
    static F_INLINE V setzero();

    template <>
    F_INLINE __m128 setzero<__m128>()
    {
        return _mm_setzero_ps();
    }
    template <>
    F_INLINE __m128i setzero<__m128i>()
    {
        return _mm_setzero_si128();
    }
#if defined(__AVX__)
    template <>
    F_INLINE __m256 setzero<__m256>()
    {
        return _mm256_setzero_ps();
    }
    template <>
    F_INLINE __m256i setzero<__m256i>()
    {
        return _mm256_setzero_si256();
    }
#endif // __AVX__



    //-------------------ADD------------------------------------------------------
    template <typename T>
    static F_INLINE __m128 add(const __m128& x, const __m128& y)
    {
        return _mm_add_ps(x, y);
    }

    template <typename T>
    static F_INLINE __m128i add(const __m128i& x, const __m128i& y)
    {
        return _mm_add_epi16(x, y);
    }
    template <>
    F_INLINE __m128i add<uint16_t>(const __m128i& x, const __m128i& y)
    {
        return _mm_add_epi32(x, y);
    }

#if defined(__AVX__)
    template <typename T>
    static F_INLINE __m256 add(const __m256& x, const __m256& y)
    {
        return _mm256_add_ps(x, y);
    }

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
#endif // __AVX__



    //-------------------SUB-----------------------------------------------------
    template <typename T>
    static F_INLINE __m128 sub(const __m128& x, const __m128& y)
    {
        return _mm_sub_ps(x, y);
    }

    template <typename T>
    static F_INLINE __m128i sub(const __m128i& x, const __m128i& y)
    {
        return _mm_sub_epi16(x, y);
    }
    template <>
    F_INLINE __m128i sub<uint16_t>(const __m128i& x, const __m128i& y)
    {
        return _mm_sub_epi32(x, y);
    }

#if defined(__AVX__)
    template <typename T>
    static F_INLINE __m256 sub(const __m256& x, const __m256& y)
    {
        return _mm256_sub_ps(x, y);
    }

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
#endif // __AVX__



    //----------------------DIV2-------------------------------------------

    template <typename T>
    static F_INLINE __m128 div2(const __m128& x)
    {
        static __m128 coef = _mm_set1_ps(0.5f);
        return _mm_mul_ps(x, coef);
    }

    template <typename T>
    static F_INLINE __m128i div2(const __m128i& x)
    {
        return _mm_srli_epi16(x, 1);
    }
    template <>
    F_INLINE __m128i div2<uint16_t>(const __m128i& x)
    {
        return _mm_srli_epi32(x, 1);
    }

#if defined(__AVX__)
    template <typename T>
    static F_INLINE __m256 div2(const __m256& x)
    {
        static __m256 coef = _mm256_set1_ps(0.5f);
        return _mm256_mul_ps(x, coef);
    }

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
#endif // __AVX__



    //-------------------MIN----------------------------------------------------

    template <typename T>
    static F_INLINE __m128 min(const __m128& x, const __m128& y)
    {
        return _mm_min_ps(x, y);
    }

    template <typename T>
    static F_INLINE __m128i min(const __m128i& x, const __m128i& y)
    {
        return _mm_min_epi16(x, y);
    }
    template <>
    F_INLINE __m128i min<uint16_t>(const __m128i& x, const __m128i& y)
    {
        return _mm_min_epi32(x, y);
    }

#if defined(__AVX__)
    template<typename T>
    static F_INLINE __m256 min(const __m256& x, const __m256& y)
    {
        return _mm256_min_ps(x, y);
    }

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
#endif // __AVX__



    //-----------------------MAX-----------------------------------------------

    template <typename T>
    static F_INLINE __m128 max(const __m128& x, const __m128& y)
    {
        return _mm_max_ps(x, y);
    }

    template <typename T>
    static F_INLINE __m128i max(const __m128i& x, const __m128i& y)
    {
        return _mm_max_epi16(x, y);
    }
    template <>
    F_INLINE __m128i max<uint16_t>(const __m128i& x, const __m128i& y)
    {
        return _mm_max_epi32(x, y);
    }

#if defined(__AVX__)
    template<typename T>
    static F_INLINE __m256 max(const __m256& x, const __m256& y)
    {
        return _mm256_max_ps(x, y);
    }

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
#endif // __AVX__




    //------------------CMPGT-----------------------------------------
    template <typename T>
    static F_INLINE __m128 cmpgt(const __m128& x, const __m128& y)
    {
        return _mm_cmpgt_ps(x, y);
    }

    template <typename T>
    static F_INLINE __m128i cmpgt(const __m128i& x, const __m128i& y)
    {
        return _mm_cmpgt_epi16(x, y);
    }
    template <>
    F_INLINE __m128i cmpgt<uint16_t>(const __m128i& x, const __m128i& y)
    {
        return _mm_cmpgt_epi32(x, y);
    }

#if defined(__AVX__)
    template <typename T>
    static F_INLINE __m256 cmpgt(const __m256& x, const __m256& y)
    {
        return _mm256_cmp_ps(x, y, _CMP_GT_OS);
    }

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
#endif // __AVX__





    //------------------MINUS-------------------------------------------
    template <typename V, typename T>
    static F_INLINE V minus(const V& x)
    {
        return sub<T>(setzero<V>(), x);
    }


    //--------------------ABS_DIFF--------------------------------------------
    template <typename T, arch_t A>
    static F_INLINE __m128 abs_diff(const __m128& x, const __m128& y)
    {
        return _mm_max_ps(_mm_sub_ps(x, y), _mm_sub_ps(y, x));
    }

    template <typename T, arch_t A>
    static F_INLINE __m128i abs_diff(const __m128i& x, const __m128i& y)
    {
        return _mm_or_si128(_mm_subs_epu16(x, y), _mm_subs_epu16(y, x));
    }
    template <>
    F_INLINE __m128i abs_diff<uint16_t, arch_t::USE_SSE2>(const __m128i& x, const __m128i& y)
    {
        return _mm_max_epi32(_mm_sub_epi32(x, y), _mm_sub_epi32(y, x));
    }

    template <>
    F_INLINE __m128i abs_diff<uint8_t, arch_t::USE_SSSE3>(const __m128i& x, const __m128i& y)
    {
        return _mm_abs_epi16(_mm_sub_epi16(x, y));
    }

    template <>
    F_INLINE __m128i abs_diff<int16_t, arch_t::USE_SSSE3>(const __m128i& x, const __m128i& y)
    {
        return _mm_abs_epi16(_mm_sub_epi16(x, y));
    }

    template <>
    F_INLINE __m128i abs_diff<uint16_t, arch_t::USE_SSSE3>(const __m128i& x, const __m128i& y)
    {
        return _mm_abs_epi32(_mm_sub_epi32(x, y));
    }

    template <>
    F_INLINE __m128i abs_diff<uint8_t, arch_t::USE_SSE41>(const __m128i& x, const __m128i& y)
    {
        return abs_diff<uint8_t, arch_t::USE_SSSE3>(x, y);
    }

    template <>
    F_INLINE __m128i abs_diff<int16_t, arch_t::USE_SSE41>(const __m128i& x, const __m128i& y)
    {
        return abs_diff<int16_t, arch_t::USE_SSSE3>(x, y);
    }

    template <>
    F_INLINE __m128i abs_diff<uint16_t, arch_t::USE_SSE41>(const __m128i& x, const __m128i& y)
    {
        return abs_diff<uint16_t, arch_t::USE_SSSE3>(x, y);
    }

#if defined(__AVX__)
    template <typename T, arch_t A>
    static F_INLINE __m256 abs_diff(const __m256& x, const __m256& y)
    {
        return _mm256_max_ps(sub<T>(x, y), sub<T>(y, x));
    }

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
#endif // __AVX__



    template <typename V, typename T>
    static F_INLINE V average(const V& x, const V& y)
    {
        return div2<T>(add<T>(x, y));
    }


    template <typename T>
    static F_INLINE __m128 adjust(const __m128& x)
    {
        return x;
    }
    template <typename T>
    static F_INLINE __m128i adjust(const __m128i& x)
    {
        return add<T>(x, _mm_cmpeq_epi32(x, x));
    }

#if defined(__AVX__)
    template <typename T>
    static F_INLINE __m256 adjust(const __m256& x)
    {
        return x;
    }

    template <typename T>
    static F_INLINE __m256i adjust(const __m256i& x)
    {
        return add<T>(x, _mm256_cmpeq_epi32(x, x));
    }
#endif // __AVX__


    template <arch_t A>
    static F_INLINE __m128
        blendv(const __m128& x, const __m128& y, const __m128& m)
    {
        return _mm_or_ps(_mm_and_ps(m, y), _mm_andnot_ps(m, x));
    }

    template <arch_t A>
    static F_INLINE __m128i
        blendv(const __m128i& x, const __m128i& y, const __m128i& m)
    {
        return _mm_or_si128(_mm_and_si128(m, y), _mm_andnot_si128(m, x));
    }

    template <>
    F_INLINE __m128i
        blendv<arch_t::USE_SSE41>(const __m128i& x, const __m128i& y, const __m128i& m)
    {
        return _mm_blendv_epi8(x, y, m);
    }

#if defined(__AVX__)
    template <arch_t A>
    static F_INLINE __m256
        blendv(const __m256& x, const __m256& y, const __m256& m)
    {
        return _mm256_blendv_ps(x, y, m);
    }

    template <arch_t A>
    static F_INLINE __m256i
        blendv(const __m256i& x, const __m256i& y, const __m256i& m)
    {
        return _mm256_blendv_epi8(x, y, m);
    }
#endif // __AVX__

}

#endif //YADIFMOD2_SIMD_SSE41_H



#pragma once

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


#ifndef YADIFMOD2_SIMD_AVX_H
#define YADIFMOD2_SIMD_AVX_H

#include <immintrin.h>

#include "common.h"

#if !defined(__AVX__)
#error "AVX option needed"
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
    F_INLINE __m256 load<__m256, float, arch_t::USE_AVX>(const uint8_t* ptr)
    {
        return _mm256_loadu_ps(reinterpret_cast<const float*>(ptr));
    }


    //-------------------STORE---------------------------------------------------

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


    //-------------------SETZERO-------------------------------------------------

    template <typename V>
    static F_INLINE V setzero();

    template <>
    F_INLINE __m256 setzero<__m256>()
    {
        return _mm256_setzero_ps();
    }


    //-------------------ADD------------------------------------------------------

    template <typename T>
    static F_INLINE __m256 add(const __m256& x, const __m256& y)
    {
        return _mm256_add_ps(x, y);
    }


    //-------------------SUB-----------------------------------------------------

    template <typename T>
    static F_INLINE __m256 sub(const __m256& x, const __m256& y)
    {
        return _mm256_sub_ps(x, y);
    }


    //----------------------DIV2-------------------------------------------

    template <typename T>
    static F_INLINE __m256 div2(const __m256& x)
    {
        static __m256 coef = _mm256_set1_ps(0.5f);
        return _mm256_mul_ps(x, coef);
    }


    //-------------------MIN----------------------------------------------------

    template<typename T>
    static F_INLINE __m256 min(const __m256& x, const __m256& y)
    {
        return _mm256_min_ps(x, y);
    }


    //-----------------------MAX-----------------------------------------------

    template<typename T>
    static F_INLINE __m256 max(const __m256& x, const __m256& y)
    {
        return _mm256_max_ps(x, y);
    }


    //------------------CMPGT-----------------------------------------

    template <typename T>
    static F_INLINE __m256 cmpgt(const __m256& x, const __m256& y)
    {
        return _mm256_cmp_ps(x, y, _CMP_GT_OS);
    }


    //------------------MINUS-------------------------------------------
    template <typename V, typename T>
    static F_INLINE V minus(const V& x)
    {
        return sub<T>(setzero<V>(), x);
    }


    //--------------------ABS_DIFF--------------------------------------------

    template <typename T, arch_t A>
    static F_INLINE __m256 abs_diff(const __m256& x, const __m256& y)
    {
        return _mm256_max_ps(sub<T>(x, y), sub<T>(y, x));
    }


    template <typename V, typename T>
    static F_INLINE V average(const V& x, const V& y)
    {
        return div2<T>(add<T>(x, y));
    }


    template <typename T>
    static F_INLINE __m256 adjust(const __m256& x)
    {
        return x;
    }


    template <arch_t A>
    static F_INLINE __m256
        blendv(const __m256& x, const __m256& y, const __m256& m)
    {
        return _mm256_blendv_ps(x, y, m);
    }
}

#endif //YADIFMOD2_SIMD_AVX_H

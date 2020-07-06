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

#include <smmintrin.h>

#include "common.h"



namespace simd {
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


    //------------------MINUS-------------------------------------------

    template <typename V, typename T>
    static F_INLINE V minus(const V& x)
    {
        return sub<T>(setzero<V>(), x);
    }



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
}

#endif //YADIF_MOD_2_SIMD_H

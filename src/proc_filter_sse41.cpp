/*
    yadifmod2 : yadif + yadifmod for avysynth2.6/Avisynth+
        Copyright (C) 2016 OKA Motofumi

    yadifmod : Modification of Fizick's yadif avisynth filter.
        Copyright (C) 2007 Kevin Stone aka tritical

    Yadif C-plugin for Avisynth 2.5 : Port from MPlayer filter
        Copyright (C) 2007 Alexander G. Balakhnin aka Fizick

    YADIF : Yet Another DeInterlacing Filter
        Copyright (C) 2006 Michael Niedermayer <michaelni@gmx.at>

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



#include <map>

#include "simd.h"


//-------------------------LOAD------------------------------------------------

template <typename V, typename T, arch_t A>
static F_INLINE V load(const uint8_t* ptr);

template <>
F_INLINE __m128i load<__m128i, uint8_t, arch_t::USE_SSE41>(const uint8_t* ptr)
{
    __m128i t = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(ptr));
    return _mm_cvtepu8_epi16(t);
}

template <>
F_INLINE __m128i load<__m128i, uint16_t, arch_t::USE_SSE41>(const uint8_t* ptr)
{
    __m128i t = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(ptr));
    return _mm_cvtepu16_epi32(t);
}


//-------------------STORE---------------------------------------------------

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


//-------------------MIN----------------------------------------------------

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


//-----------------------MAX-----------------------------------------------

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


//--------------------ABS_DIFF--------------------------------------------

template <typename T, arch_t A>
static F_INLINE __m128i abs_diff(const __m128i& x, const __m128i& y)
{
    return _mm_or_si128(_mm_subs_epu16(x, y), _mm_subs_epu16(y, x));
}

template <>
F_INLINE __m128i abs_diff<uint8_t, arch_t::USE_SSSE3>(const __m128i& x, const __m128i& y)
{
    return _mm_abs_epi16(_mm_sub_epi16(x, y));
}

template <>
F_INLINE __m128i abs_diff<uint8_t, arch_t::USE_SSE41>(const __m128i& x, const __m128i& y)
{
    return abs_diff<uint8_t, arch_t::USE_SSSE3>(x, y);
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


template <typename V, typename T, arch_t ARCH>
static F_INLINE V calc_score(const uint8_t* ct, const uint8_t* cb, int n)
{
    using namespace simd;

    constexpr int s = sizeof(T);
    V ad0 = abs_diff<T, ARCH>(load<V, T, ARCH>(ct + (n - static_cast<int64_t>(1)) * s),
        load<V, T, ARCH>(cb - (n + static_cast<int64_t>(1)) * s));
    V ad1 = abs_diff<T, ARCH>(load<V, T, ARCH>(ct + static_cast<int64_t>(n) * s),
        load<V, T, ARCH>(cb - static_cast<int64_t>(n) * s));
    V ad2 = abs_diff<T, ARCH>(load<V, T, ARCH>(ct + (n + static_cast<int64_t>(1)) * s),
        load<V, T, ARCH>(cb - (n - static_cast<int64_t>(1)) * s));
    return add<T>(add<T>(ad0, ad1), ad2);
}


template <typename V, typename T, arch_t ARCH>
static F_INLINE V calc_spatial_pred(const uint8_t* ct, const uint8_t* cb)
{
    using namespace simd;

    constexpr int s = sizeof(T);

    V score = adjust<T>(calc_score<V, T, ARCH>(ct, cb, 0));
    V pred = average<V, T>(load<V, T, ARCH>(ct), load<V, T, ARCH>(cb));

    V sl_score1 = calc_score<V, T, ARCH>(ct, cb, -1);
    V sl_score2 = calc_score<V, T, ARCH>(ct, cb, -2);
    V sl_pred1 = average<V, T>(load<V, T, ARCH>(ct - s), load<V, T, ARCH>(cb + s));
    V sl_pred2 = average<V, T>(load<V, T, ARCH>(ct - 2 * s), load<V, T, ARCH>(cb + 2 * s));
    V mask = cmpgt<T>(sl_score1, sl_score2);
    sl_pred2 = blendv<ARCH>(sl_pred1, sl_pred2, mask);
    sl_score2 = min<T>(sl_score1, sl_score2);
    mask = cmpgt<T>(score, sl_score1);
    pred = blendv<ARCH>(pred, sl_pred2, mask);
    score = blendv<ARCH>(score, sl_score2, mask);

    sl_score1 = calc_score<V, T, ARCH>(ct, cb, 1);
    sl_score2 = calc_score<V, T, ARCH>(ct, cb, 2);
    sl_pred1 = average<V, T>(load<V, T, ARCH>(ct + s), load<V, T, ARCH>(cb - s));
    sl_pred2 = average<V, T>(load<V, T, ARCH>(ct + 2 * s), load<V, T, ARCH>(cb - 2 * s));
    mask = cmpgt<T>(sl_score1, sl_score2);
    sl_pred2 = blendv<ARCH>(sl_pred1, sl_pred2, mask);
    mask = cmpgt<T>(score, sl_score1);
    pred = blendv<ARCH>(pred, sl_pred2, mask);

    return pred;
}


template <typename V, typename T, size_t STEP, arch_t ARCH, bool SP_CHECK, bool HAS_EDEINT>
static void __stdcall
proc_simd(const uint8_t* currp, const uint8_t* prevp, const uint8_t* nextp,
    const uint8_t* fm_prev, const uint8_t* fm_next, const uint8_t* edeintp,
    uint8_t* dstp, int width, int cstride, int pstride, int nstride,
    const int fm_pstride, const int fm_nstride, const int estride2,
    const int dstride2, const int count) noexcept
{
    using namespace simd;

    const uint8_t* ct = currp - cstride;
    const uint8_t* cb = currp + cstride;
    const uint8_t* pt = prevp - pstride;
    const uint8_t* pb = prevp + pstride;
    const uint8_t* nt = nextp - nstride;
    const uint8_t* nb = nextp + nstride;
    const uint8_t* fmpt = fm_prev - fm_pstride;
    const uint8_t* fmpb = fm_prev + fm_pstride;
    const uint8_t* fmnt = fm_next - fm_nstride;
    const uint8_t* fmnb = fm_next + fm_nstride;

    cstride *= 2;
    pstride *= 2;
    nstride *= 2;
    width *= sizeof(T);

    for (int y = 0; y < count; ++y) {
        for (int x = 0; x < width; x += STEP) {
            const V fpm = load<V, T, ARCH>(fm_prev + x);
            const V fnm = load<V, T, ARCH>(fm_next + x);

            const V p1 = load<V, T, ARCH>(ct + x);
            const V p2 = average<V, T>(fpm, fnm);
            const V p3 = load<V, T, ARCH>(cb + x);

            const V d0 = div2<T>(abs_diff<T, ARCH>(fpm, fnm));
            const V d1 = average<V, T>(
                abs_diff<T, ARCH>(load<V, T, ARCH>(pt + x), p1),
                abs_diff<T, ARCH>(load<V, T, ARCH>(pb + x), p3));
            const V d2 = average<V, T>(
                abs_diff<T, ARCH>(load<V, T, ARCH>(nt + x), p1),
                abs_diff<T, ARCH>(load<V, T, ARCH>(nb + x), p3));

            V diff = max<T>(max<T>(d0, d1), d2);

            if (SP_CHECK) {
                const V p0 = sub<T>(average<V, T>(load<V, T, ARCH>(fmpt + x),
                    load<V, T, ARCH>(fmnt + x)),
                    p1);
                const V p4 = sub<T>(average<V, T>(load<V, T, ARCH>(fmpt + x),
                    load<V, T, ARCH>(fmnt + x)),
                    p3);
                const V p1_ = sub<T>(p2, p1);
                const V p3_ = sub<T>(p2, p3);

                const V maxs = max<T>(max<T>(p1_, p3_), min<T>(p0, p4));
                const V mins = min<T>(min<T>(p1_, p3_), max<T>(p0, p4));

                diff = max<T>(max<T>(diff, mins), minus<V, T>(maxs));
            }

            V sp_pred = HAS_EDEINT ? load<V, T, ARCH>(edeintp + x) :
                calc_spatial_pred<V, T, ARCH>(ct + x, cb + x);

            const V dst = min<T>(max<T>(sp_pred, sub<T>(p2, diff)), add<T>(p2, diff));

            store<T>(dstp + x, dst);
        }
        ct += cstride;
        cb += cstride;
        pt += pstride;
        pb += pstride;
        nt += nstride;
        nb += nstride;
        fm_prev += fm_pstride;
        fmpt += fm_pstride;
        fmpb += fm_pstride;
        fm_next += fm_nstride;
        fmnt += fm_nstride;
        fmnb += fm_nstride;
        dstp += dstride2;
        if (HAS_EDEINT) {
            edeintp += estride2;
        }

    }
}


proc_filter_t
get_main_proc_sse41(int bps, bool spcheck, bool edeint, arch_t arch)
{
    using std::make_tuple;

    std::map<std::tuple<int, int, bool, arch_t>, proc_filter_t> table;

    table[make_tuple(8, true, true, arch_t::USE_SSE41)] = proc_simd<__m128i, uint8_t, 8, arch_t::USE_SSE41, true, true>;
    table[make_tuple(8, true, false, arch_t::USE_SSE41)] = proc_simd<__m128i, uint8_t, 8, arch_t::USE_SSE41, true, false>;
    table[make_tuple(8, false, true, arch_t::USE_SSE41)] = proc_simd<__m128i, uint8_t, 8, arch_t::USE_SSE41, false, true>;
    table[make_tuple(8, false, false, arch_t::USE_SSE41)] = proc_simd<__m128i, uint8_t, 8, arch_t::USE_SSE41, false, false>;

    table[make_tuple(16, true, true, arch_t::USE_SSE41)] = proc_simd<__m128i, uint16_t, 8, arch_t::USE_SSE41, true, true>;
    table[make_tuple(16, true, false, arch_t::USE_SSE41)] = proc_simd<__m128i, uint16_t, 8, arch_t::USE_SSE41, true, false>;
    table[make_tuple(16, false, true, arch_t::USE_SSE41)] = proc_simd<__m128i, uint16_t, 8, arch_t::USE_SSE41, false, true>;
    table[make_tuple(16, false, false, arch_t::USE_SSE41)] = proc_simd<__m128i, uint16_t, 8, arch_t::USE_SSE41, false, false>;

    return table[make_tuple(bps, spcheck, edeint, arch)];
}

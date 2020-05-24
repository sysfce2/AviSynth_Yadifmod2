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



#include <cstdint>
#include <algorithm>
#include <map>
#include <tuple>

#include "common.h"
#include "simd.h"


static F_INLINE int average(const int x, const int y) noexcept
{
    return (x + y + 1) / 2;
}


F_INLINE float average(const float x, const float y) noexcept
{
    return (x + y) * 0.5f;
}


static F_INLINE int average2(const int x, const int y) noexcept
{
    return (x + y) / 2;
}


static F_INLINE float average2(const float x, const float y) noexcept
{
    return (x + y) * 0.5f;
}


static F_INLINE int absdiff(const int x, const int y) noexcept
{
    return x > y ? x - y : y - x;
}


static F_INLINE float absdiff(const float x, const float y) noexcept
{
    return x > y ? x - y : y - x;
}


template <typename T0, typename T1>
static F_INLINE T0 clamp(const T1 val, const T1 minimum, const T1 maximum) noexcept
{
    return static_cast<T0>(std::min(std::max(val, minimum), maximum));
}


template <typename T>
static void __stdcall
interpolate(uint8_t* dstp, const uint8_t* srcp, int stride, int width) noexcept
{
    const T* s0 = reinterpret_cast<const T*>(srcp);
    const T* s1 = reinterpret_cast<const T*>(srcp + stride * static_cast<int64_t>(2));
    T* d = reinterpret_cast<T*>(dstp);
    width /= sizeof(T);
    for (int x = 0; x < width; ++x) {
        d[x] = average(s0[x], s1[x]);
    }
}


template <typename T0, typename T1>
static F_INLINE T1 calc_score(const T0* ct, const T0* cb, int n) noexcept
{
    return absdiff(ct[-1 + n], cb[-1 - n]) + absdiff(ct[n], cb[-n])
        + absdiff(ct[1 + n], cb[1 - n]);
}


template <typename T0, typename T1>
static inline T1 calc_spatial_pred(const T0* ct, const T0* cb) noexcept
{
    constexpr T1 adjust = sizeof(T0) == 4 ? 0 : 1;

    T1 pred = average2(ct[0], cb[0]);
    T1 score = calc_score<T0, T1>(ct, cb, 0) - adjust;

    T1 sl_score = calc_score<T0, T1>(ct, cb, -1);
    if (sl_score < score) {
        score = sl_score;
        pred = average2(ct[-1], cb[1]);

        sl_score = calc_score<T0, T1>(ct, cb, -2);
        if (sl_score < score) {
            score = sl_score;
            pred = average2(ct[-2], cb[2]);
        }
    }

    sl_score = calc_score<T0, T1>(ct, cb, 1);
    if (sl_score < score) {
        score = sl_score;
        pred = average2(ct[1], cb[-1]);

        sl_score = calc_score<T0, T1>(ct, cb, 2);
        if (sl_score < score) {
            pred = average2(ct[2], cb[-2]);
        }
    }

    return pred;
}


template <typename T0, typename T1, bool SP_CHECK, bool HAS_EDEINT>
static void __stdcall
proc_cpp(const uint8_t* currp, const uint8_t* prevp, const uint8_t* nextp,
    const uint8_t* fm_prev, const uint8_t* fm_next, const uint8_t* edeintp,
    uint8_t* dstp, const int width, int cstride, int pstride, int nstride,
    int fm_pstride, int fm_nstride, int estride2, int dstride2,
    const int count) noexcept
{
    const T0* ct = reinterpret_cast<const T0*>(currp - cstride);
    const T0* cb = reinterpret_cast<const T0*>(currp + cstride);
    const T0* pt = reinterpret_cast<const T0*>(prevp - pstride);
    const T0* pb = reinterpret_cast<const T0*>(prevp + pstride);
    const T0* nt = reinterpret_cast<const T0*>(nextp - nstride);
    const T0* nb = reinterpret_cast<const T0*>(nextp + nstride);
    const T0* fmp = reinterpret_cast<const T0*>(fm_prev);
    const T0* fmn = reinterpret_cast<const T0*>(fm_next);
    const T0* fmpt = reinterpret_cast<const T0*>(fm_prev - fm_pstride);
    const T0* fmpb = reinterpret_cast<const T0*>(fm_prev + fm_pstride);
    const T0* fmnt = reinterpret_cast<const T0*>(fm_next - fm_nstride);
    const T0* fmnb = reinterpret_cast<const T0*>(fm_next + fm_nstride);
    const T0* edp = reinterpret_cast<const T0*>(edeintp);
    T0* dst0 = reinterpret_cast<T0*>(dstp);

    cstride = cstride * static_cast<int64_t>(2) / sizeof(T0);
    pstride = pstride * static_cast<int64_t>(2) / sizeof(T0);
    nstride = nstride * static_cast<int64_t>(2) / sizeof(T0);
    fm_pstride /= sizeof(T0);
    fm_nstride /= sizeof(T0);
    dstride2 /= sizeof(T0);
    estride2 /= sizeof(T0);

    for (int y = 0; y < count; ++y) {
        for (int x = 0; x < width; ++x) {
            const T1 p1 = static_cast<T1>(ct[x]);
            const T1 p2 = average2(fmp[x], fmn[x]);
            const T1 p3 = static_cast<T1>(cb[x]);
            const T1 d0 = absdiff(fmp[x], fmn[x]) / 2;
            const T1 d1 = average2(absdiff(pt[x], p1), absdiff(pb[x], p3));
            const T1 d2 = average2(absdiff(nt[x], p1), absdiff(nb[x], p3));
            T1 diff = std::max(std::max(d0, d1), d2);

            if (SP_CHECK) {
                const T1 p1_ = p2 - p1;
                const T1 p3_ = p2 - p3;
                const T1 p0 = average2(fmpt[x], fmnt[x]) - p1;
                const T1 p4 = average2(fmpb[x], fmnb[x]) - p3;
                const T1 maxs = std::max(std::max(p1_, p3_), std::min(p0, p4));
                const T1 mins = std::min(std::min(p1_, p3_), std::max(p0, p4));
                diff = std::max(std::max(diff, mins), -maxs);
            }

            const T1 spatial_pred = HAS_EDEINT ? static_cast<T1>(edp[x])
                : calc_spatial_pred<T0, T1>(ct + x, cb + x);

            dst0[x] = clamp<T0, T1>(spatial_pred, p2 - diff, p2 + diff);
        }
        ct += cstride;
        cb += cstride;
        pt += pstride;
        pb += pstride;
        nt += nstride;
        nb += nstride;
        fmp += fm_pstride;
        fmpt += fm_pstride;
        fmpb += fm_pstride;
        fmn += fm_nstride;
        fmnt += fm_nstride;
        fmnb += fm_nstride;
        dst0 += dstride2;
        if (HAS_EDEINT) {
            edp += estride2;
        }
    }
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
get_main_proc(int bps, bool spcheck, bool edeint, arch_t arch)
{
    using std::make_tuple;

    std::map<std::tuple<int, int, bool, arch_t>, proc_filter_t> table;

    table[make_tuple( 8, true,  true,  arch_t::NO_SIMD)] = proc_cpp<uint8_t, int, true,  true>;
    table[make_tuple( 8, true,  false, arch_t::NO_SIMD)] = proc_cpp<uint8_t, int, true,  false>;
    table[make_tuple( 8, false, true,  arch_t::NO_SIMD)] = proc_cpp<uint8_t, int, false, true>;
    table[make_tuple( 8, false, false, arch_t::NO_SIMD)] = proc_cpp<uint8_t, int, false, false>;

    table[make_tuple(16, true,  true,  arch_t::NO_SIMD)] = proc_cpp<uint16_t, int, true,  true>;
    table[make_tuple(16, true,  false, arch_t::NO_SIMD)] = proc_cpp<uint16_t, int, true,  false>;
    table[make_tuple(16, false, true,  arch_t::NO_SIMD)] = proc_cpp<uint16_t, int, false, true>;
    table[make_tuple(16, false, false, arch_t::NO_SIMD)] = proc_cpp<uint16_t, int, false, false>;

    table[make_tuple(32, true,  true,  arch_t::NO_SIMD)] = proc_cpp<float, float, true,  true>;
    table[make_tuple(32, true,  false, arch_t::NO_SIMD)] = proc_cpp<float, float, true,  false>;
    table[make_tuple(32, false, true,  arch_t::NO_SIMD)] = proc_cpp<float, float, false, true>;
    table[make_tuple(32, false, false, arch_t::NO_SIMD)] = proc_cpp<float, float, false, false>;

    table[make_tuple(8, true,  true,  arch_t::USE_SSE2)] = proc_simd<__m128i, uint8_t, 8, arch_t::USE_SSE2, true,  true>;
    table[make_tuple(8, true,  false, arch_t::USE_SSE2)] = proc_simd<__m128i, uint8_t, 8, arch_t::USE_SSE2, true,  false>;
    table[make_tuple(8, false, true,  arch_t::USE_SSE2)] = proc_simd<__m128i, uint8_t, 8, arch_t::USE_SSE2, false, true>;
    table[make_tuple(8, false, false, arch_t::USE_SSE2)] = proc_simd<__m128i, uint8_t, 8, arch_t::USE_SSE2, false, false>;

    table[make_tuple(16, true,  true,  arch_t::USE_SSE2)] = proc_simd<__m128i, uint16_t, 8, arch_t::USE_SSE2, true,  true>;
    table[make_tuple(16, true,  false, arch_t::USE_SSE2)] = proc_simd<__m128i, uint16_t, 8, arch_t::USE_SSE2, true,  false>;
    table[make_tuple(16, false, true,  arch_t::USE_SSE2)] = proc_simd<__m128i, uint16_t, 8, arch_t::USE_SSE2, false, true>;
    table[make_tuple(16, false, false, arch_t::USE_SSE2)] = proc_simd<__m128i, uint16_t, 8, arch_t::USE_SSE2, false, false>;

    table[make_tuple(32, true,  true,  arch_t::USE_SSE2)] = proc_simd<__m128, float, 16, arch_t::USE_SSE2, true,  true>;
    table[make_tuple(32, true,  false, arch_t::USE_SSE2)] = proc_simd<__m128, float, 16, arch_t::USE_SSE2, true,  false>;
    table[make_tuple(32, false, true,  arch_t::USE_SSE2)] = proc_simd<__m128, float, 16, arch_t::USE_SSE2, false, true>;
    table[make_tuple(32, false, false, arch_t::USE_SSE2)] = proc_simd<__m128, float, 16, arch_t::USE_SSE2, false, false>;

    table[make_tuple(8, true,  true,  arch_t::USE_SSSE3)] = proc_simd<__m128i, uint8_t, 8, arch_t::USE_SSSE3, true,  true>;
    table[make_tuple(8, true,  false, arch_t::USE_SSSE3)] = proc_simd<__m128i, uint8_t, 8, arch_t::USE_SSSE3, true,  false>;
    table[make_tuple(8, false, true,  arch_t::USE_SSSE3)] = proc_simd<__m128i, uint8_t, 8, arch_t::USE_SSSE3, false, true>;
    table[make_tuple(8, false, false, arch_t::USE_SSSE3)] = proc_simd<__m128i, uint8_t, 8, arch_t::USE_SSSE3, false, false>;

    table[make_tuple(16, true,  true,  arch_t::USE_SSSE3)] = proc_simd<__m128i, uint16_t, 8, arch_t::USE_SSSE3, true,  true>;
    table[make_tuple(16, true,  false, arch_t::USE_SSSE3)] = proc_simd<__m128i, uint16_t, 8, arch_t::USE_SSSE3, true,  false>;
    table[make_tuple(16, false, true,  arch_t::USE_SSSE3)] = proc_simd<__m128i, uint16_t, 8, arch_t::USE_SSSE3, false, true>;
    table[make_tuple(16, false, false, arch_t::USE_SSSE3)] = proc_simd<__m128i, uint16_t, 8, arch_t::USE_SSSE3, false, false>;

    table[make_tuple(8, true,  true,  arch_t::USE_SSE41)] = proc_simd<__m128i, uint8_t, 8, arch_t::USE_SSE41, true,  true>;
    table[make_tuple(8, true,  false, arch_t::USE_SSE41)] = proc_simd<__m128i, uint8_t, 8, arch_t::USE_SSE41, true,  false>;
    table[make_tuple(8, false, true,  arch_t::USE_SSE41)] = proc_simd<__m128i, uint8_t, 8, arch_t::USE_SSE41, false, true>;
    table[make_tuple(8, false, false, arch_t::USE_SSE41)] = proc_simd<__m128i, uint8_t, 8, arch_t::USE_SSE41, false, false>;

    table[make_tuple(16, true,  true,  arch_t::USE_SSE41)] = proc_simd<__m128i, uint16_t, 8, arch_t::USE_SSE41, true,  true>;
    table[make_tuple(16, true,  false, arch_t::USE_SSE41)] = proc_simd<__m128i, uint16_t, 8, arch_t::USE_SSE41, true,  false>;
    table[make_tuple(16, false, true,  arch_t::USE_SSE41)] = proc_simd<__m128i, uint16_t, 8, arch_t::USE_SSE41, false, true>;
    table[make_tuple(16, false, false, arch_t::USE_SSE41)] = proc_simd<__m128i, uint16_t, 8, arch_t::USE_SSE41, false, false>;
#if defined(__AVX__)
    table[make_tuple(32, true,  true,  arch_t::USE_AVX)] = proc_simd<__m256, float, 32, arch_t::USE_AVX, true,  true>;
    table[make_tuple(32, true,  false, arch_t::USE_AVX)] = proc_simd<__m256, float, 32, arch_t::USE_AVX, true,  false>;
    table[make_tuple(32, false, true,  arch_t::USE_AVX)] = proc_simd<__m256, float, 32, arch_t::USE_AVX, false, true>;
    table[make_tuple(32, false, false, arch_t::USE_AVX)] = proc_simd<__m256, float, 32, arch_t::USE_AVX, false, false>;

    table[make_tuple(8, true,  true,  arch_t::USE_AVX2)] = proc_simd<__m256i, uint8_t, 16, arch_t::USE_AVX2, true,  true>;
    table[make_tuple(8, true,  false, arch_t::USE_AVX2)] = proc_simd<__m256i, uint8_t, 16, arch_t::USE_AVX2, true,  false>;
    table[make_tuple(8, false, true,  arch_t::USE_AVX2)] = proc_simd<__m256i, uint8_t, 16, arch_t::USE_AVX2, false, true>;
    table[make_tuple(8, false, false, arch_t::USE_AVX2)] = proc_simd<__m256i, uint8_t, 16, arch_t::USE_AVX2, false, false>;

    table[make_tuple(16, true,  true,  arch_t::USE_AVX2)] = proc_simd<__m256i, uint16_t, 16, arch_t::USE_AVX2, true,  true>;
    table[make_tuple(16, true,  false, arch_t::USE_AVX2)] = proc_simd<__m256i, uint16_t, 16, arch_t::USE_AVX2, true,  false>;
    table[make_tuple(16, false, true,  arch_t::USE_AVX2)] = proc_simd<__m256i, uint16_t, 16, arch_t::USE_AVX2, false, true>;
    table[make_tuple(16, false, false, arch_t::USE_AVX2)] = proc_simd<__m256i, uint16_t, 16, arch_t::USE_AVX2, false, false>;
#endif // __AVX__

    return table[make_tuple(bps, spcheck, edeint, arch)];
}



interpolate_t get_interp(int bps)
{
    if (bps == 1) {
        return interpolate<uint8_t>;
    }
    if (bps == 2) {
        return interpolate<uint16_t>;
    }
    return interpolate<float>;
}

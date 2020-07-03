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

#include "common.h"

#ifndef __GNUC__
#define F_INLINE __forceinline
#else
#define F_INLINE __attribute__((always_inline)) inline
#endif

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

    return table[make_tuple(bps, spcheck, edeint, arch)];
}

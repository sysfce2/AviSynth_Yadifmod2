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
#include <tuple>
#include <map>
#include "yadifmod2.h"
#include "simd.h"


/* --- NO SIMD version ----------------------------------------------------- */
static inline int average(const int& x, const int& y)
{
    return (x + y) / 2;
}

static inline int abs_diff(const int& x, const int& y)
{
    return x > y ? x - y : y - x;
}

static inline uint8_t clamp(int val, int min, int max)
{
    return static_cast<uint8_t>(std::max(std::min(val, max), min));
}


template <int N>
static inline int
calc_slanting_score(const uint8_t* ct, const uint8_t* cb)
{
    return abs_diff(ct[-1 + N], cb[-1 - N]) + abs_diff(ct[N], cb[-N]) +
        abs_diff(ct[1 + N], cb[1 - N]);
}


static inline int calc_spatial_pred(const uint8_t* ct, const uint8_t* cb)
{
    int pred = average(ct[0], cb[0]);
    int score = abs_diff(ct[-1], cb[-1]) + abs_diff(ct[0], cb[0]) +
                abs_diff(ct[1], cb[1]) - 1;

    int sl_score = calc_slanting_score<-1>(ct, cb);
    if (sl_score < score) {
        score = sl_score;
        pred = average(ct[-1], cb[1]);

        sl_score = calc_slanting_score<-2>(ct, cb);
        if (sl_score < score) {
            pred = average(ct[-2], cb[2]);
        }
    }

    sl_score = calc_slanting_score<1>(ct, cb);
    if (sl_score < score) {
        score = sl_score;
        pred = average(ct[1], cb[-1]);

        sl_score = calc_slanting_score<2>(ct, cb);
        if (sl_score < score) {
            pred = average(ct[2], cb[-2]);
        }
    }

    return pred;
}


template <bool SP_CHECK, bool HAS_EDEINT>
static void __stdcall
proc_filter_c(const uint8_t* currp, const uint8_t* prevp, const uint8_t* nextp,
              const uint8_t* fmprev, const uint8_t* fmnext,
              const uint8_t* edeintp, uint8_t* dstp, const int width,
              const int cpitch, const int ppitch, const int npitch,
              const int fmppitch, const int fmnpitch, const int epitch2,
              const int dpitch2, const int count)
{
    const uint8_t* ct = currp - cpitch;
    const uint8_t* cb = currp + cpitch;
    const uint8_t* pt = prevp - ppitch;
    const uint8_t* pb = prevp + ppitch;
    const uint8_t* nt = nextp - npitch;
    const uint8_t* nb = nextp + npitch;
    const uint8_t* fmpt = fmprev - fmppitch;
    const uint8_t* fmpb = fmprev + fmppitch;
    const uint8_t* fmnt = fmnext - fmnpitch;
    const uint8_t* fmnb = fmnext + fmnpitch;

    for (int y = 0; y < count; ++y) {
        for (int x = 0; x < width; ++x) {
            const int p1 = ct[x];
            const int p2 = average(fmprev[x], fmnext[x]);
            const int p3 = cb[x];
            const int d0 = abs_diff(fmprev[x], fmnext[x]) / 2;
            const int d1 = average(abs_diff(pt[x], p1), abs_diff(pb[x], p3));
            const int d2 = average(abs_diff(nt[x], p1), abs_diff(nb[x], p3));
            int diff = std::max({ d0, d1, d2 });

            if (SP_CHECK) {
                const int p1_ = p2 - p1;
                const int p3_ = p2 - p3;
                const int p0 = average(fmpt[x], fmnt[x]) - p1;
                const int p4 = average(fmpb[x], fmnb[x]) - p3;
                const int maxs = std::max({ p1_, p3_, std::min(p0, p4) });
                const int mins = std::min({ p1_, p3_, std::max(p0, p4) });
                diff = std::max({ diff, mins, -maxs });
            }

            int spatial_pred =
                HAS_EDEINT ? edeintp[x] : calc_spatial_pred(ct + x, cb + x);

            dstp[x] = clamp(spatial_pred, p2 - diff, p2 + diff);

        }

        ct += cpitch * 2;
        cb += cpitch * 2;
        pt += ppitch * 2;
        pb += ppitch * 2;
        nt += npitch * 2;
        nb += npitch * 2;
        fmprev += fmppitch;
        fmpt += fmppitch;
        fmpb += fmppitch;
        fmnext += fmnpitch;
        fmnt += fmnpitch;
        fmnb += fmnpitch;
        dstp += dpitch2;
        if (HAS_EDEINT) {
            edeintp += epitch2;
        }
    }
}


/* --- SIMD version -------------------------------------------------------- */

template <typename T>
SFINLINE T calc_score(const T& ct, const T& cb)
{
    const T absdiff = abs_diff_u8(ct, cb);
    T sum = add_i16(cvt8to16lo(absdiff), cvt8to16lo(rshift<1>(absdiff)));
    return add_i16(sum, cvt8to16lo(rshift<2>(absdiff)));
}


template <typename T>
SFINLINE T
update_pred(const T& ct, const T& cb, const T& score, const T& sl_score, const T& pred)
{
    const T new_pred = avg_i16(cvt8to16lo(ct), cvt8to16lo(cb));
    return blendv(pred, new_pred, cmpgt_i16(score, sl_score));
}


template <typename T>
SFINLINE T calc_spatial_pred(const uint8_t* ct, const uint8_t* cb, const T& pred_orig)
{
    const T ct3 = loadu<T>(ct - 3);
    const T cb3 = loadu<T>(cb - 3);

    T score = calc_score(rshift<2>(ct3), rshift<2>(cb3));
    score = add_i16(score, cmpeq_i16(score, score)); // -1

    T sl_score = calc_score(rshift<1>(ct3), rshift<3>(cb3));
    T pred = update_pred(rshift<2>(ct3), rshift<4>(cb3), score, sl_score, pred_orig);
    score = min_i16(score, sl_score);

    sl_score = calc_score(ct3, rshift<4>(cb3));
    pred = update_pred(rshift<1>(ct3), rshift<5>(cb3), score, sl_score, pred);
    score = min_i16(score, sl_score);

    sl_score = calc_score(rshift<3>(ct3), rshift<1>(cb3));
    pred = update_pred(rshift<4>(ct3), rshift<2>(cb3), score, sl_score, pred);
    score = min_i16(score, sl_score);

    sl_score = calc_score(rshift<4>(ct3), cb3);
    return update_pred(rshift<5>(ct3), rshift<1>(cb3), score, sl_score, pred);
}


template <typename T, arch_t ARCH, bool SP_CHECK, bool HAS_EDEINT>
static void __stdcall
proc_filter(const uint8_t* currp, const uint8_t* prevp, const uint8_t* nextp,
            const uint8_t* fmprev, const uint8_t* fmnext,
            const uint8_t* edeintp, uint8_t* dstp, const int width,
            const int cpitch, const int ppitch, const int npitch,
            const int fmppitch, const int fmnpitch, const int epitch2,
            const int dpitch2, const int count)
{
    const uint8_t* ct = currp - cpitch;
    const uint8_t* cb = currp + cpitch;
    const uint8_t* pt = prevp - ppitch;
    const uint8_t* pb = prevp + ppitch;
    const uint8_t* nt = nextp - npitch;
    const uint8_t* nb = nextp + npitch;
    const uint8_t* fmpt = fmprev - fmppitch;
    const uint8_t* fmpb = fmprev + fmppitch;
    const uint8_t* fmnt = fmnext - fmnpitch;
    const uint8_t* fmnb = fmnext + fmnpitch;
    constexpr size_t step = sizeof(T) / 2;

    for (int y = 0; y < count; ++y) {
        for (int x = 0; x < width; x += step) {
            const T fpm = load_half<T>(fmprev + x);
            const T fnm = load_half<T>(fmnext + x);

            const T p1 = load_half<T>(ct + x);
            const T p2 = avg_i16<T>(fpm, fnm);
            const T p3 = load_half<T>(cb + x);

            const T d0 = div2_i16(abs_diff_i16<T, ARCH>(fpm, fnm));
            const T d1 = avg_i16<T>(abs_diff_i16<T, ARCH>(load_half<T>(pt + x), p1),
                                 abs_diff_i16<T, ARCH>(load_half<T>(pb + x), p3));
            const T d2 = avg_i16<T>(abs_diff_i16<T, ARCH>(load_half<T>(nt + x), p1),
                                 abs_diff_i16<T, ARCH>(load_half<T>(nb + x), p3));

            T diff = max3<T>(d0, d1, d2);

            if (SP_CHECK) {
                const T p0 = sub_i16(
                    avg_i16<T>(load_half<T>(fmpt + x), load_half<T>(fmnt + x)), p1);

                const T p4 = sub_i16(
                    avg_i16<T>(load_half<T>(fmpb + x), load_half<T>(fmnb + x)), p3);

                const T p1_ = sub_i16(p2, p1);
                const T p3_ = sub_i16(p2, p3);

                const T maxs = max3<T>(p1_, p3_, min_i16(p0, p4));
                const T mins = min3<T>(p1_, p3_, max_i16(p0, p4));

                diff = max3<T>(diff, mins, minus_i16<T>(maxs));
            }

            T sp_pred;
            if (HAS_EDEINT) {
                sp_pred = load_half<T>(edeintp + x);
            } else {
                sp_pred = calc_spatial_pred<T>(ct + x, cb + x, avg_i16(p1, p3));
            }
            const T dst = clamp_i16<T>(sp_pred, sub_i16(p2, diff), add_i16(p2, diff));

            store_half<T>(dstp + x, dst);
        }
        ct += cpitch * 2;
        cb += cpitch * 2;
        pt += ppitch * 2;
        pb += ppitch * 2;
        nt += npitch * 2;
        nb += npitch * 2;
        fmpt += fmppitch;
        fmprev += fmppitch;
        fmpb += fmppitch;
        fmnt += fmnpitch;
        fmnext += fmnpitch;
        fmnb += fmnpitch;
        dstp += dpitch2;
        if (HAS_EDEINT) {
            edeintp += epitch2;
        }
    }
}

/* --- function selector ---------------------------------------------------- */

proc_filter_t get_main_proc(bool sp_check, bool has_edeint, arch_t arch)
{
    using std::make_tuple;

    std::map<std::tuple<bool, bool, arch_t>, proc_filter_t> func;

    func[make_tuple(true,  true,  NO_SIMD)] = proc_filter_c<true,  true>;
    func[make_tuple(true,  false, NO_SIMD)] = proc_filter_c<true,  false>;
    func[make_tuple(false, true,  NO_SIMD)] = proc_filter_c<false, true>;
    func[make_tuple(false, false, NO_SIMD)] = proc_filter_c<false, false>;

    func[make_tuple(true,  true,  USE_SSE2)] = proc_filter<__m128i, USE_SSE2, true, true>;
    func[make_tuple(true,  false, USE_SSE2)] = proc_filter<__m128i, USE_SSE2, true, false>;
    func[make_tuple(false, true,  USE_SSE2)] = proc_filter<__m128i, USE_SSE2, false, true>;
    func[make_tuple(false, false, USE_SSE2)] = proc_filter<__m128i, USE_SSE2, false, false>;

    func[make_tuple(true,  true,  USE_SSSE3)] = proc_filter<__m128i, USE_SSSE3, true, true>;
    func[make_tuple(true,  false, USE_SSSE3)] = proc_filter<__m128i, USE_SSSE3, true, false>;
    func[make_tuple(false, true,  USE_SSSE3)] = proc_filter<__m128i, USE_SSSE3, false, true>;
    func[make_tuple(false, false, USE_SSSE3)] = proc_filter<__m128i, USE_SSSE3, false, false>;

    func[make_tuple(true,  true,  USE_AVX2)] = proc_filter<__m256i, USE_AVX2, true, true>;
    func[make_tuple(true,  false, USE_AVX2)] = proc_filter<__m256i, USE_AVX2, true, false>;
    func[make_tuple(false, true,  USE_AVX2)] = proc_filter<__m256i, USE_AVX2, false, true>;
    func[make_tuple(false, false, USE_AVX2)] = proc_filter<__m256i, USE_AVX2, false, false>;

    return func[make_tuple(sp_check, has_edeint, arch)];
}

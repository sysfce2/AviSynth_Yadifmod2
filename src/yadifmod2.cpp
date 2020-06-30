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



#include <stdexcept>

#include "avisynth.h"
#include "common.h"

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define NOMINMAX
#define NOGDI

#define YADIF_MOD_2_VERSION "0.2.6"

#define F_INLINE __forceinline


static F_INLINE int average(const int x, const int y) noexcept
{
    return (x + y + 1) / 2;
}

F_INLINE float average(const float x, const float y) noexcept
{
    return (x + y) * 0.5f;
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

interpolate_t get_interp(int bps)
{
    if (bps == 1)
        return interpolate<uint8_t>;

    if (bps == 2)
        return interpolate<uint16_t>;

    return interpolate<float>;
}


class YadifMod2 : public GenericVideoFilter
{
    PClip edeint;
    int nfSrc;
    int order;
    int field;
    int mode;
    int prevFirst;
    bool has_at_least_v8;

    proc_filter_t mainProc;
    interpolate_t interp;

public:
    YadifMod2(PClip child, PClip edeint, int order, int field, int mode,
             arch_t arch, IScriptEnvironment* env);
    ~YadifMod2() {}
    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
    int __stdcall SetCacheHints(int hints, int)
    {
        return hints == CACHE_GET_MTMODE ? MT_NICE_FILTER : 0;
    }
};


extern proc_filter_t get_main_proc(int bits, bool sp_check, bool has_edeint, arch_t arch);
extern proc_filter_t get_main_proc_sse2(int bits, bool sp_check, bool has_edeint, arch_t arch);
extern proc_filter_t get_main_proc_ssse3(int bits, bool sp_check, bool has_edeint, arch_t arch);
extern proc_filter_t get_main_proc_sse41(int bits, bool sp_check, bool has_edeint, arch_t arch);
extern proc_filter_t get_main_proc_avx(int bits, bool sp_check, bool has_edeint, arch_t arch);
extern proc_filter_t get_main_proc_avx2(int bits, bool sp_check, bool has_edeint, arch_t arch);
extern interpolate_t get_interp(int bytes_per_sample);


YadifMod2::YadifMod2(PClip c, PClip e, int o, int f, int m, arch_t arch, IScriptEnvironment* env) :
    GenericVideoFilter(c), edeint(e), order(o), field(f), mode(m)
{
    has_at_least_v8 = true;
    try { env->CheckVersion(8); }

    catch (const AvisynthError&) { has_at_least_v8 = false; }

    nfSrc = vi.num_frames;

    if (mode == 1 || mode == 3) {
        vi.num_frames *= 2;
        vi.SetFPS(vi.fps_numerator * 2, vi.fps_denominator);
    }

    if (order == -1) {
        order = child->GetParity(0) ? 1 : 0;
    }
    if (field == -1) {
        field = order;
    }

    prevFirst = nfSrc == 1 ? 0 : edeint ? 0 : 1; // forth original yadif and yadifmod

    interp = get_interp(vi.ComponentSize());

    int bits;
    if (vi.ComponentSize() == 1)
        bits = 8;
    else if (vi.ComponentSize() == 2)
        bits = 16;
    else
        bits = 32;

    if (arch != arch_t::NO_SIMD && bits == 32)
        arch = arch < arch_t::USE_AVX ? arch_t::USE_SSE2 : arch_t::USE_AVX;
    else if (arch == arch_t::USE_AVX && bits != 32)
        arch = arch_t::USE_SSE41;

    if (arch == arch_t::NO_SIMD)
        mainProc = get_main_proc(bits, mode < 2, edeint != nullptr, arch);
    else if (arch == arch_t::USE_SSE2)
        mainProc = get_main_proc_sse2(bits, mode < 2, edeint != nullptr, arch);
    else if (arch == arch_t::USE_SSSE3)
        mainProc = get_main_proc_ssse3(bits, mode < 2, edeint != nullptr, arch);
    else if (arch == arch_t::USE_SSE41)
        mainProc = get_main_proc_sse41(bits, mode < 2, edeint != nullptr, arch);
    else if (arch == arch_t::USE_AVX)
        mainProc = get_main_proc_avx(bits, mode < 2, edeint != nullptr, arch);
    else
        mainProc = get_main_proc_avx2(bits, mode < 2, edeint != nullptr, arch);

    child->SetCacheHints(CACHE_WINDOW, 3);
    if (edeint) {
        edeint->SetCacheHints(CACHE_NOTHING, 0);
    }
}


PVideoFrame __stdcall YadifMod2::GetFrame(int n, IScriptEnvironment* env)
{
    PVideoFrame edeint;
    if (this->edeint) {
        edeint = this->edeint->GetFrame(n, env);
    }

    int ft = field;
    if (mode == 1 || mode == 3) {
        ft = (n & 1) ? 1 - order : order;
        n /= 2;
    }

    auto next = child->GetFrame(std::min(n + 1, nfSrc - 1), env);
    auto curr = child->GetFrame(n, env);
    auto prev = child->GetFrame(n == 0 ? prevFirst : n - 1, env);

    PVideoFrame dst;
    if (has_at_least_v8) dst = env->NewVideoFrameP(vi, &curr); else dst = env->NewVideoFrame(vi);

    int planes_y[4] = { PLANAR_Y, PLANAR_U, PLANAR_V, PLANAR_A };
    int planes_r[4] = { PLANAR_G, PLANAR_B, PLANAR_R, PLANAR_A };
    const int* current_planes = (vi.IsYUV() || vi.IsYUVA()) ? planes_y : planes_r;
    int planecount = std::min(vi.NumComponents(), 3);
    for (int i = 0; i < planecount; i++)
    {
        const int plane = current_planes[i];
 
        const uint8_t* currp = curr->GetReadPtr(plane);
        const uint8_t* prevp = prev->GetReadPtr(plane);
        const uint8_t* nextp = next->GetReadPtr(plane);

        const int width = curr->GetRowSize(plane);
        const int height = curr->GetHeight(plane);
        const int cpitch = curr->GetPitch(plane);
        const int ppitch = prev->GetPitch(plane);
        const int npitch = next->GetPitch(plane);

        const int begin = 2 + ft;
        const int count = (height - 4 + ft - begin) / 2 + 1;

        const uint8_t *fm_prev, *fm_next;
        int fm_ppitch, fm_npitch;
        if (ft != order) {
            fm_ppitch = cpitch * 2;
            fm_npitch = npitch * 2;
            fm_prev = currp + static_cast<int64_t>(begin) * cpitch;
            fm_next = nextp + static_cast<int64_t>(begin) * npitch;
        } else {
            fm_ppitch = ppitch * 2;
            fm_npitch = cpitch * 2;
            fm_prev = prevp + static_cast<int64_t>(begin) * ppitch;
            fm_next = currp + static_cast<int64_t>(begin) * cpitch;
        }

        const uint8_t* edeintp = nullptr;
        int epitch = 0;
        if (this->edeint) {
            edeintp = edeint->GetReadPtr(plane);
            epitch = edeint->GetPitch(plane);
        }

        uint8_t* dstp = dst->GetWritePtr(plane);
        const int dpitch = dst->GetPitch(plane);

        if (ft == 0) {
            memcpy(dstp, currp + cpitch, width);
            if (edeintp) {
                memcpy(dstp + static_cast<int64_t>(dpitch) * (static_cast<int64_t>(height) - 2),
                       edeintp + static_cast<int64_t>(epitch) * (static_cast<int64_t>(height) - 2), width);
            } else {
                interp(dstp + static_cast<int64_t>(dpitch) * (static_cast<int64_t>(height) - 2),
                       currp + static_cast<int64_t>(cpitch) * (static_cast<int64_t>(height) - 3), cpitch, width);
            }
        } else {
            if (edeintp) {
                memcpy(dstp + dpitch, edeintp + epitch, width);
            } else {
                interp(dstp + dpitch, currp, cpitch, width);
            }
            memcpy(dstp + static_cast<int64_t>(dpitch) * (static_cast<int64_t>(height) - 1),
                   currp + static_cast<int64_t>(cpitch) * (static_cast<int64_t>(height) - 2), width);
        }
        env->BitBlt(dstp + (1 - static_cast<int64_t>(ft)) * dpitch, 2 * dpitch,
                    currp + (1 - static_cast<int64_t>(ft)) * cpitch, 2 * cpitch, width, height / 2);

        mainProc(currp + static_cast<int64_t>(begin) * cpitch, prevp + static_cast<int64_t>(begin) * ppitch,
                 nextp + static_cast<int64_t>(begin) * npitch, fm_prev, fm_next,
                 edeintp + static_cast<int64_t>(begin) * epitch, dstp + static_cast<int64_t>(begin) * dpitch, width / vi.ComponentSize(),
                 cpitch, ppitch, npitch, fm_ppitch, fm_npitch, 2 * epitch,
                 2 * dpitch, count);
    }

    return dst;
}


static arch_t get_arch(int opt, IScriptEnvironment* env) noexcept
{
    const bool has_sse2 = env->GetCPUFlags() & CPUF_SSE2;
    const bool has_ssse3 = env->GetCPUFlags() & CPUF_SSSE3;
    const bool has_sse41 = env->GetCPUFlags() & CPUF_SSE4_1;
    const bool has_avx = env->GetCPUFlags() & CPUF_AVX;
    const bool has_avx2 = env->GetCPUFlags() & CPUF_AVX2;

    if (opt == 0 || !has_sse2)
        return arch_t::NO_SIMD;

    if (opt == 1 || !has_ssse3)
        return arch_t::USE_SSE2;

    if (opt == 2 || !has_sse41)
        return arch_t::USE_SSSE3;

    if (opt == 3 || !has_avx)
        return arch_t::USE_SSE41;

    if (opt == 4 || !has_avx2)
        return arch_t::USE_AVX;

    return arch_t::USE_AVX2;
}


static inline void validate(bool cond, const char* msg)
{
    if (cond) {
        throw std::runtime_error(msg);
    }
}


static AVSValue __cdecl
create_yadifmod2(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    try {
        PClip child = args[0].AsClip();
        const VideoInfo& vi = child->GetVideoInfo();
        validate(!vi.IsPlanar(), "input clip must be a planar format.");

        int order = args[1].AsInt(-1);
        validate(order < -1 || order > 1, "order must be set to -1, 0 or 1.");

        int field = args[2].AsInt(-1);
        validate(field < -1 || field > 1, "field must be set to -1, 0 or 1.");

        int mode = args[3].AsInt(0);
        validate(mode < 0 || mode > 3, "mode must be set to 0, 1, 2 or 3");

        PClip edeint = nullptr;
        if (args[4].Defined()) {
            edeint = args[4].AsClip();
            const VideoInfo& vi_ed = edeint->GetVideoInfo();
            validate(!vi.IsSameColorspace(vi_ed),
                     "edeint clip's colorspace doesn't match.");
            validate(vi.width != vi_ed.width || vi.height != vi_ed.height,
                    "input and edeint must be the same resolution.");
            validate(vi.num_frames * ((mode & 1) ? 2 : 1) != vi_ed.num_frames,
                     "edeint clip's number of frames doesn't match.");
        }

        int opt = args[5].AsInt(5);
        arch_t arch = get_arch(opt, env);
        validate(opt < 0 || opt > 5, "opt must be between 0..5.");

        return new YadifMod2(child, edeint, order, field, mode, arch, env);

    } catch (std::runtime_error& e) {
        env->ThrowError("yadifmod2: %s", e.what());
    }

    return 0;
}


static const AVS_Linkage* AVS_linkage = nullptr;


extern "C" __declspec(dllexport) const char* __stdcall
AvisynthPluginInit3(IScriptEnvironment* env, const AVS_Linkage* vectors)
{
    AVS_linkage = vectors;

    const char* args =
        "c"
        "[order]i"
        "[field]i"
        "[mode]i"
        "[edeint]c"
        "[opt]i";

    env->AddFunction("yadifmod2", args, create_yadifmod2, nullptr);

    return "yadifmod2 = yadif + yadifmod ... ver. " YADIF_MOD_2_VERSION;
}

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
#include <stdexcept>
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define NOMINMAX
#define NOGDI
#include <windows.h>
#include <avisynth.h>

#include "common.h"



#define YADIF_MOD_2_VERSION "0.2.3"


typedef IScriptEnvironment ise_t;




class YadifMod2 : public GenericVideoFilter {
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
              int bits, arch_t arch, ise_t* env);
    ~YadifMod2() {}
    PVideoFrame __stdcall GetFrame(int n, ise_t* env);
    int __stdcall SetCacheHints(int hints, int)
    {
        return hints == CACHE_GET_MTMODE ? MT_NICE_FILTER : 0;
    }
};


extern proc_filter_t get_main_proc(int bits, bool sp_check, bool has_edeint, arch_t arch);
extern interpolate_t get_interp(int bytes_per_sample);


YadifMod2::YadifMod2(PClip c, PClip e, int o, int f, int m, int bits, arch_t arch, ise_t* env) :
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

    if (arch == arch_t::NO_SIMD && bits == 10) {
        bits = 16;
    }
    if (arch != arch_t::NO_SIMD && bits == 32) {
        arch = arch < arch_t::USE_AVX ? arch_t::USE_SSE2 : arch_t::USE_AVX;
    }

    mainProc = get_main_proc(bits, mode < 2, edeint != nullptr, arch);

    child->SetCacheHints(CACHE_WINDOW, 3);
    if (edeint) {
        edeint->SetCacheHints(CACHE_NOTHING, 0);
    }
}


PVideoFrame __stdcall YadifMod2::GetFrame(int n, ise_t* env)
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
    const int* current_planes = planes_y;
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


static arch_t get_arch(int opt, ise_t* env) noexcept
{
    const bool has_sse2 = env->GetCPUFlags() & CPUF_SSE2;
    const bool has_ssse3 = env->GetCPUFlags() & CPUF_SSSE3;
    const bool has_sse41 = env->GetCPUFlags() & CPUF_SSE4_1;
    const bool has_avx = env->GetCPUFlags() & CPUF_AVX;
    const bool has_avx2 = env->GetCPUFlags() & CPUF_AVX2;

    if (opt == 0 || !has_sse2) {
        return arch_t::NO_SIMD;
    }
    if (opt == 1 || !has_ssse3) {
        return arch_t::USE_SSE2;
    }
    if (opt == 2 || !has_sse41) {
        return arch_t::USE_SSSE3;
    }
#if !defined(__AVX__)
    return arch_t::USE_SSE41;
#else
    if (opt == 3 || !has_avx) {
        return arch_t::USE_SSE41;
    }
    if (opt == 4 || !has_avx2) {
        return arch_t::USE_AVX;
    }
    return arch_t::USE_AVX2;
#endif
}


static inline void validate(bool cond, const char* msg)
{
    if (cond) {
        throw std::runtime_error(msg);
    }
}


static int get_bits(int pixel_type)
{
    int bits = pixel_type & VideoInfo::CS_Sample_Bits_Mask;
    switch (bits) {
    case VideoInfo::CS_Sample_Bits_8: return 8;
    case VideoInfo::CS_Sample_Bits_10: return 10;
    case VideoInfo::CS_Sample_Bits_12:
    case VideoInfo::CS_Sample_Bits_14:
    case VideoInfo::CS_Sample_Bits_16: return 16;
    case VideoInfo::CS_Sample_Bits_32: return 32;
    }
    return 0;
}


static AVSValue __cdecl
create_yadifmod2(AVSValue args, void* user_data, ise_t* env)
{
    try {
        PClip child = args[0].AsClip();
        const VideoInfo& vi = child->GetVideoInfo();
        validate(!vi.IsPlanar(), "input clip must be a planar format.");
        validate(vi.IsYUVA() || vi.IsPlanarRGBA(), "alpha is not supported.");

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

        bool is_plus = env->FunctionExists("SetFilterMTMode");
        int sample_bytes = vi.BytesFromPixels(1);

        int bits = get_bits(vi.pixel_type);

        arch_t arch = get_arch(args[5].AsInt(-1), env);

        return new YadifMod2(child, edeint, order, field, mode, bits, arch, env);

    } catch (std::runtime_error& e) {
        env->ThrowError("yadifmod2: %s", e.what());
    }
    return 0;
}


static const AVS_Linkage* AVS_linkage = nullptr;


extern "C" __declspec(dllexport) const char* __stdcall
AvisynthPluginInit3(ise_t* env, const AVS_Linkage* vectors)
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

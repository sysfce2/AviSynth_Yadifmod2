#ifndef VAPOURSYNTH_YADIF_MOD2_H
#define VAPOURSYNTH_YADIF_MOD2_H

#include <cstdint>
#include "myvshelper.h"


typedef void(*proc_filter_t)(
    const uint8_t* currp, const uint8_t* prevp, const uint8_t* nextp,
    const uint8_t* fm_prev, const uint8_t* fm_next, const uint8_t* edeintp,
    uint8_t* dstp, size_t width, int cstride, int pstride, int nstride,
    int fm_pstride, int fm_nstride, int estride2, int dstride2, const int count);

typedef void(*interpolate_t)(
    uint8_t* dstp, const uint8_t* srcp, int stride, size_t width);


class YadifMod2 {
    VSNodeRef* clip;
    VSNodeRef* eclip;
    VSVideoInfo vi;
    int nfSrc;
    int order;
    int field;
    int mode;
    int prevFirst;

    interpolate_t interp;
    proc_filter_t mainProc;

public:
    YadifMod2(VSNodeRef* clip, VSNodeRef* eclip, int order, int field,
              int mode, arch_t arch, VSCore* core, const VSAPI* api);
    ~YadifMod2(){}
    const VSFrameRef* getFrame(int n, VSCore* core, const VSAPI* api,
                               VSFrameContext* ctx);
    void requestFrames(int n, const VSAPI* api, VSFrameContext* ctx);
    void freeClips(const VSAPI* api) {
        if (eclip) {
            api->freeNode(eclip);
        }
        api->freeNode(clip);
    };
    const VSVideoInfo* getVideoInfo() { return &vi; }
};


proc_filter_t
get_main_proc(int bits_per_sample, bool spcheck, bool edeint, arch_t arch);


interpolate_t get_interp(int bytes_per_sample);


#endif

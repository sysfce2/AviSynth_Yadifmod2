#ifndef VAPOURSYNTH_YADIF_MOD2_H
#define VAPOURSYNTH_YADIF_MOD2_H

#include <cstdint>
#include "myvshelper.h"


typedef void(*proc_filter_t)(
    const uint8_t* currp, const uint8_t* prevp, const uint8_t* nextp,
    const uint8_t* fm_prev, const uint8_t* fm_next, const uint8_t* edeintp,
    uint8_t* dstp, size_t width, int cstride, int pstride, int nstride,
    int fm_pstride, int fm_nstride, int estride2, int dstride2, const int count);

class YadifMod2 {
    VSNodeRef* clip;
    VSNodeRef* edeint;
    VSVideoInfo vi;
    int nfSrc;
    int order;
    int field;
    int mode;
    int prevFirst;

    void(*interp)(uint8_t* dstp, const uint8_t* srcp0, int stride, size_t width);
    proc_filter_t mainProc;

public:
    YadifMod2(VSNodeRef* clip, VSNodeRef* edeint, int order, int field,
              int mode, arch_t arch, VSCore* core, const VSAPI* api);
    ~YadifMod2(){}
    const VSFrameRef* getFrame(int n, VSCore* core, const VSAPI* api,
                               VSFrameContext* ctx);
    void requestFrames(int n, const VSAPI* api, VSFrameContext* ctx);
    void freeClips(const VSAPI* api) {
        if (edeint) {
            api->freeNode(edeint);
        }
        api->freeNode(clip);
    };
    const VSVideoInfo* getVideoInfo() { return &vi; }
};


#endif

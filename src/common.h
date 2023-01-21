#ifndef YADIF_MOD2_COMMON_H
#define YADIF_MOD2_COMMON_H

#include <cstdint>
#include <algorithm>

#include <avisynth.h>

#ifndef __GNUC__
#define F_INLINE __forceinline
#else
#define F_INLINE __attribute__((always_inline)) inline
#endif


enum class arch_t {
    NO_SIMD,
    USE_SSE2,
    USE_SSSE3,
    USE_SSE41,
    USE_AVX,
    USE_AVX2,
};


typedef void(__stdcall *proc_filter_t)(
    const uint8_t* currp, const uint8_t* prevp, const uint8_t* nextp,
    const uint8_t* fm_prev, const uint8_t* fm_next, const uint8_t* edeintp,
    uint8_t* dstp, const int width, const int cpitch, const int ppitch,
    const int npitch, const int fm_ppitch, const int fm_npitch,
    const int epitch2, const int dpitch2, const int count);


typedef void(__stdcall *interpolate_t)(
    uint8_t* dstp, const uint8_t* srcp, int stride, int width);

#endif

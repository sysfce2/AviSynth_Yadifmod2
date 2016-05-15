#include <algorithm>
#include <cstring>
#include <map>
#include <tuple>
#include "yadifmod2.h"
#include "proc_filter.h"



static proc_filter_t
get_main_proc(int bps, bool spcheck, bool edeint, arch_t arch)
{
    using std::make_tuple;

    std::map<std::tuple<int, int, bool, arch_t>, proc_filter_t> table;

    table[make_tuple( 8, true,  true,  NO_SIMD)] = proc_cpp<uint8_t, int, true,  true>;
    table[make_tuple( 8, true,  false, NO_SIMD)] = proc_cpp<uint8_t, int, true,  false>;
    table[make_tuple( 8, false, true,  NO_SIMD)] = proc_cpp<uint8_t, int, false, true>;
    table[make_tuple( 8, false, false, NO_SIMD)] = proc_cpp<uint8_t, int, false, false>;

    table[make_tuple(16, true,  true,  NO_SIMD)] = proc_cpp<uint16_t, int, true,  true>;
    table[make_tuple(16, true,  false, NO_SIMD)] = proc_cpp<uint16_t, int, true,  false>;
    table[make_tuple(16, false, true,  NO_SIMD)] = proc_cpp<uint16_t, int, false, true>;
    table[make_tuple(16, false, false, NO_SIMD)] = proc_cpp<uint16_t, int, false, false>;

    table[make_tuple(32, true,  true,  NO_SIMD)] = proc_cpp<float, float, true,  true>;
    table[make_tuple(32, true,  false, NO_SIMD)] = proc_cpp<float, float, true,  false>;
    table[make_tuple(32, false, true,  NO_SIMD)] = proc_cpp<float, float, false, true>;
    table[make_tuple(32, false, false, NO_SIMD)] = proc_cpp<float, float, false, false>;
#if defined(__SSE2__)
    table[make_tuple(8, true,  true,  USE_SSE2)] = proc_simd<__m128i, uint8_t, 8, USE_SSE2, true,  true>;
    table[make_tuple(8, true,  false, USE_SSE2)] = proc_simd<__m128i, uint8_t, 8, USE_SSE2, true,  false>;
    table[make_tuple(8, false, true,  USE_SSE2)] = proc_simd<__m128i, uint8_t, 8, USE_SSE2, false, true>;
    table[make_tuple(8, false, false, USE_SSE2)] = proc_simd<__m128i, uint8_t, 8, USE_SSE2, false, true>;

    table[make_tuple(10, true,  true,  USE_SSE2)] = proc_simd<__m128i, int16_t, 16, USE_SSE2, true,  true>;
    table[make_tuple(10, true,  false, USE_SSE2)] = proc_simd<__m128i, int16_t, 16, USE_SSE2, true,  false>;
    table[make_tuple(10, false, true,  USE_SSE2)] = proc_simd<__m128i, int16_t, 16, USE_SSE2, false, true>;
    table[make_tuple(10, false, false, USE_SSE2)] = proc_simd<__m128i, int16_t, 16, USE_SSE2, false, true>;

    table[make_tuple(16, true,  true,  USE_SSE2)] = proc_simd<__m128i, uint16_t, 8, USE_SSE2, true,  true>;
    table[make_tuple(16, true,  false, USE_SSE2)] = proc_simd<__m128i, uint16_t, 8, USE_SSE2, true,  false>;
    table[make_tuple(16, false, true,  USE_SSE2)] = proc_simd<__m128i, uint16_t, 8, USE_SSE2, false, true>;
    table[make_tuple(16, false, false, USE_SSE2)] = proc_simd<__m128i, uint16_t, 8, USE_SSE2, false, true>;

    table[make_tuple(32, true,  true,  USE_SSE2)] = proc_simd<__m128, float, 16, USE_SSE2, true,  true>;
    table[make_tuple(32, true,  false, USE_SSE2)] = proc_simd<__m128, float, 16, USE_SSE2, true,  false>;
    table[make_tuple(32, false, true,  USE_SSE2)] = proc_simd<__m128, float, 16, USE_SSE2, false, true>;
    table[make_tuple(32, false, false, USE_SSE2)] = proc_simd<__m128, float, 16, USE_SSE2, false, true>;
#if defined(__SSSE3__)
    table[make_tuple(8, true,  true,  USE_SSSE3)] = proc_simd<__m128i, uint8_t, 8, USE_SSSE3, true,  true>;
    table[make_tuple(8, true,  false, USE_SSSE3)] = proc_simd<__m128i, uint8_t, 8, USE_SSSE3, true,  false>;
    table[make_tuple(8, false, true,  USE_SSSE3)] = proc_simd<__m128i, uint8_t, 8, USE_SSSE3, false, true>;
    table[make_tuple(8, false, false, USE_SSSE3)] = proc_simd<__m128i, uint8_t, 8, USE_SSSE3, false, true>;

    table[make_tuple(10, true,  true,  USE_SSSE3)] = proc_simd<__m128i, int16_t, 16, USE_SSSE3, true,  true>;
    table[make_tuple(10, true,  false, USE_SSSE3)] = proc_simd<__m128i, int16_t, 16, USE_SSSE3, true,  false>;
    table[make_tuple(10, false, true,  USE_SSSE3)] = proc_simd<__m128i, int16_t, 16, USE_SSSE3, false, true>;
    table[make_tuple(10, false, false, USE_SSSE3)] = proc_simd<__m128i, int16_t, 16, USE_SSSE3, false, true>;

    table[make_tuple(16, true,  true,  USE_SSSE3)] = proc_simd<__m128i, uint16_t, 8, USE_SSSE3, true,  true>;
    table[make_tuple(16, true,  false, USE_SSSE3)] = proc_simd<__m128i, uint16_t, 8, USE_SSSE3, true,  false>;
    table[make_tuple(16, false, true,  USE_SSSE3)] = proc_simd<__m128i, uint16_t, 8, USE_SSSE3, false, true>;
    table[make_tuple(16, false, false, USE_SSSE3)] = proc_simd<__m128i, uint16_t, 8, USE_SSSE3, false, true>;
#if defined(__SSE4_1__)
    table[make_tuple(8, true,  true,  USE_SSE41)] = proc_simd<__m128i, uint8_t, 8, USE_SSE41, true,  true>;
    table[make_tuple(8, true,  false, USE_SSE41)] = proc_simd<__m128i, uint8_t, 8, USE_SSE41, true,  false>;
    table[make_tuple(8, false, true,  USE_SSE41)] = proc_simd<__m128i, uint8_t, 8, USE_SSE41, false, true>;
    table[make_tuple(8, false, false, USE_SSE41)] = proc_simd<__m128i, uint8_t, 8, USE_SSE41, false, true>;

    table[make_tuple(10, true,  true,  USE_SSE41)] = proc_simd<__m128i, int16_t, 16, USE_SSE41, true,  true>;
    table[make_tuple(10, true,  false, USE_SSE41)] = proc_simd<__m128i, int16_t, 16, USE_SSE41, true,  false>;
    table[make_tuple(10, false, true,  USE_SSE41)] = proc_simd<__m128i, int16_t, 16, USE_SSE41, false, true>;
    table[make_tuple(10, false, false, USE_SSE41)] = proc_simd<__m128i, int16_t, 16, USE_SSE41, false, true>;

    table[make_tuple(16, true,  true,  USE_SSE41)] = proc_simd<__m128i, uint16_t, 8, USE_SSE41, true,  true>;
    table[make_tuple(16, true,  false, USE_SSE41)] = proc_simd<__m128i, uint16_t, 8, USE_SSE41, true,  false>;
    table[make_tuple(16, false, true,  USE_SSE41)] = proc_simd<__m128i, uint16_t, 8, USE_SSE41, false, true>;
    table[make_tuple(16, false, false, USE_SSE41)] = proc_simd<__m128i, uint16_t, 8, USE_SSE41, false, true>;
#if defined(__AVX__)
    table[make_tuple(32, true,  true,  USE_AVX)] = proc_simd<__m256, float, 32, USE_AVX, true,  true>;
    table[make_tuple(32, true,  false, USE_AVX)] = proc_simd<__m256, float, 32, USE_AVX, true,  false>;
    table[make_tuple(32, false, true,  USE_AVX)] = proc_simd<__m256, float, 32, USE_AVX, false, true>;
    table[make_tuple(32, false, false, USE_AVX)] = proc_simd<__m256, float, 32, USE_AVX, false, true>;
#if defined(__AVX2__)
    table[make_tuple(8, true,  true,  USE_AVX2)] = proc_simd<__m256i, uint8_t, 16, USE_AVX2, true,  true>;
    table[make_tuple(8, true,  false, USE_AVX2)] = proc_simd<__m256i, uint8_t, 16, USE_AVX2, true,  false>;
    table[make_tuple(8, false, true,  USE_AVX2)] = proc_simd<__m256i, uint8_t, 16, USE_AVX2, false, true>;
    table[make_tuple(8, false, false, USE_AVX2)] = proc_simd<__m256i, uint8_t, 16, USE_AVX2, false, true>;

    table[make_tuple(10, true,  true,  USE_AVX2)] = proc_simd<__m256i, int16_t, 32, USE_AVX2, true,  true>;
    table[make_tuple(10, true,  false, USE_AVX2)] = proc_simd<__m256i, int16_t, 32, USE_AVX2, true,  false>;
    table[make_tuple(10, false, true,  USE_AVX2)] = proc_simd<__m256i, int16_t, 32, USE_AVX2, false, true>;
    table[make_tuple(10, false, false, USE_AVX2)] = proc_simd<__m256i, int16_t, 32, USE_AVX2, false, true>;

    table[make_tuple(16, true,  true,  USE_AVX2)] = proc_simd<__m256i, uint16_t, 16, USE_AVX2, true,  true>;
    table[make_tuple(16, true,  false, USE_AVX2)] = proc_simd<__m256i, uint16_t, 16, USE_AVX2, true,  false>;
    table[make_tuple(16, false, true,  USE_AVX2)] = proc_simd<__m256i, uint16_t, 16, USE_AVX2, false, true>;
    table[make_tuple(16, false, false, USE_AVX2)] = proc_simd<__m256i, uint16_t, 16, USE_AVX2, false, true>;
#endif // __AVX2__
#endif // __AVX__
#endif // __SSE4_1__
#endif // __SSSE3__
#endif // __SSE2__

    return table[make_tuple(bps, spcheck, edeint, arch)];
}

YadifMod2::YadifMod2(VSNodeRef* c, VSNodeRef* e, int o, int f, int m,
                     arch_t arch, VSCore* core, const VSAPI* api) :
    clip(c), edeint(e), order(o), field(f), mode(m)
{
    vi = *api->getVideoInfo(clip);

    nfSrc = vi.numFrames - 1;

    if (mode == 1 || mode == 3) {
        vi.numFrames *= 2;
        set_fps(vi, vi.fpsNum * 2, vi.fpsDen);
    }

    if (field == -1) {
        field = order;
    }

    prevFirst = nfSrc == 0 ? 0 : edeint ? 0 : 1;

    switch (vi.format->bytesPerSample) {
    case 1:
        interp = interpolate<uint8_t>;
        break;
    case 2:
        interp = interpolate<uint16_t>;
        break;
    default:
        interp = interpolate<float>;
    }
    
    int bps = vi.format->bitsPerSample;
    if (bps == 9) {
        bps = 10;
    }
    if (arch == NO_SIMD && bps == 10) {
        bps = 16;
    }
    if (bps == 32) {
        arch = arch < USE_AVX ? USE_SSE2 : USE_AVX;
    }

    mainProc = get_main_proc(bps, mode < 2, edeint != nullptr, arch);
}


void YadifMod2::requestFrames(int n, const VSAPI* api, VSFrameContext* ctx)
{
    if (mode == 1 || mode == 3) {
        n /= 2;
    }

    api->requestFrameFilter(n == 0 ? prevFirst : n - 1, clip, ctx);
    api->requestFrameFilter(n, clip, ctx);
    api->requestFrameFilter(std::min(n + 1, nfSrc), clip, ctx);

    if (edeint) {
        api->requestFrameFilter(n, edeint, ctx);
    }
}


const VSFrameRef* YadifMod2::
getFrame(int n, VSCore* core, const VSAPI* api, VSFrameContext* ctx)
{
    int ft = field;
    if (mode == 1 || mode == 3) {
        ft = (n & 1) ? 1 - order : order;
        n /= 2;
    }

    auto edeint = this->edeint ? api->getFrameFilter(n, this->edeint, ctx)
                : nullptr;
    auto curr = api->getFrameFilter(n, clip, ctx);
    auto prev = api->getFrameFilter(n == 0 ? prevFirst : n - 1, clip, ctx);
    auto next = api->getFrameFilter(std::min(n + 1, nfSrc), clip, ctx);
    auto dst = api->newVideoFrame(vi.format, vi.width, vi.height, curr, core);

    for (int p = 0; p < vi.format->numPlanes; ++p) {
        auto currp = api->getReadPtr(curr, p);
        auto prevp = api->getReadPtr(prev, p);
        auto nextp = api->getReadPtr(next, p);

        size_t width = api->getFrameWidth(curr, p);
        size_t rowsize = width * vi.format->bytesPerSample;
        size_t height = api->getFrameHeight(curr, p);
        int cstride = api->getStride(curr, p);
        int pstride = api->getStride(prev, p);
        int nstride = api->getStride(next, p);

        int begin = 2 + ft;
        int count = (height - 4 + ft - begin) / 2 + 1;

        const uint8_t *fm_prev, *fm_next;
        int fm_pstride, fm_nstride;
        if (ft != order) {
            fm_pstride = cstride * 2;
            fm_nstride = nstride * 2;
            fm_prev = prevp + begin + cstride;
            fm_next = nextp + begin * nstride;
        } else {
            fm_pstride = pstride * 2;
            fm_nstride = cstride * 2;
            fm_prev = prevp + begin * pstride;
            fm_next = currp + begin * cstride;
        }

        const uint8_t* edeintp = nullptr;
        int estride = 0;
        if (this->edeint) {
            edeintp = api->getReadPtr(edeint, p);
            estride = api->getStride(edeint, p);
        }

        auto dstp = api->getWritePtr(dst, p);
        int dstride = api->getStride(dst, p);

        if (ft == 0) {
            memcpy(dstp, currp + cstride, rowsize);
            if (edeintp) {
                memcpy(dstp + dstride * (height - 2),
                       edeintp + estride * (height - 2), rowsize);
            } else {
                interp(dstp + dstride * (height - 2),
                       currp + cstride * (height - 3), cstride, width);
            }
        } else {
            if (edeintp) {
                memcpy(dstp + dstride, edeintp + estride, rowsize);
            } else {
                interp(dstp + dstride, currp, cstride, width);
            }
            memcpy(dstp + dstride * (height - 1),
                   currp + cstride * (height - 2), rowsize);
        }
        bitblt(dstp + (1 - ft) * dstride, dstride * 2,
               currp + (1 - ft) * cstride, cstride * 2, rowsize, height / 2);

        mainProc(currp + begin * cstride, prevp + begin * pstride,
                 nextp + begin * nstride, fm_prev, fm_next,
                 edeintp + begin * estride, dstp + begin * dstride, width,
                 cstride, pstride, nstride, fm_pstride, fm_nstride, estride * 2,
                 dstride * 2, count);
    }

    return dst;

}
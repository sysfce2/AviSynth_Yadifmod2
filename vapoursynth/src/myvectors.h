#ifndef MY_VECTORS_H
#define MY_VECTORS_H

#include <cstdint>
#include "arch.h"

class Vm128i {
protected:
    __m128i xmm;
public:
    Vm128i(const __m128i& x)
    {
        xmm = x;
    }


};

class Vi16x8 {
protected:
    __m128i xmm;
public:
    Vi16x8(const __m128i& x) {
        xmm = x;
    }
    template <arch_t ARCH>
    Vi16x8& load(const uint8_t* p) {
        xmm = _mm_unpacklo_epi8(            _mm_loadl_epi64(reinterpret_cast<const __m128i*>(p)),            _mm_setzero_si128());        return *this;
    }
    template <arch_t ARCH>
    Vi16x8& loadu(const uint8_t* p) {
        return load<ARCH>(p);
    }
#if defined(__SSE4_1__)
    template <>
    Vi16x8& load<USE_SSE41>(const uint8_t* p) {
        xmm = _mm_cvtepu8_epi16(
            _mm_loadl_epi64(reinterpret_cast<const __m128i*>(p)));
        return *this;
    }
#endif

    void store(uint8_t* p) {
        _mm_storel_epi64(reinterpret_cast<__m128i*>(p), _mm_packus_epi16(xmm, xmm));
    }

};
#endif


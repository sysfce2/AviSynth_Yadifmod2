#ifndef YADIF_MOD2_ARCH_H
#define YADIF_MOD2_ARCH_H


typedef enum {
    NO_SIMD,
    USE_SSE2,
    USE_SSSE3,
    USE_AVX2,
} arch_t;


extern int has_sse2(void);
extern int has_ssse3(void);
extern int has_avx2(void);


static arch_t get_arch(int opt) noexcept
{
    if (opt == 0 || !has_sse2()) {
        return NO_SIMD;
    }
    if (opt == 1 || !has_ssse3()) {
        return USE_SSE2;
    }
#if !defined(__AVX2__)
    return USE_SSSE3;
#else
    if (opt == 2 || !has_avx2()) {
        return USE_SSSE3;
    }
    return USE_AVX2;
#endif
}

#endif


/*
 * AC792 backend for floating-point Opus.
 *
 * Uses the SDK's shared hardware FFT driver. The upstream software FFT remains
 * available for unsupported lengths, unusual float inputs, and failed checks.
 * PI32V2 rep loops follow the SDK kernels' instruction conventions, adapted to
 * floating-point Opus rather than copying the vendor's fixed-point SILK code.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "opus_pi32v2.h"
#include "opus_pi32v2_overrides.h"

#ifndef OPUS_JIELI_HW_FFT
#define OPUS_JIELI_HW_FFT 0
#endif
#ifndef OPUS_JIELI_PI32V2_ASM
#define OPUS_JIELI_PI32V2_ASM 0
#endif
#ifdef FIXED_POINT
#error "The AC792 backend is for floating-point Opus; do not mix fixed-point kernels."
#endif
#if OPUS_JIELI_PI32V2_ASM && !defined(OPUS_JIELI_PORTABLE_TEST) && !defined(__GCC_PI32V2__)
#error "PI32V2 assembly requires the Jieli toolchain."
#endif

static int backend_mask;
static int initialized;

static float scalar_inner_prod(const float *x, const float *y, int n)
{
    float sum = 0.0f;
    int i;
    for (i = 0; i < n; ++i) {
        sum += x[i] * y[i];
    }
    return sum;
}

float opus_pi32v2_inner_prod(const float *x, const float *y, int n)
{
#if OPUS_JIELI_PI32V2_ASM && !defined(OPUS_JIELI_PORTABLE_TEST)
    if ((backend_mask & OPUS_PI32V2_ASM) && n > 0) {
        float sum = 0.0f, a, b;
        __asm__ volatile(
            "1:\n\t"
            "rep %[n] {\n\t"
            "%[a] = [%[x]++=4]\n\t"
            "%[b] = [%[y]++=4]\n\t"
            "%[a] = %[a] * %[b] (f)\n\t"
            "%[sum] = %[sum] + %[a] (f)\n\t"
            "}\n\t"
            "if(%[n] != 0) goto 1b\n\t"
            : [sum] "+&r"(sum), [x] "+&r"(x), [y] "+&r"(y), [n] "+&r"(n),
              [a] "=&r"(a), [b] "=&r"(b)
            :
            : "memory");
        return sum;
    }
#endif
    return scalar_inner_prod(x, y, n);
}

void opus_pi32v2_dual_inner_prod(const float *x, const float *y1,
                               const float *y2, int n, float *sum1, float *sum2)
{
    float s1 = 0.0f, s2 = 0.0f;
#if OPUS_JIELI_PI32V2_ASM && !defined(OPUS_JIELI_PORTABLE_TEST)
    if ((backend_mask & OPUS_PI32V2_ASM) && n > 0) {
        float a, b, product;
        __asm__ volatile(
            "1:\n\t"
            "rep %[n] {\n\t"
            "%[a] = [%[x]++=4]\n\t"
            "%[b] = [%[y1]++=4]\n\t"
            "%[product] = %[a] * %[b] (f)\n\t"
            "%[s1] = %[s1] + %[product] (f)\n\t"
            "%[b] = [%[y2]++=4]\n\t"
            "%[product] = %[a] * %[b] (f)\n\t"
            "%[s2] = %[s2] + %[product] (f)\n\t"
            "}\n\t"
            "if(%[n] != 0) goto 1b\n\t"
            : [s1] "+&r"(s1), [s2] "+&r"(s2), [x] "+&r"(x),
              [y1] "+&r"(y1), [y2] "+&r"(y2), [n] "+&r"(n),
              [a] "=&r"(a), [b] "=&r"(b), [product] "=&r"(product)
            :
            : "memory");
    } else
#endif
    {
        int i;
        for (i = 0; i < n; ++i) {
            s1 += x[i] * y1[i];
            s2 += x[i] * y2[i];
        }
    }
    *sum1 = s1;
    *sum2 = s2;
}

void *opus_pi32v2_copy(void *dst, const void *src, size_t bytes)
{
#if OPUS_JIELI_PI32V2_ASM && !defined(OPUS_JIELI_PORTABLE_TEST)
    if ((backend_mask & OPUS_PI32V2_ASM) && bytes >= 16 &&
        !(((uintptr_t)dst | (uintptr_t)src) & 3) && (bytes >> 2) <= 0x7fffffffU) {
        unsigned int *d = (unsigned int *)dst;
        const unsigned int *s = (const unsigned int *)src;
        int count = (int)(bytes >> 2);
        unsigned int value;
        __asm__ volatile(
            "1:\n\t"
            "rep %[count] {\n\t"
            "%[value] = [%[src]++=4]\n\t"
            "[%[dst]++=4] = %[value]\n\t"
            "}\n\t"
            "if(%[count] != 0) goto 1b\n\t"
            : [dst] "+&r"(d), [src] "+&r"(s), [count] "+&r"(count),
              [value] "=&r"(value)
            :
            : "memory");
        if (bytes & 3) {
            memcpy(d, s, bytes & 3);
        }
        return dst;
    }
#endif
    return memcpy(dst, src, bytes);
}

void opus_pi32v2_clear(void *dst, size_t bytes)
{
#if OPUS_JIELI_PI32V2_ASM && !defined(OPUS_JIELI_PORTABLE_TEST)
    if ((backend_mask & OPUS_PI32V2_ASM) && bytes >= 16 &&
        !((uintptr_t)dst & 3) && (bytes >> 2) <= 0x7fffffffU) {
        unsigned int *d = (unsigned int *)dst;
        int count = (int)(bytes >> 2);
        unsigned int zero = 0;
        __asm__ volatile(
            "1:\n\t"
            "rep %[count] {\n\t"
            "[%[dst]++=4] = %[zero]\n\t"
            "}\n\t"
            "if(%[count] != 0) goto 1b\n\t"
            : [dst] "+&r"(d), [count] "+&r"(count)
            : [zero] "r"(zero)
            : "memory");
        if (bytes & 3) {
            memset(d, 0, bytes & 3);
        }
        return;
    }
#endif
    memset(dst, 0, bytes);
}

static float magnitude(float value)
{
    return value < 0.0f ? -value : value;
}

#if OPUS_JIELI_PI32V2_ASM
static int check_asm(void)
{
    float x[64], y[64], z[64];
    unsigned char src[96] __attribute__((aligned(4)));
    unsigned char actual[96] __attribute__((aligned(4)));
    unsigned char expected[96] __attribute__((aligned(4)));
    int i, n, offset;
    for (i = 0; i < 64; ++i) {
        x[i] = (i - 31) * 0.125f;
        y[i] = ((i * 13) % 17 - 8) * 0.25f;
        z[i] = ((i * 7) % 23 - 11) * 0.0625f;
    }
    for (n = -1; n <= 64; ++n) {
        float r1 = scalar_inner_prod(x, y, n);
        float r2 = scalar_inner_prod(x, z, n);
        float a = opus_pi32v2_inner_prod(x, y, n), b, c;
        opus_pi32v2_dual_inner_prod(x, y, z, n, &b, &c);
        if (!(magnitude(a - r1) <= (1.0f + magnitude(r1)) * 1e-6f) ||
            !(magnitude(b - r1) <= (1.0f + magnitude(r1)) * 1e-6f) ||
            !(magnitude(c - r2) <= (1.0f + magnitude(r2)) * 1e-6f)) {
            return 0;
        }
    }
    for (i = 0; i < 96; ++i) {
        src[i] = (unsigned char)(i * 17 + 3);
    }
    for (offset = 0; offset < 4; ++offset) {
        for (n = 0; n <= 80; ++n) {
            memset(actual, 0xa5, sizeof(actual));
            memset(expected, 0xa5, sizeof(expected));
            memcpy(expected + offset, src + offset, (size_t)n);
            opus_pi32v2_copy(actual + offset, src + offset, (size_t)n);
            if (memcmp(actual, expected, sizeof(actual))) {
                return 0;
            }
            memset(expected + offset, 0, (size_t)n);
            opus_pi32v2_clear(actual + offset, (size_t)n);
            if (memcmp(actual, expected, sizeof(actual))) {
                return 0;
            }
        }
    }
    return 1;
}
#endif

#if OPUS_JIELI_HW_FFT
#include "kiss_fft.h"
#include "modes.h"
#include "opus_custom.h"
#include "hw_fft_f32.h"

void opus_fft_impl_c(const kiss_fft_state *st, kiss_fft_cpx *data);

typedef struct {
    int n;
    unsigned int config;
    int input_exponent;
    float correction;
} opus_fft_backend;

/* SDK fftvec_math_drv.h forward complex configurations, with separate input
 * and output SRAM. These are the FFT_EXT configurations, not FFT_EXT_V2.
 *
 * Conservative input headroom: max(|component|) < 2^(25-ceil(log2(n))+1).
 * This stays below the SDK's documented complex FFT input limits and reserves
 * headroom for butterflies. The post-scale is checked against the software FFT.
 */
static opus_fft_backend fft_backends[4] = {
    {60,  0x203c3200U, 19, 1.0f},
    {120, 0x20783300U, 18, 1.0f},
    {240, 0x20f03410U, 17, 1.0f},
    {480, 0x21e03520U, 16, 1.0f}
};

static int fft_index(int n)
{
    int i;
    for (i = 0; i < 4; ++i) {
        if (fft_backends[i].n == n) {
            return i;
        }
    }
    return -1;
}

static int run_fft(const kiss_fft_state *st, kiss_fft_cpx *data, int index,
                   float correction)
{
    union { float f; uint32_t u; } value, up, down;
    uint32_t maximum = 0;
    int i, exponent, shift;
    for (i = 0; i < st->nfft; ++i) {
        uint32_t bits;
        value.f = data[i].r;
        bits = value.u & 0x7fffffffU;
        if (bits > maximum) maximum = bits;
        value.f = data[i].i;
        bits = value.u & 0x7fffffffU;
        if (bits > maximum) maximum = bits;
    }
    if (maximum == 0) {
        memset(data, 0, (size_t)st->nfft * sizeof(*data));
        return 1;
    }
    /* No float->int conversion of NaNs, infinities, or subnormal-only blocks.
     * Keep both powers of two normal so scaling cannot create Inf * 0. */
    if (maximum >= 0x7f800000U || maximum < 0x00800000U) return 0;
    exponent = (int)(maximum >> 23) - 127;
    shift = fft_backends[index].input_exponent - exponent;
    if (shift < -126 || shift > 126) return 0;
    up.u = (uint32_t)(127 + shift) << 23;
    down.u = (uint32_t)(127 - shift) << 23;
    return hw_fft_run_f32(fft_backends[index].config, (const float *)data,
                         (float *)data, st->bitrev, up.f, down.f * correction);
}

/* Preserve the bit-reversed input on failure: the software fallback consumes
 * exactly the same data. This also covers backward MDCT and analysis FFT calls.
 */
void opus_fft_impl(const kiss_fft_state *st, kiss_fft_cpx *data)
{
    int index = fft_index(st->nfft);
    if (index >= 0 && (backend_mask & (1 << index)) &&
        run_fft(st, data, index, fft_backends[index].correction)) {
        return;
    }
    opus_fft_impl_c(st, data);
}

static void test_input(const kiss_fft_state *st, kiss_fft_cpx *data, int pattern)
{
    uint32_t random = 1;
    float amplitude = pattern == 3 ? 1e-12f : pattern == 4 ? 32768.0f : 1.0f;
    int i;
    for (i = 0; i < st->nfft; ++i) {
        kiss_fft_cpx sample;
        random = random * 1664525U + 1013904223U;
        sample.r = ((int)((random >> 8) & 65535U) - 32768) * (1.0f / 32768.0f);
        random = random * 1664525U + 1013904223U;
        sample.i = ((int)((random >> 8) & 65535U) - 32768) * (1.0f / 32768.0f);
        if (pattern == 1) {
            sample.r = i == 1 ? 1.0f : 0.0f;
            sample.i = i == 1 ? -0.5f : 0.0f;
        } else if (pattern == 2) {
            sample.r = 0.875f;
            sample.i = -0.625f;
        }
        sample.r *= amplitude;
        sample.i *= amplitude;
        data[st->bitrev[i]] = sample;
    }
}

static int check_fft(const kiss_fft_state *st, int index,
                     kiss_fft_cpx *actual, kiss_fft_cpx *reference)
{
    int pattern, i;
    float correction = 1.0f;
    for (pattern = 0; pattern < 5; ++pattern) {
        float cross = 0.0f, actual_energy = 0.0f, reference_energy = 0.0f;
        float error_energy = 0.0f;
        test_input(st, actual, pattern);
        memcpy(reference, actual, (size_t)st->nfft * sizeof(*actual));
        opus_fft_impl_c(st, reference);
        if (!run_fft(st, actual, index, 1.0f)) return 0;
        if (pattern == 0) {
            for (i = 0; i < st->nfft; ++i) {
                cross += actual[i].r * reference[i].r + actual[i].i * reference[i].i;
                actual_energy += actual[i].r * actual[i].r + actual[i].i * actual[i].i;
            }
            correction = cross / actual_energy;
            if (!(correction >= 1.0f / 4096.0f && correction <= 4096.0f)) return 0;
        }
        for (i = 0; i < st->nfft; ++i) {
            float er = actual[i].r * correction - reference[i].r;
            float ei = actual[i].i * correction - reference[i].i;
            error_energy += er * er + ei * ei;
            reference_energy += reference[i].r * reference[i].r + reference[i].i * reference[i].i;
        }
        /* Reject wrong ordering, sign, saturation, or poor numerical accuracy.
         * 1e-7 relative error energy corresponds to at least 70 dB test SNR. */
        if (!(error_energy <= reference_energy * 1e-7f)) return 0;
    }
    fft_backends[index].correction = correction;
    return 1;
}
#endif

int opus_pi32v2_init(void)
{
    if (initialized) return backend_mask;
#if OPUS_JIELI_PI32V2_ASM
    backend_mask |= OPUS_PI32V2_ASM;
    if (!check_asm()) backend_mask &= ~OPUS_PI32V2_ASM;
#endif
#if OPUS_JIELI_HW_FFT
    {
        int error = 0, shift;
        const CELTMode *mode = opus_custom_mode_create(48000, 960, &error);
        kiss_fft_cpx *work = (kiss_fft_cpx *)malloc(2 * 480 * sizeof(*work));
        if (mode && !error && work) {
            for (shift = 0; shift <= mode->mdct.maxshift; ++shift) {
                const kiss_fft_state *st = mode->mdct.kfft[shift];
                int index = fft_index(st->nfft);
                if (index >= 0 && check_fft(st, index, work, work + 480)) {
                    backend_mask |= 1 << index;
                }
            }
        }
        free(work);
#ifdef CUSTOM_MODES
        if (mode) opus_custom_mode_destroy((OpusCustomMode *)mode);
#endif
    }
#endif
    initialized = 1;
    return backend_mask;
}

int opus_pi32v2_backends(void)
{
    return backend_mask;
}

#ifdef OPUS_JIELI_PORTABLE_TEST
/* Test builds only: compare independent codec states with identical upstream
 * code and different FFT dispatch. Not exported by the firmware library. */
int opus_pi32v2_test_backends(int mask)
{
    int previous = backend_mask;
    backend_mask = mask;
    return previous;
}
#endif

#ifndef OPUS_PI32V2_OVERRIDES_H
#define OPUS_PI32V2_OVERRIDES_H

#include <stddef.h>

/* The containing CMake project deliberately builds floating-point Opus. These
 * declarations use float so this header can precede Opus's own type headers. */
float opus_pi32v2_inner_prod(const float *x, const float *y, int n);
void opus_pi32v2_dual_inner_prod(const float *x, const float *y1,
                               const float *y2, int n, float *sum1, float *sum2);
void *opus_pi32v2_copy(void *dst, const void *src, size_t bytes);
void opus_pi32v2_clear(void *dst, size_t bytes);

#define OVERRIDE_CELT_INNER_PROD
#define celt_inner_prod(x, y, n, arch) \
    ((void)(arch), opus_pi32v2_inner_prod((x), (y), (n)))
#define OVERRIDE_DUAL_INNER_PROD
#define dual_inner_prod(x, y1, y2, n, sum1, sum2, arch) \
    ((void)(arch), opus_pi32v2_dual_inner_prod((x), (y1), (y2), (n), (sum1), (sum2)))

#endif

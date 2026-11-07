#ifndef OPUS_PI32V2_CUSTOM_SUPPORT_H
#define OPUS_PI32V2_CUSTOM_SUPPORT_H

#include "opus_pi32v2_overrides.h"

#define OVERRIDE_OPUS_COPY
#define OPUS_COPY(dst, src, n) \
    opus_pi32v2_copy((dst), (src), (n) * sizeof(*(dst)) + 0 * ((dst) - (src)))
#define OVERRIDE_OPUS_CLEAR
#define OPUS_CLEAR(dst, n) opus_pi32v2_clear((dst), (n) * sizeof(*(dst)))

#endif

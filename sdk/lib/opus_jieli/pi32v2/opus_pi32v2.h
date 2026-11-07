#ifndef OPUS_PI32V2_PORT_H
#define OPUS_PI32V2_PORT_H

/* Call once during startup, before creating codec tasks. No standard Opus API
 * or bitstream format changes. A failing check leaves that backend disabled. */
#define OPUS_PI32V2_FFT_60   (1 << 0)
#define OPUS_PI32V2_FFT_120  (1 << 1)
#define OPUS_PI32V2_FFT_240  (1 << 2)
#define OPUS_PI32V2_FFT_480  (1 << 3)
#define OPUS_PI32V2_ASM      (1 << 8)

int opus_pi32v2_init(void);
int opus_pi32v2_backends(void);

#endif

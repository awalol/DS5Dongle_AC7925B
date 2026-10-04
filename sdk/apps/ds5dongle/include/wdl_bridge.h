#ifndef DS5_WDL_BRIDGE_H
#define DS5_WDL_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wdl_resampler wdl_resampler_t;

/* Input-driven linear interpolation, with no low-pass or sinc filter.
 * All frame counts are per channel; samples are interleaved floats.
 * Returns NULL on invalid parameters or allocation failure. The caller owns
 * the handle and must serialize access to it.
 */
wdl_resampler_t* wdl_resampler_create(double input_rate, double output_rate,
                                     int channels, int input_frames, int output_frames);
void wdl_resampler_destroy(wdl_resampler_t* resampler);

/* Returns writable input frames and a borrowed buffer owned by the handle.
 * Fill at most the returned frames before calling out. The buffer is valid
 * until the next prepare/out/destroy call. Returns 0 on failure.
 */
int wdl_resampler_prepare(wdl_resampler_t* resampler, int frames, float** input);

/* Converts the prepared input and returns the number of output frames.
 * output must hold output_frames * channels floats. A short input count
 * flushes WDL's buffered tail; use the full prepared count for streaming.
 */
int wdl_resampler_out(wdl_resampler_t* resampler, float* output,
                      int input_frames, int output_frames);

#ifdef __cplusplus
}
#endif

#endif

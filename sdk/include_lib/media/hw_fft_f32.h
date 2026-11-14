#ifndef JL_HW_FFT_F32_H
#define JL_HW_FFT_F32_H

/* Forward complex FFT using the existing, shared FFT engine lock and SRAM.
 * input_order maps natural complex indices to indices in the input; NULL means
 * natural order. Scales must be finite, and the caller must bound the converted
 * integer input for the selected hardware configuration.
 *
 * Returns 1 on success, 0 without changing out on invalid input or timeout.
 * in == out is supported. Intended for task context, like hw_fft_run().
 */
int hw_fft_run_f32(unsigned int config, const float *in, float *out,
                   const short *input_order, float scale, float inverse_scale);

#endif

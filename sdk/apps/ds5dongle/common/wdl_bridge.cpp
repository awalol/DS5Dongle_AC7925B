#include "wdl_bridge.h"

#include <new>

// Keep WDL's C++ headers separate from the SDK's C headers and logging macros.
// Must match lib/wdl_jieli's public sample ABI.
#define WDL_RESAMPLE_TYPE float
#include "resample.h"

struct wdl_resampler
{
    WDL_Resampler instance;
    int channels;
};

wdl_resampler_t* wdl_resampler_create(double input_rate, double output_rate,
                                     int channels, int input_frames, int output_frames)
{
    if (!(input_rate > 0) || !(output_rate > 0) || channels <= 0 ||
        input_frames <= 0 || output_frames <= 0)
    {
        return NULL;
    }

    // The SDK's ordinary operator new spins on OOM. Allocate explicitly so
    // the C caller can handle failure, and construct only when init is called.
    void* memory = malloc(sizeof(wdl_resampler_t));
    if (!memory)
    {
        return NULL;
    }
    wdl_resampler_t* resampler = new (memory) wdl_resampler_t;
    resampler->channels = channels;
    resampler->instance.Reset();
    resampler->instance.SetMode(true, 0, false);
    resampler->instance.SetRates(input_rate, output_rate);
    resampler->instance.SetFeedMode(true);
    resampler->instance.Prealloc(channels, input_frames, output_frames);

    // Prealloc has no result; Prepare verifies the input allocation without
    // feeding samples. The caller's first Prepare safely replaces this request.
    float* input = NULL;
    int frames = resampler->instance.ResamplePrepare(input_frames, channels, &input);
    if (frames != input_frames || !input)
    {
        wdl_resampler_destroy(resampler);
        return NULL;
    }
    return resampler;
}

void wdl_resampler_destroy(wdl_resampler_t* resampler)
{
    if (resampler)
    {
        resampler->~wdl_resampler();
        free(resampler);
    }
}

int wdl_resampler_prepare(wdl_resampler_t* resampler, int frames, float** input)
{
    if (!input)
    {
        return 0;
    }
    *input = NULL;
    if (!resampler || frames <= 0)
    {
        return 0;
    }
    return resampler->instance.ResamplePrepare(frames, resampler->channels, input);
}

int wdl_resampler_out(wdl_resampler_t* resampler, float* output,
                      int input_frames, int output_frames)
{
    if (!resampler || !output || input_frames < 0 || output_frames <= 0)
    {
        return 0;
    }
    return resampler->instance.ResampleOut(output, input_frames, output_frames,
                                          resampler->channels);
}

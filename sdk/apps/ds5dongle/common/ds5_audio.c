//
// Created by awalol on 2026/10/1.
//
#define LOG_TAG "[DS5_AUDIO]"
#define LOG_ERROR_ENABLE
#define LOG_WARN_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DEBUG_ENABLE
#include "debug.h"
#include "ds5_audio.h"
#include "circular_buf.h"
#include "ds5_report.h"
#include "os_api.h"
#include "queue.h"
#include "task.h"
#include "wdl_bridge.h"
#include "opus.h"

#define UAC_CBUF_SIZE (1 * 1024)
#define UAC_CHANNELS 4
#define UAC_FRAME_BYTES (UAC_CHANNELS * sizeof(s16))
#define UAC_INPUT_FRAMES 48
#define HAPTICS_BUF_SIZE 64
#define HAPTICS_CHANNELS 2

#define HAPTICS_OUTPUT_FRAMES 4
#define HAPTICS_QUEUE_DEPTH 2
#define HAPTICS_GAIN 1.0f

static wdl_resampler_t* haptics_resampler;
static wdl_resampler_t* spk_resampler;
static OpusEncoder *opus_enc;
static cbuffer_t uac_cbuf;
static u8 uac_buf[UAC_CBUF_SIZE];
static OS_SEM uac_sem;
static queue_t haptics_queue;
static volatile bool task_created = false;

void ds5_audio_rx(u8* data, u32 size)
{
    // keep 384

    if (!task_created)
    {
        return;
    }

    if (cbuf_is_write_able(&uac_cbuf, size) < size)
    {
        return;
    }
    if (cbuf_write(&uac_cbuf, data, size) == size)
    {
        os_sem_post(&uac_sem);
    }
}

static void ds5_audio_task(void* priv)
{
    s16 buffer[UAC_INPUT_FRAMES * UAC_CHANNELS];
    float haptics_resample_buf[HAPTICS_OUTPUT_FRAMES * HAPTICS_CHANNELS];
    s8 haptics_buf[HAPTICS_BUF_SIZE];
    u8 haptics_buf_pos = 0;

    while (1)
    {
        os_sem_set(&uac_sem, 0);
        u32 frames = cbuf_get_data_len(&uac_cbuf) / UAC_FRAME_BYTES;
        if (!frames)
        {
            os_sem_pend(&uac_sem, 0);
            continue;
        }
        if (frames > UAC_INPUT_FRAMES)
        {
            frames = UAC_INPUT_FRAMES;
        }

        float* in_buf = NULL;
        // nframes keep 48
        int nframes = wdl_resampler_prepare(haptics_resampler, frames, &in_buf);
        if (nframes <= 0 || !in_buf)
        {
            log_error("haptics resampler prepare failed\n");
            os_time_dly(1);
            continue;
        }
        nframes = cbuf_read(&uac_cbuf, buffer, nframes * UAC_FRAME_BYTES) / UAC_FRAME_BYTES;
        if (!nframes)
        {
            continue;
        }

        // split channel
        for (int i = 0; i < nframes; i++)
        {
            in_buf[i * 2] = buffer[i * UAC_CHANNELS + 2] / 32768.0f;
            in_buf[i * 2 + 1] = buffer[i * UAC_CHANNELS + 3] / 32768.0f;
        }

        // haptics proc
        int out_frames = wdl_resampler_out(haptics_resampler, haptics_resample_buf, nframes,
                                           HAPTICS_OUTPUT_FRAMES);
        for (int i = 0; i < out_frames * HAPTICS_CHANNELS; i++)
        {
            float sample = haptics_resample_buf[i] * 127.0f * HAPTICS_GAIN;
            // clamp
            if (sample > 127.0f)
            {
                sample = 127.0f;
            }
            else if (sample < -128.0f)
            {
                sample = -128.0f;
            }

            haptics_buf[haptics_buf_pos++] = (s8)sample;

            if (haptics_buf_pos == HAPTICS_BUF_SIZE)
            {
                /*if (!queue_push(&haptics_queue, haptics_buf))
                {
                    queue_pop(&haptics_queue, NULL);
                    queue_push(&haptics_queue, haptics_buf);
                }*/
                haptics_buf_pos = 0;
                ds5_report_send_haptics(haptics_buf);
            }
        }
    }
}

void ds5_audio_init(void)
{
    if (task_created)
    {
        return;
    }
    //TODO: 以后换成杰理的 SRC
    haptics_resampler = wdl_resampler_create(48000, 3000, HAPTICS_CHANNELS,
                                            UAC_INPUT_FRAMES, HAPTICS_OUTPUT_FRAMES);
    spk_resampler = wdl_resampler_create(51200, 48000, HAPTICS_CHANNELS,
                                        UAC_INPUT_FRAMES, HAPTICS_OUTPUT_FRAMES);
    if (!haptics_resampler || spk_resampler)
    {
        log_error("haptics resampler init failed");
    }

    /*
    int opus_err = 0;
    opus_enc = opus_encoder_create(48000,2,OPUS_APPLICATION_RESTRICTED_CELT,&opus_err);
    if (opus_err != 0)
    {
        log_error("opus encoder create failed: %d", opus_err);
    }
    opus_encoder_ctl(opus_enc,OPUS_SET_EXPERT_FRAME_DURATION(OPUS_FRAMESIZE_10_MS));
    opus_encoder_ctl(opus_enc,OPUS_SET_BITRATE(200 * 8 * 100));
    opus_encoder_ctl(opus_enc,OPUS_SET_VBR(false));
    opus_encoder_ctl(opus_enc,OPUS_SET_COMPLEXITY(0)); // max 4
    */

    bool queue_created = queue_init(&haptics_queue, HAPTICS_BUF_SIZE, HAPTICS_QUEUE_DEPTH);
    if (!queue_created)
    {
        log_error("haptics queue init failed");
    }

    int sem_err = os_sem_create(&uac_sem, 0);
    if (sem_err)
    {
        log_error("audio semaphore create failed: %d", sem_err);
    }

    cbuf_init(&uac_cbuf, uac_buf, sizeof(uac_buf));

    int task_err = task_create(ds5_audio_task, NULL, "audio");
    if (task_err)
    {
        log_error("audio task create failed: %d", task_err);
    }

    if (!haptics_resampler || !spk_resampler || !queue_created || sem_err || task_err) // || opus_err
    {
        os_sem_del(&uac_sem, OS_DEL_ALWAYS);
        free(haptics_queue.data);
        haptics_queue.data = NULL;
        wdl_resampler_destroy(haptics_resampler);
        haptics_resampler = NULL;
        opus_enc = NULL;
        return;
    }

    task_created = true;
    log_info("audio task created");
}

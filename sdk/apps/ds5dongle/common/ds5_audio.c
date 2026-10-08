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
#include "asm/src.h"
#include "opus.h"

#define UAC_CBUF_SIZE (1 * 1024)
#define UAC_CHANNELS 4
#define UAC_FRAME_BYTES (UAC_CHANNELS * sizeof(s16))
#define UAC_INPUT_FRAMES 48

#define HAPTICS_BUF_SIZE 64
#define HAPTICS_CHANNELS 2

#define SPEAKER_BUF_SIZE (480 * 2)
#define SPEAKER_OPUS_SIZE 200
#define SPEAKER_CHANNELS 2
#define SPEAKER_PCM_QUEUE_DEPTH 2

#define HAPTICS_GAIN 2.0f

#define PACKET_MODE 1

/* 杰理 SRC 驱动引用的全局配置：完整 SDK 由 sdk/audio/log_config/lib_media_config.c
   提供，本工程没有编译该文件。数值是驱动内部输入缓冲的每通道帧数。*/
const int config_resample_pcm_frame_buffer = 256;

static cbuffer_t uac_cbuf;
static u8 uac_buf[UAC_CBUF_SIZE];
static OS_SEM uac_sem;
static OS_SEM opus_sem;
static volatile bool task_created = false;

static void* haptics_src;
static s16 haptics_src_in[UAC_INPUT_FRAMES * HAPTICS_CHANNELS];
static s8 haptics_buf[HAPTICS_BUF_SIZE];
static u8 haptics_buf_pos;
static u32 haptics_src_out_cnt;

static void* speaker_src;
static s16 speaker_src_in[UAC_INPUT_FRAMES * HAPTICS_CHANNELS];
// SRC
static s16 speaker_buf[SPEAKER_BUF_SIZE];
static u16 speaker_buf_pos;
static u32 speaker_src_out_cnt;
static OpusEncoder *opus_enc;
static queue_t spk_pcm_queue;
typedef struct
{
    s16 data[SPEAKER_BUF_SIZE];
} pcm_element;

static struct
{
    bool fresh;
    s8 data[64];
} haptics_slot[2];

static struct
{
    bool fresh;
    u8 data[200];
} speaker_slot[2];

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

static void ds5_bt_audio_send()
{
    if (!haptics_slot[1].fresh || !speaker_slot[1].fresh)
    {
        return;
    }
    haptics_slot[1].fresh = false;
    speaker_slot[1].fresh = false;
    ds5_report_send_state_speaker_haptics(speaker_slot[1].data, haptics_slot[1].data);
}

/* 硬件 SRC 的输出回调：在调用 src_base_write / src_base_push_data_out 的任务
   上下文里被同步调用，必须返回本次消费的全部字节数。*/
static int haptics_src_output(void* priv, void* data, int len)
{
    s16* pcm = (s16*)data;
    int samples = len / sizeof(s16); // 交织的 2 声道

    if (haptics_src_out_cnt < 4)
    {
        haptics_src_out_cnt++;
        log_info("haptics src out: %d bytes", len);
    }

    for (int i = 0; i < samples; i++)
    {
        float sample = pcm[i] * (127.0f * HAPTICS_GAIN / 32768.0f);
        int v = (int)sample;
        if (v > 127)
        {
            v = 127;
        }
        else if (v < -128)
        {
            v = -128;
        }

        haptics_buf[haptics_buf_pos++] = (s8)v;

        if (haptics_buf_pos == HAPTICS_BUF_SIZE)
        {
            haptics_buf_pos = 0;
            if (PACKET_MODE == 1)
            {
                memcpy(haptics_slot[1].data,haptics_buf,HAPTICS_BUF_SIZE);
                haptics_slot[1].fresh = true;
                ds5_bt_audio_send();
            }
        }
    }

    return len;
}

static int speaker_src_output(void* priv, void* data, int len)
{
    s16* pcm = (s16*)data;
    int samples = len / sizeof(s16); // 交织的 2 声道

    if (speaker_src_out_cnt < 4)
    {
        speaker_src_out_cnt++;
        log_info("speaker src out: %d bytes", len);
    }

    for (int i = 0; i < samples; i++)
    {
        speaker_buf[speaker_buf_pos++] = pcm[i];

        if (speaker_buf_pos == SPEAKER_BUF_SIZE)
        {
            speaker_buf_pos = 0;
            pcm_element element;
            memcpy(element.data,speaker_buf,SPEAKER_BUF_SIZE * sizeof(s16));
            if (queue_count(&spk_pcm_queue) == SPEAKER_PCM_QUEUE_DEPTH)
            {
                queue_pop(&spk_pcm_queue, NULL);
            }
            queue_push(&spk_pcm_queue, &element);
            os_sem_post(&opus_sem);
        }
    }

    return len;
}

static void ds5_audio_task(void* priv)
{
    s16 buffer[UAC_INPUT_FRAMES * UAC_CHANNELS];

    while (1)
    {
        os_sem_set(&uac_sem, 0);
        u32 frames = cbuf_get_data_len(&uac_cbuf) / UAC_FRAME_BYTES;
        if (frames < UAC_INPUT_FRAMES)
        {
            os_sem_pend(&uac_sem, 0);
            continue;
        }

        int rlen = cbuf_read(&uac_cbuf, buffer, UAC_INPUT_FRAMES * UAC_FRAME_BYTES) / UAC_FRAME_BYTES;
        if (rlen != UAC_INPUT_FRAMES)
        {
            continue;
        }

        // 取出第 3、4 声道（haptics），整理成连续的 2 声道数据
        for (int i = 0; i < UAC_INPUT_FRAMES; i++)
        {
            speaker_src_in[i * 2] = buffer[i * UAC_CHANNELS];
            speaker_src_in[i * 2 + 1] = buffer[i * UAC_CHANNELS + 1];
            haptics_src_in[i * 2] = buffer[i * UAC_CHANNELS + 2];
            haptics_src_in[i * 2 + 1] = buffer[i * UAC_CHANNELS + 3];
        }

        // 48kHz -> 3kHz，杰理硬件 SRC；转换结果在 haptics_src_output 回调里给出
        int h_wlen = src_base_write(haptics_src, haptics_src_in, sizeof(haptics_src_in));
        if (h_wlen != (int)sizeof(haptics_src_in))
        {
            log_error("haptics src write: %d/%d", h_wlen, (int)sizeof(haptics_src_in));
        }
        int h_pret = src_base_push_data_out(haptics_src);
        if (h_pret)
        {
            log_error("haptics src push: %d", h_pret);
        }

        // 51.2KHz -> 48KHz
        int s_wlen = src_base_write(speaker_src, speaker_src_in, sizeof(speaker_src_in));
        if (s_wlen != (int)sizeof(speaker_src_in))
        {
            log_error("speaker src write: %d/%d", s_wlen, (int)sizeof(speaker_src_in));
        }
        int s_pret = src_base_push_data_out(speaker_src);
        if (s_pret)
        {
            log_error("speaker src push: %d", s_pret);
        }
    }
}

static void ds5_audio_opus_task(void* priv)
{
    while (1)
    {
        os_sem_set(&opus_sem, 0);
        if (queue_count(&spk_pcm_queue) == 0)
        {
            os_sem_pend(&opus_sem, 0);
            continue;
        }
        pcm_element element;
        if (!queue_pop(&spk_pcm_queue, &element))
        {
            log_error("spk_pcm_queue pop failed");
            continue;
        }

        u8 opus_data[SPEAKER_OPUS_SIZE];
        int encoded_len = opus_encode(opus_enc,element.data,480,opus_data,SPEAKER_OPUS_SIZE);
        if (encoded_len != 200)
        {
            log_error("opus encode len mismatch: %d", encoded_len);
            continue;
        }
        if (PACKET_MODE == 1)
        {
            memcpy(speaker_slot[1].data,opus_data,SPEAKER_OPUS_SIZE);
            speaker_slot[1].fresh = true;
            ds5_bt_audio_send();
        }
    }
}

void ds5_audio_init(void)
{
    if (task_created)
    {
        return;
    }
    /* haptics: 48kHz -> 3kHz，换成杰理硬件 SRC（src_v2）。*/
    haptics_src = src_base_open(HAPTICS_CHANNELS, 48000, 3000, AUDIO_ONLY_RESAMPLE);
    if (!haptics_src)
    {
        log_error("haptics hw src open failed");
    }
    else
    {
        src_base_set_output_handler(haptics_src, NULL, haptics_src_output);
        log_info("haptics hw src opened");
    }

    speaker_src = src_base_open(SPEAKER_CHANNELS, 51200, 48000, AUDIO_ONLY_RESAMPLE);
    if (!speaker_src)
    {
        log_error("speaker hw src open failed");
    }else
    {
        src_base_set_output_handler(speaker_src, NULL, speaker_src_output);
        log_info("speaker hw src opened");
    }
    int opus_err = 0;
    opus_enc = opus_encoder_create(48000,2,OPUS_APPLICATION_RESTRICTED_CELT,&opus_err);
    if (opus_err != 0)
    {
        log_error("opus encoder create failed: %d", opus_err);
    }
    opus_encoder_ctl(opus_enc,OPUS_SET_EXPERT_FRAME_DURATION(OPUS_FRAMESIZE_10_MS));
    opus_encoder_ctl(opus_enc,OPUS_SET_BITRATE(200 * 8 * 100));
    opus_encoder_ctl(opus_enc,OPUS_SET_VBR(false));
    opus_encoder_ctl(opus_enc,OPUS_SET_COMPLEXITY(0)); // max 10 in 320MHz

    bool queue_created = queue_init(&spk_pcm_queue, sizeof(pcm_element), SPEAKER_PCM_QUEUE_DEPTH);
    if (!queue_created)
    {
        log_error("pcm queue init failed");
    }

    int sem_err = os_sem_create(&uac_sem, 0);
    if (sem_err)
    {
        log_error("audio semaphore create failed: %d", sem_err);
    }

    int opus_sem_err = os_sem_create(&opus_sem, 0);
    if (opus_sem_err)
    {
        log_error("bt send semaphore create failed: %d", opus_sem_err);
    }

    cbuf_init(&uac_cbuf, uac_buf, sizeof(uac_buf));

    int task_err = task_create(ds5_audio_task, NULL, "audio_src");
    if (task_err)
    {
        log_error("audio task create failed: %d", task_err);
    }

    int opus_task_err = task_create(ds5_audio_opus_task, NULL, "audio_opus");
    if (opus_task_err)
    {
        log_error("bt audio send task create failed: %d", task_err);
    }


    if (!haptics_src ||
        !speaker_src ||
        !queue_created ||
        sem_err ||
        opus_sem_err ||
        task_err ||
        opus_task_err ||
        opus_err)
    {
        os_sem_del(&uac_sem, OS_DEL_ALWAYS);
        free(spk_pcm_queue.data);
        spk_pcm_queue.data = NULL;
        if (haptics_src)
        {
            src_base_close(haptics_src);
            haptics_src = NULL;
        }
        opus_enc = NULL;
        return;
    }

    task_created = true;
    log_info("audio task created");
}

/**
 * @file lv_ffmpeg.h
 *
 */
#ifndef LV_JLFFMPEG_H
#define LV_JLFFMPEG_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../../../lvgl.h"
#if LV_USE_JLFFMPEG != 0

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
extern const uint8_t lv_jlffmpeg_debug_time;
extern const uint8_t lv_jlffmpeg_use_frame_ctl;
extern const uint8_t frame_control_sens;
extern const uint8_t jlffmpeg_use_media_interface;
extern const uint32_t lv_jlffmpeg_msg;

struct ffmpeg_context_s;

typedef struct ffmpeg_clock {
    int pts;
    int pts_drift;
    // int last_updated;
} ffmpeg_clock;

extern const lv_obj_class_t lv_ffmpeg_player_class;

struct ffmpeg_context_s {
    bool has_alpha;
    uint32_t width;
    uint32_t height;
    uint32_t dst_width;
    uint32_t dst_heitht;
    uint32_t total_time;
    uint32_t total_frame;
    uint32_t idx;
    uint32_t audio_idx;
    uint32_t audio_chunk_num;
    uint32_t audio_to_video_idx;
    uint32_t audio_sample_rate;
    uint8_t audio_pause;
    uint8_t audio_paused;
    uint8_t play_sync;
    uint8_t audio_task_exit;
    void *audio_pause_sem;
    void *read_mutex;
    lv_fs_file_t *lv_file;
    uint32_t audio_frame_timer;
    uint32_t audio_read_len;
    uint32_t audio_frame_len;
    uint32_t video_frame_duration;
    ffmpeg_clock audio_clk;
    ffmpeg_clock video_clk;
    int size;
    uint8_t *jpg_data;
    uint8_t *buf_act;
    uint8_t *buf1_data;
    uint8_t *buf1_ori;
    uint8_t *buf2_data;
    uint8_t *buf2_ori;
    uint8_t *buf3_data;
    uint8_t *audio_cbuf;
    uint8_t *audio_data_buf;
    void *audio_dec_save_cbuf;
    void *audio_dec_server;
    void *audio_player;
    void *pipeline_core;
    bool auto_cycle;
    lv_obj_t *parent_obj;
};

typedef struct {
    lv_img_t img;
    lv_timer_t *timer;
    lv_img_dsc_t imgdsc;
    bool auto_restart;
    struct ffmpeg_context_s *ffmpeg_ctx;
    void (*autostop_cb)(lv_obj_t *);
    void (*frame_begin_cb)(lv_obj_t *);
    void (*frame_end_cb)(lv_obj_t *);
} lv_ffmpeg_player_t;

typedef enum {
    LV_FFMPEG_PLAYER_CMD_START,
    LV_FFMPEG_PLAYER_CMD_STOP,
    LV_FFMPEG_PLAYER_CMD_PAUSE,
    LV_FFMPEG_PLAYER_CMD_RESUME,
    LV_FFMPEG_PLAYER_CMD_AUDIO_PAUSE,
    LV_FFMPEG_PLAYER_CMD_AUDIO_RESUME,
    _LV_FFMPEG_PLAYER_CMD_LAST
} lv_ffmpeg_player_cmd_t;

typedef enum {
    LV_FFMPEG_IMG_DECODE_START,
    LV_FFMPEG_IMG_DECODE_STOP,
    LV_FFMPEG_IMG_DECODE_TASK_KILL
} lv_ffmpeg_img_decode_status_t;
/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Register FFMPEG image decoder
 * @param doublebuf_en 配置方法见lv_conf.h
 * @param msg_value Q_USER_TYPE
 * @param msg_event_value  与UI_MSG_RPC_FUNC同值
 */
void lv_ffmpeg_init(uint8_t doublebuf_en, int msg_value, uint8_t msg_event_value);
/**
 * Get the number of frames contained in the file
 * @param path image or video file name
 * @return Number of frames, less than 0 means failed
 */
int lv_ffmpeg_get_frame_num(const char *path);

/**
 * Create ffmpeg_player object
 * @param parent pointer to an object, it will be the parent of the new player
 * @return pointer to the created ffmpeg_player
 */
lv_obj_t *lv_ffmpeg_player_create(lv_obj_t *parent);

/**
 * Set the path of the file to be played
 * @param obj pointer to a ffmpeg_player object
 * @param path video file path
 * @return LV_RES_OK: no error; LV_RES_INV: can't get the info.
 */
lv_res_t lv_ffmpeg_player_set_src(lv_obj_t *obj, const char *path);

/**
 * Set command control video player
 * @param obj pointer to a ffmpeg_player object
 * @param cmd control commands
 */
void lv_ffmpeg_player_set_cmd(lv_obj_t *obj, lv_ffmpeg_player_cmd_t cmd);

/**
 * 跳转到指定视频帧（跳帧/seek）
 * @param obj pointer to a ffmpeg_player object
 * @param frame 目标帧号，从 1 开始（1 表示第一帧），越界会自动钳制到 [1, total_frame]
 */
void lv_ffmpeg_player_seek(lv_obj_t *obj, uint32_t frame);

/**
 * Set the video to automatically replay
 * @param obj pointer to a ffmpeg_player object
 * @param en true: enable the auto restart
 */
void lv_ffmpeg_player_set_auto_restart(lv_obj_t *obj, bool en);

void lv_ffmpeg_player_set_zoom(lv_obj_t *obj, uint16_t zoom);
/*=====================
 * Other functions
 *====================*/
/**
 * 注册播完自动暂停回调
 * @param obj pointer to a ffmpeg_player object
 * @param autostop_cb: cb: void cb(lv_obj_t *obj);
 */
void lv_jlffmpeg_set_autostop_cb(lv_obj_t *obj, void (*autostop_cb)(lv_obj_t *));

/**
 * 注册解码开始回调
 * @param obj pointer to a ffmpeg_player object
 * @param frame_begin_cb: cb: void cb(lv_obj_t *obj);
 */
void lv_jlffmpeg_set_frame_begin_cb(lv_obj_t *obj, void (*frame_begin_cb)(lv_obj_t *));

/**
 * 注册解码结束回调
 * @param obj pointer to a ffmpeg_player object
 * @param frame_end_cb: cb: void cb(lv_obj_t *obj);
 */
void lv_jlffmpeg_set_frame_end_cb(lv_obj_t *obj, void (*frame_end_cb)(lv_obj_t *));

/**
 * 获取当前解码器状态
 */
lv_ffmpeg_img_decode_status_t lv_ffmpeg_player_get_decode_state(void);
/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_JLFFMPEG*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_JLFFMPEG_H*/

#include "system/includes.h"
#include "asm/includes.h"
#include "server/video_server.h"
#include "server/video_engine_server.h"
#include "video_rec.h"
#include "video_photo.h"
#include "video_system.h"
#include "app_msg.h"
#include "lcd_config.h"
#include "event/key_event.h"
#include "event/device_event.h"
#include "app_power_manage.h"
#include "asm/uvc_device.h"


#include "action.h"
#include "app_config.h"
#include "asm/debug.h"
#include "vrec_osd.h"
#include "vrec_icon_osd.h"
#include "app_database.h"
#include "storage_device.h"
#include "net_video_rec.h"

#if TCFG_GSENSOR_ENABLE
#include "gSensor_manage.h"
#endif


#define AUDIO_VOLUME	100


#ifndef CONFIG_VIDEO0_ENABLE
#undef VREC0_FBUF_SIZE
#define VREC0_FBUF_SIZE   0
#endif
#ifndef CONFIG_VIDEO1_ENABLE
#undef VREC1_FBUF_SIZE
#define VREC1_FBUF_SIZE   0
#endif
#ifndef CONFIG_VIDEO2_ENABLE
#undef VREC2_FBUF_SIZE
#define VREC2_FBUF_SIZE   0
#endif
#ifndef CONFIG_VIDEO3_ENABLE
#undef VREC3_FBUF_SIZE
#define VREC3_FBUF_SIZE   0
#endif
#ifndef CONFIG_VIDEO4_ENABLE
#undef VREC4_FBUF_SIZE
#define VREC4_FBUF_SIZE   0
#endif
#ifndef CONFIG_VIDEO5_ENABLE
#undef VREC5_FBUF_SIZE
#define VREC5_FBUF_SIZE   0
#endif


//*********************************************************************************//
//                             编码图片分辨率                                      //
//*********************************************************************************//
#if __SDRAM_SIZE__ > (8 * 1024 * 1024)
#define CONFIG_VIDEO_IMAGE_W    1280
#define CONFIG_VIDEO_IMAGE_H    720
#else
#define CONFIG_VIDEO_IMAGE_W    640
#define CONFIG_VIDEO_IMAGE_H    480
#endif



#if ( LCD_W > 960 && LCD_H > 320) //显示大分辨率关闭裁剪功能
#define VIDEO_LARGE_IMAGE 		1
#else
#define VIDEO_LARGE_IMAGE		0
#endif

#define SCREEN_W        LCD_W
#define SCREEN_H        LCD_H
#if LCD_W > 1280
#define SMALL_SCREEN_W          480 //16 aline
#define SMALL_SCREEN_H          400 //16 aline
#elif LCD_W > 480
#define SMALL_SCREEN_W          320 //16 aline
#define SMALL_SCREEN_H          240 //16 aline
#else
#define SMALL_SCREEN_W          480 //16 aline
#define SMALL_SCREEN_H           320//16 aline
#endif

#define MOTION_STOP_SEC     20
#define MOTION_START_SEC    2

#define LOCK_FILE_PERCENT	40    //0~100
#define NAME_FILE_BY_DATE   0


//暂不使用
#define CAMERA_THUMBNAIL_ENABLE 0
#define THUMBNAIL_W     192
#define THUMBNAIL_H     160
#define IMAGE_THUMB_BUF_SIZE      (64 * 1024)

//三路屏幕尺寸限制
#if THREE_WAY_ENABLE
#if ((LCD_W > 640) || (LCD_H > 368)) && (__SDRAM_SIZE__ <= 8*1024*1024)
#error "Three way enable. This resolution is not supported!"
#endif
#endif



static int video_rec_buf_alloc(void);
static int video_rec_start();
static int video_rec_stop(u8 close);
static int ve_mdet_start();
static int ve_mdet_stop();
static int ve_lane_det_start(u8 fun_sel);
static int ve_lane_det_stop(u8 fun_sel);
static int video_rec_savefile(int dev_id);
static void get_isp_lv_timer_cb(void *p);
extern int video_rec_set_config(struct intent *it);
extern int user_isp_get_current_scene(int channel);

//pipeline 事件回调函数
#include "pipeline_core.h"
void video_pipeline_extern_do(const char *name, int event, void *arg)
{
    buffer_meta_t *buf_meta = (buffer_meta_t *)arg;
    if (name && !strncmp(name, "uvc", 3)) {
        switch (event) {
        case EVENT_DATA_DONE: //获取uvc收到的数据帧
            if (buf_meta) {
                /* put_buf(buf_meta->ext_data,buf_meta->data_len); */
            }
            break;
        case EVENT_FRAME_DONE: //uvc解码完一帧数据回调
            break;
        case EVENT_BUFFER_EMPTY: //uvc获取不到数据回调
            break;
        }
    }
}

static void ve_mdet_reset();
static int video_rec_sd_in();
static int video_rec_sd_out();
static int video_rec_device_event_handler(struct sys_event *event);
void ve_lane_det_reset();
extern char *video_rec_finish_get_name(FILE *fd, int index, u8 is_emf); //index：video0则0，video1则1，video2则2
extern int video_rec_finish_notify(char *path);
extern int video_rec_delect_notify(FILE *fd, int id);
extern int video_rec_err_notify(const char *method);
extern int video_rec_state_notify(void);
extern int video_rec_start_notify(void);
extern int video_rec_all_stop_notify(void);
#ifdef CONFIG_NET_ENABLE
/******************************用于网络实时流*************************************/
extern int net_video_rec_event_notify(void);
extern int net_video_rec_event_stop(void);
extern int net_video_rec_event_start(void);

static int video_rec_get_abr(u32 width);
static void video_disp_stop(int id);
static int video_disp_start(int id, const struct video_window *win);
static int show_main_ui();
static void hide_main_ui();

/**********************************************************************************/
#endif

int x_offset = 0;
int y_offset = 0;
char video_rec_osd_buf[64] ALIGNE(64);
struct video_rec_hdl rec_handler;
#define __this 	(&rec_handler)


static const u16 rec_pix_w[] = {1920, 1280, 640};
static const u16 rec_pix_h[] = {1088, 720,  480};
static const u16 pic_pix_w[] = {1280, 1920, 2560, 3072};
static const u16 pic_pix_h[] = {720,  1088, 1600, 2208};

static struct video_window disp_window[DISP_MAX_WIN][4] = {0};

static void video_set_disp_window()
{
    u16 small_screen_w;
    u16 small_screen_h;

#if THREE_WAY_ENABLE
    if (LCD_W > 1280) {
        small_screen_w = 480; //16 aline
        small_screen_h = 400; //16 aline
    } else if (LCD_W > 480) {
        small_screen_w = 192; //16 aline
        small_screen_h = 160; //16 aline
    } else {
        small_screen_w = 192; //16 aline
        small_screen_h = 160; //16 aline
    }
#else
    if (LCD_W > 1280) {
        small_screen_w = 480; //16 aline
        small_screen_h = 400; //16 aline
    } else if (LCD_W > 480) {
        small_screen_w = 320; //16 aline
        small_screen_h = 240; //16 aline
    } else {
        small_screen_w = 480; //16 aline
        small_screen_h = 320; //16 aline
    }
#endif

    //DISP_MAIN_WIN
    disp_window[DISP_MAIN_WIN][0].width  = SCREEN_W;
    disp_window[DISP_MAIN_WIN][0].height = SCREEN_H;
    disp_window[DISP_MAIN_WIN][1].width  = small_screen_w;
    disp_window[DISP_MAIN_WIN][1].height = small_screen_h;

    disp_window[DISP_MAIN_WIN][2].width  = small_screen_w;
    disp_window[DISP_MAIN_WIN][2].height = small_screen_h;
#if THREE_WAY_ENABLE
    //右边UI遮挡到摄像头
    u16 ui_width = 75;
    disp_window[DISP_MAIN_WIN][1].width  = small_screen_w;
    disp_window[DISP_MAIN_WIN][1].height = small_screen_h;
    disp_window[DISP_MAIN_WIN][1].left   = SCREEN_W - small_screen_w - ui_width;
    disp_window[DISP_MAIN_WIN][1].top    = 0;
#endif

    //DISP_HALF_WIN
    disp_window[DISP_HALF_WIN][0].width  = SCREEN_W / 2;
    disp_window[DISP_HALF_WIN][0].height = SCREEN_H;
    disp_window[DISP_HALF_WIN][2].left   = SCREEN_W / 2;
    disp_window[DISP_HALF_WIN][2].width  = SCREEN_W / 2;
    disp_window[DISP_HALF_WIN][2].height = SCREEN_H;
#if THREE_WAY_ENABLE
    disp_window[DISP_HALF_WIN][2].height = SCREEN_H / 2;
    disp_window[DISP_HALF_WIN][1].left   = SCREEN_W / 2;
    disp_window[DISP_HALF_WIN][1].top    = SCREEN_H / 2;
    disp_window[DISP_HALF_WIN][1].width  = SCREEN_W / 2;
    disp_window[DISP_HALF_WIN][1].height = SCREEN_H / 2;
#endif

    //DISP_FRONT_WIN
    disp_window[DISP_FRONT_WIN][0].width  = SCREEN_W;
    disp_window[DISP_FRONT_WIN][0].height = SCREEN_H;
    disp_window[DISP_FRONT_WIN][2].width  = (u16) - 1;
    disp_window[DISP_FRONT_WIN][2].height = 0;
#if THREE_WAY_ENABLE
    disp_window[DISP_FRONT_WIN][1].width  = (u16) - 1;
    disp_window[DISP_FRONT_WIN][1].height = 0;
#endif

    //DISP_BACK_WIN
    disp_window[DISP_BACK_WIN][0].width  = (u16) - 1;
    disp_window[DISP_BACK_WIN][0].height = 0;
    disp_window[DISP_BACK_WIN][2].width  = SCREEN_W;
    disp_window[DISP_BACK_WIN][2].height = SCREEN_H;
#if THREE_WAY_ENABLE
    disp_window[DISP_BACK_WIN][1].width  = (u16) - 1;
    disp_window[DISP_BACK_WIN][1].height = 0;
#endif


#if THREE_WAY_ENABLE
    //DISP_THREE_WIN
    disp_window[DISP_THREE_WIN][0].width  = (u16) - 1;
    disp_window[DISP_THREE_WIN][0].height = 0;
    disp_window[DISP_THREE_WIN][2].width  = (u16) - 1;
    disp_window[DISP_THREE_WIN][2].height = (u16) - 1;
    disp_window[DISP_THREE_WIN][1].width  = SCREEN_W;
    disp_window[DISP_THREE_WIN][1].height = SCREEN_H;
#endif


    //DISP_PARK_WIN
    disp_window[DISP_PARK_WIN][0].width  = (u16) - 1;
    disp_window[DISP_PARK_WIN][0].height = 0;
    disp_window[DISP_PARK_WIN][2].width  = SCREEN_W;
    disp_window[DISP_PARK_WIN][2].height = SCREEN_H;
#if THREE_WAY_ENABLE
    disp_window[DISP_PARK_WIN][1].width  = (u16) - 1;
    disp_window[DISP_PARK_WIN][1].height = 0;
#endif
}

static const char *rec_dir[][2] = {
#ifdef CONFIG_EMR_DIR_ENABLE
    {CONFIG_REC_DIR_0, CONFIG_EMR_REC_DIR_0},
    {CONFIG_REC_DIR_1, CONFIG_EMR_REC_DIR_1},
    {CONFIG_REC_DIR_2, CONFIG_EMR_REC_DIR_2},
    {CONFIG_REC_DIR_2, CONFIG_EMR_REC_DIR_2},
    {CONFIG_REC_DIR_4, CONFIG_EMR_REC_DIR_4},
    {CONFIG_REC_DIR_5, CONFIG_EMR_REC_DIR_5},
#else
    {CONFIG_REC_DIR_0, CONFIG_REC_DIR_0},
    {CONFIG_REC_DIR_1, CONFIG_REC_DIR_1},
    {CONFIG_REC_DIR_2, CONFIG_REC_DIR_2},
    {CONFIG_REC_DIR_2, CONFIG_REC_DIR_2},
    {CONFIG_REC_DIR_4, CONFIG_REC_DIR_4},
    {CONFIG_REC_DIR_5, CONFIG_REC_DIR_5},
#endif
};

static const char *rec_path[][2] = {
#ifdef CONFIG_EMR_DIR_ENABLE
    {CONFIG_REC_PATH_0, CONFIG_EMR_REC_PATH_0},
    {CONFIG_REC_PATH_1, CONFIG_EMR_REC_PATH_1},
    {CONFIG_REC_PATH_2, CONFIG_EMR_REC_PATH_2},
    {CONFIG_REC_PATH_2, CONFIG_EMR_REC_PATH_2},
    {CONFIG_REC_PATH_4, CONFIG_EMR_REC_PATH_4},
    {CONFIG_REC_PATH_5, CONFIG_EMR_REC_PATH_2},
#else
    {CONFIG_REC_PATH_0, CONFIG_REC_PATH_0},
    {CONFIG_REC_PATH_1, CONFIG_REC_PATH_1},
    {CONFIG_REC_PATH_2, CONFIG_REC_PATH_2},
    {CONFIG_REC_PATH_2, CONFIG_REC_PATH_2},
    {CONFIG_REC_PATH_4, CONFIG_REC_PATH_4},
    {CONFIG_REC_PATH_5, CONFIG_REC_PATH_5},
#endif
};

void *get_video_rec_handler(void)
{
    return (void *)&rec_handler;
}
int video_rec_get_fps()
{
#ifdef LOCAL_VIDEO_REC_FPS
    return LOCAL_VIDEO_REC_FPS;
#else
    return 0;
#endif
}

int video_rec_get_audio_sampel_rate(void)
{
#ifdef  VIDEO_REC_AUDIO_SAMPLE_RATE
    return VIDEO_REC_AUDIO_SAMPLE_RATE;
#else
    return 8000;
#endif
}
int video_rec_control_start(void)
{
    int err;
    err = video_rec_start();
    return err;
}

int video_rec_control_doing(void)
{
    int err;
    if (__this->state == VIDREC_STA_START) {
        err = video_rec_stop(0);
        ve_mdet_reset();
        ve_lane_det_reset();
    } else {
        err = video_rec_start();
    }
    return err;
}

int video_rec_device_event_action(struct sys_event *event)
{
    return video_rec_device_event_handler(event);
}

int video_rec_sd_in_notify(void)
{
    return video_rec_sd_in();
}

int video_rec_sd_out_notify(void)
{
    return video_rec_sd_out();
}

int video_rec_get_abr_from(u32 width)
{
    return video_rec_get_abr(width);
}
#ifdef CONFIG_NET_ENABLE
/******************************用于网络实时流*************************************/

int net_video_disp_stop(int id)
{
    video_disp_stop(id);
    return 0;
}
int net_video_disp_start(int id)
{
    video_disp_start(id, &disp_window[0][0]);
    return 0;
}

/******************************************************************/
#endif

static int set_disp_crop(u8 camera_id, int crop_w, int crop_h, int x_offset, int y_offset)
{
    int err = 0;
    union video_req req = {0};
    struct video_source_crop crop = {0};

    //摄像头输出大小
    int src_width = CONFIG_VIDEO_IMAGE_W;
    int src_height = CONFIG_VIDEO_IMAGE_H;

    if (crop_w > src_width || crop_h > src_height) {
        log_e("crop size > src size !!!");
        return -EFAULT;
    }

    crop.width = crop_w;
    crop.height = crop_h;

    //居中裁剪
    crop.x_offset = (src_width - crop.width) / 2;
    crop.y_offset = (src_height - crop.height) / 2;

    //在中间的基础上进行偏移
    crop.x_offset += x_offset;
    crop.y_offset += y_offset;

    //限制偏移范围
    int x_offset_max = src_width - crop.width;
    int x_offset_min = 0;
    int y_offset_max = src_height - crop.height;
    int y_offset_min = 0;
    crop.x_offset = crop.x_offset < x_offset_min ? x_offset_min :
                    crop.x_offset > x_offset_max ? x_offset_max :
                    crop.x_offset;
    crop.y_offset = crop.y_offset < y_offset_min ? y_offset_min :
                    crop.y_offset > y_offset_max ? y_offset_max :
                    crop.y_offset;

    if (!__this->video_display[camera_id]) {
        log_e("video_display id:%d  err", camera_id);
        return -EFAULT;
    }

    req.display.crop = &crop;
    req.display.state = VIDEO_STATE_SET_DIS_CROP;
    err = server_request(__this->video_display[camera_id], VIDEO_REQ_DISPLAY, &req);
    if (err) {
        log_e("%s err", __FUNCTION__);
        return err;
    }

    return 0;
}



#if NAME_FILE_BY_DATE
static int __get_sys_time(struct sys_time *time)
{
    void *fd = dev_open("rtc", NULL);
    if (fd) {
        dev_ioctl(fd, IOCTL_GET_SYS_TIME, (u32)time);
        dev_close(fd);
        return 0;
    }

    return -EINVAL;
}
#endif

static const char *rec_file_name(int format)
{
#if NAME_FILE_BY_DATE
    struct sys_time time;
    static char file_name[MAX_FILE_NAME_LEN];

    if (__get_sys_time(&time) == 0) {
        if (format == VIDEO_FMT_AVI) {
            sprintf(file_name, "VID_%d%02d%02d_%02d%02d%02d.AVI",
                    time.year, time.month, time.day, time.hour, time.min, time.sec);
        } else if (format == VIDEO_FMT_MOV) {
            sprintf(file_name, "VID_%d%02d%02d_%02d%02d%02d.MOV",
                    time.year, time.month, time.day, time.hour, time.min, time.sec);
        } else if (format == VIDEO_FMT_MP4) {
            sprintf(file_name, "VID_%d%02d%02d_%02d%02d%02d.MP4",
                    time.year, time.month, time.day, time.hour, time.min, time.sec);
        }
        return file_name;
    }
#endif

    if (format == VIDEO_FMT_AVI) {
        return "VID_****.AVI";
    } else if (format == VIDEO_FMT_MP4) {
        return "VID_****.MP4";
    } else {
        return "VID_****.MOV";
    }
}




u32 get_video_disp_state()
{
    return __this->disp_state;
}

static void video_home_post_msg(const char *msg, ...)
{
#ifdef CONFIG_UI_STYLE_JL02_ENABLE
    union uireq req;
    va_list argptr;

    va_start(argptr, msg);

    if (__this->ui) {
        req.msg.receiver = ID_WINDOW_MAIN_PAGE;
        req.msg.msg = msg;
        req.msg.exdata = argptr;

        server_request(__this->ui, UI_REQ_MSG, &req);
    }

    va_end(argptr);

#endif
}

void video_parking_post_msg(const char *msg, ...)
{
#if 0 //def CONFIG_UI_ENABLE
    union uireq req;
    va_list argptr;

    va_start(argptr, msg);

    if (__this->ui) {
        req.msg.receiver = ID_WINDOW_PARKING;
        req.msg.msg = msg;
        req.msg.exdata = argptr;

        server_request(__this->ui, UI_REQ_MSG, &req);
    }

    va_end(argptr);
#endif
}

_WEAK_ void video_rec_post_msg(const char *msg, ...)
{
#if 0 //def CONFIG_UI_ENABLE
    union uireq req;
    va_list argptr;

    va_start(argptr, msg);

    if (__this->ui) {
        req.msg.receiver = ID_WINDOW_VIDEO_REC;
        req.msg.msg = msg;
        req.msg.exdata = argptr;

        server_request(__this->ui, UI_REQ_MSG, &req);
    }

    va_end(argptr);

#endif

}


static int video_rec_online_nums()
{
    u8 nums = 0;

    for (int i = 0; i < CONFIG_VIDEO_REC_NUM; i++) {
        if (__this->video_online[i]) {
            nums++;
        }
    }

    return nums;
}

//仅支持用于单raw情况下打开, 双raw不支持
int video_pre_create(int id, int open)
{
    int err = 0;

    if (!__this->video_pre[id]) {
        char dev_name[12] = {0};
        sprintf(dev_name, "video%d.%d", id, id < 2 ? 0 : __this->uvc_id);
        __this->video_pre[id] = server_open("video_server", (void *)dev_name);
        if (!__this->video_pre[id]) {
            log_e("open video_server: faild, id = %d\n", id);
            return -EFAULT;
        }
    }

    err = server_request(__this->video_pre[id], VIDEO_REQ_PRE_CREATE, &open);
    if (err) {
        server_close(__this->video_pre[id]);
        __this->video_pre[id] = NULL;
    }

    return err;
}

int video_disp_start(int id, const struct video_window *win)
{
    int err = 0;
    union video_req req = {0};
    char dev_name[12] = {0};
    char fb_name [12] = {0};
    /* if(id == 1) { */
    /* return 0; */
    /* } */
#ifdef CONFIG_DISPLAY_ENABLE

    if (win->width == (u16) - 1) {
        puts("video_disp_hide\n");
        return 0;
    }
    if (__this->video_display[id]) {
        return 0;
    }

    sprintf(fb_name, "fb%d", id + 1);

    if (id == 2) {
        sprintf(dev_name, "video%d.%d", 10 + __this->uvc_id, id < 2 ? 0 : __this->uvc_id);
    } else {
        sprintf(dev_name, "video%d.0", id);
    }

    log_d("%s: %d, %d x %d\n", __func__, id, win->width, win->height);
    __this->video_display[id] = server_open("video_server", (void *)dev_name);
    if (!__this->video_display[id]) {
        log_e("open video_server: faild, id = %d\n", id);
        return -EFAULT;
    }
    //req.display.disp_id	        = id; //test

    req.display.fb 		        = fb_name;
    req.display.left  	        = win->left;
    req.display.top 	        = win->top;
    req.display.width 	        = win->width;
    req.display.height 	        = win->height;
    req.display.border_left     = win->border_left;
    req.display.border_top      = win->border_top;
    req.display.border_right    = win->border_right;
    req.display.border_bottom   = win->border_bottom;
    req.display.win_type        = win->win_type;
#if (DOUBLE_720_SMALL_SCR == 1)
    req.display.double720_small_scr = true;
#endif
#ifdef DIGITAL_SCALE
    req.display.src_crop_enable = VIDEO_CROP_ENABNLE | VIDEO_LARGE_IMAGE;////支持通过配置比例对IMC源数据进行裁剪,使用数字变焦需要写1
    req.display.sca.x = (1280 - x_offset) / 2;
    req.display.sca.y = (720 - y_offset) / 2;
    req.display.sca.tar_w = x_offset;
    req.display.sca.tar_h = y_offset;
#else
    req.display.src_crop_enable = VIDEO_LARGE_IMAGE;
#endif


#if THREE_WAY_DOUBLE_RAW
    req.display.double_raw      = 1;
    req.display.three_way_type = VIDEO_THREE_WAY_DOU_RAW;
#endif

    if (id == 2) {
        req.display.uvc_id = __this->uvc_id;
        req.display.camera_config = NULL;
        req.display.camera_type = VIDEO_CAMERA_UVC;
        req.display.src_w = __this->src_width[2];
        req.display.src_h = __this->src_height[2];
        /* req.display.mirror = VIDEO_HOR_MIRROR | VIDEO_VER_MIRROR; */
        /* req.display.rotate = 90; //90/270 */
        if (req.display.rotate == 90 || req.display.rotate == 270) {
            //旋转图像时，先缩放再旋转，display.width和req.display.height表示缩放后旋转前的图像分辨率
            /* req.display.width 	        = win->height; */
            /* req.display.height 	        = win->width; */
        }
    } else {
        req.display.camera_config   = NULL;//load_default_camera_config;
        req.display.camera_type     = VIDEO_CAMERA_NORMAL;
    }

    req.display.state 	        = VIDEO_STATE_START;
    req.display.pctl            = NULL;

#ifdef PHOTO_STICKER_ENABLE_SMALL_MEM
    if (__this->sticker_name) {
        req.display.sticker_name      = __this->sticker_name;
    }
#endif
    err = server_request(__this->video_display[id], VIDEO_REQ_DISPLAY, &req);
    if (err) {
        server_close(__this->video_display[id]);
        __this->video_display[id] = NULL;
    }

    if (id == 2) {
#ifdef CONFIG_VIDEO2_ENABLE
        uvc_parking_enable(1);
#endif
    } else {
        /*rec显示重设曝光补偿*/
        /* __this->exposure_set = 1; */
        /* video_rec_set_exposure(db_select("exp")); */
    }
#endif

    return err;
}

int video2_disp_start(int sub_id, struct video_window *win)
{
    int err = 0;
    union video_req req = {0};
    static char dev_name[12];
#ifdef CONFIG_DISPLAY_ENABLE
    /* video2.x 子设备号x划分: 0-4为uvc0 5-6为uvc1 7-8为uvc2 */
    u8 id = sub_id;

    if (win->width == (u16) - 1) {
        puts("video_disp_hide\n");
        return 0;
    }
    sprintf(dev_name, "video10.%d", sub_id * 5);
    if (sub_id >= 2) {
        sprintf(dev_name, "video10.%d", sub_id + 5);
    }
    log_d("video2_disp_start: %s, %d x %d\n", dev_name, win->width, win->height);
    if (!__this->video2_display[id]) {
        __this->video2_display[id] = server_open("video_server", (void *)dev_name);
        if (!__this->video2_display[id]) {
            log_e("open video_server: faild, id = %d\n", id);
            return -EFAULT;
        }
    } else {
        log_w("%s already open display.\n");
    }
    req.display.fb 		        = "fb2";
    req.display.left  	        = win->left;
    req.display.top 	        = win->top;
    req.display.width 	        = win->width;
    req.display.height 	        = win->height;
    req.display.border_left     = win->border_left;
    req.display.border_top      = win->border_top;
    req.display.border_right    = win->border_right;
    req.display.border_bottom   = win->border_bottom;
    req.display.win_type        = win->win_type;

    req.display.uvc_id = sub_id;
    req.display.camera_config = NULL;
    req.display.camera_type = VIDEO_CAMERA_UVC;
    req.display.mirror = 0;  //1: 显示镜像
    req.display.src_w = __this->src_width[2];
    req.display.src_h = __this->src_height[2];
    req.display.state 	        = VIDEO_STATE_START;
    req.display.pctl            = NULL;
    err = server_request(__this->video2_display[id], VIDEO_REQ_DISPLAY, &req);
    if (err) {
        server_close(__this->video2_display[id]);
        __this->video2_display[id] = NULL;
    }

#endif

    return err;
}

void video2_disp_stop(int id)
{
#ifdef CONFIG_DISPLAY_ENABLE
    union video_req req = {0};
    if (__this->video2_display[id]) {
        req.display.state 	= VIDEO_STATE_STOP;
        server_request(__this->video2_display[id], VIDEO_REQ_DISPLAY, &req);

        server_close(__this->video2_display[id]);
        __this->video2_display[id] = NULL;
    }
#endif
}

static void video_disp_stop(int id)
{
#ifdef CONFIG_DISPLAY_ENABLE
    union video_req req;

    if (__this->video_display[id]) {

        req.display.state 	= VIDEO_STATE_STOP;
        server_request(__this->video_display[id], VIDEO_REQ_DISPLAY, &req);

        server_close(__this->video_display[id]);
        __this->video_display[id] = NULL;
        log_d("%s: %d\n", __func__, id);

    }
#endif
}

static int video_disp_win_switch(int mode, int dev_id)
{
    int err = 0;
#ifdef CONFIG_DISPLAY_ENABLE
    int i;
    int next_win;
    int curr_win = __this->disp_state;
    int second_disp_dev = __this->second_disp_dev;

    switch (mode) {
    case DISP_WIN_SW_SHOW_PARKING:
        if (!__this->video_online[__this->disp_park_sel]) {
            return -ENODEV;
        }
        if (curr_win == DISP_BACK_WIN && second_disp_dev == __this->disp_park_sel) {
            return 0;
        }
        next_win        = DISP_PARK_WIN;
        second_disp_dev = __this->disp_park_sel;
        break;

    case DISP_WIN_SW_HIDE_PARKING:
        if (curr_win == DISP_BACK_WIN && second_disp_dev == __this->disp_park_sel) {
            return 0;
        }
        next_win = curr_win;
        curr_win = DISP_PARK_WIN;
        break;

    case DISP_WIN_SW_SHOW_SMALL:
        curr_win        = DISP_MAIN_WIN;
        next_win        = DISP_MAIN_WIN;
        second_disp_dev = 0;
        break;

    case DISP_WIN_SW_SHOW_NEXT:
        if (video_rec_online_nums() < 2) {
            return 0;
        }
        if (get_parking_status() == 1) {
            return 0;
        }
        next_win = curr_win;
        if (++next_win >= DISP_PARK_WIN) {
            next_win = DISP_MAIN_WIN;
        }
        break;

    case DISP_WIN_SW_DEV_IN:
        next_win = curr_win;
        break;

    case DISP_WIN_SW_DEV_OUT:
        if (dev_id == 0) {
            curr_win        = -1;
            next_win        = DISP_MAIN_WIN;
            second_disp_dev = 0;
        } else if (second_disp_dev == dev_id) {
            next_win        = curr_win;
            second_disp_dev = 0;
            for (i = 1; i < CONFIG_VIDEO_REC_NUM; i++) {
                if (__this->video_online[i]) {
                    second_disp_dev = i;
                    break;
                }
            }
            if (second_disp_dev == 0) {
                next_win = DISP_MAIN_WIN;
            }
        } else {
            return 0;
        }
        break;

    default:
        return -EINVAL;
    }

    printf("disp_win_switch: %d, %d, %d\n", curr_win, next_win, second_disp_dev);

    for (i = CONFIG_VIDEO_REC_NUM - 1; i >= 0 ; i--) {
        video_disp_stop(i);
    }

    for (i = 0; i < CONFIG_VIDEO_REC_NUM; i++) {
        if (__this->video_online[i]) {
            err = video_disp_start(i, &disp_window[next_win][i == 4 ? 0 : (i == 5 ? 2 : i)]);
        }
    }

    if (i == CONFIG_VIDEO_REC_NUM) {
        second_disp_dev = 0;
    }

    if (next_win != DISP_PARK_WIN) {
        __this->disp_state = next_win;
        __this->second_disp_dev = second_disp_dev;
    }

    if (__this->disp_state == DISP_BACK_WIN) {
        /* 进入后视窗口前照灯关闭 */
        video_rec_post_msg("HlightOff");

        /* 后拉全屏显示（非录像状态），固定屏显场景1 */
        isp_scr_work_hdl(1);
    }

#endif
    return err;
}

static void rec_dev_server_event_handler(void *priv, int argc, int *argv)
{
    /*
     *该回调函数会在录像过程中，写卡出错被当前录像APP调用，例如录像过程中突然拔卡
     */
    int mark = 2;
    struct intent it;

    switch (argv[0]) {
    case VIDEO_SERVER_UVM_ERR:
        log_e("APP_UVM_DEAL_ERR\n");
        break;
    case VIDEO_SERVER_PKG_ERR:
        log_e("video_server_pkg_err\n");
        if (__this->state == VIDREC_STA_START) {
#ifdef CONFIG_NET_ENABLE
            video_rec_err_notify("VIDEO_REC_ERR");
#endif
            video_rec_stop(0);
        }
        break;
    case VIDEO_SERVER_PKG_END:
        if (db_select("cyc")) {
            video_rec_savefile((int)priv);
        } else {
            video_rec_stop(0);
        }
        break;

#ifdef CONFIG_NET_ENABLE
    case VIDEO_SERVER_NET_ERR:
        log_e("\nVIDEO_SERVER_NET_ERR\n");
        init_intent(&it);
        it.data = &mark;
        net_video_rec_stop(0);
        set_net_video_rec_state(0);
        break;
#endif

    default :
        log_e("unknow rec server cmd %x , %x!\n", argv[0], (int)priv);
        break;
    }
}

static void ve_server_event_handler(void *priv, int argc, int *argv)
{
    struct device_event eve = {0};
    eve.arg = "mot";
    switch (argv[0]) {
    case VE_MSG_MOTION_DETECT_STILL:
        /*
         *录像时，移动侦测打开的情况下，画面基本静止20秒，则进入该分支
         */
        printf("**************VE_MSG_MOTION_DETECT_STILL**********\n");
        if (!db_select("mot") || (__this->menu_inout)) {
            return;
        }
        if (__this->state == VIDREC_STA_START && __this->user_rec == 0) {
            video_rec_stop(0);
        }
        eve.event = DEVICE_EVENT_CHANGE;
        eve.value = false;
        device_event_notify(DEVICE_EVENT_FROM_VIDEO, &eve);

        break;
    case VE_MSG_MOTION_DETECT_MOVING:
        /*
         *移动侦测打开，当检测到画面活动一段时间，则进入该分支去启动录像
         */
        printf("**************VE_MSG_MOTION_DETECT_MOVING**********\n");
        if (!db_select("mot") || (__this->menu_inout)) {
            return;
        }
        if ((__this->state == VIDREC_STA_STOP) || (__this->state == VIDREC_STA_IDLE)) {
            video_rec_start();
        }
        eve.event = DEVICE_EVENT_CHANGE;
        eve.value = true;
        device_event_notify(DEVICE_EVENT_FROM_VIDEO, &eve);

        break;
    case VE_MSG_LANE_DETECT_WARNING:

        if (!__this->lan_det_setting) {
            if (!db_select("lan")) {
                return;
            }
        }

        /* play_voice_file("mnt/spiflash/audlogo/lane.adp"); */

        puts("==lane dete waring==\n");
        break;
    case VE_MSG_LANE_DETCET_LEFT:
        puts("==lane dete waring==l\n");
        break;
    case VE_MSG_LANE_DETCET_RIGHT:
        puts("==lane dete waring==r\n");
        break;
    case VE_MSG_VEHICLE_DETECT_WARNING:
        //printf("x = %d,y = %d,w = %d,hid = %d\n",argv[1],argv[2],argv[3],argv[4]);
        //位置
        video_rec_post_msg("carpos:p=%4", ((u32)(argv[1]) | (argv[2] << 16))); //x:x y:y
        //颜色
        if (argv[3] > 45) {
            video_rec_post_msg("carpos:w=%4", ((u32)(argv[3]) | (3 << 16)));    //w:width c:color,0:transparent, 1:green,2:yellow,3:red
        } else {
            video_rec_post_msg("carpos:w=%4", ((u32)(argv[3]) | (1 << 16)));
        }
        //隐藏
        if (argv[4] == 0) {
            video_rec_post_msg("carpos:w=%4", ((u32)(1) | (0 << 16)));
        }
        //刷新
        video_rec_post_msg("carpos:s=%4", 1);
        break;
    default :
        break;
    }
#ifdef CONFIG_NET_ENABLE
    net_video_rec_status_notify();
#endif


}

/*
 *智能引擎服务打开，它包括移动侦测等一些功能,在打开这些功能之前，必须要打开这个智能引擎服务
 */
static s32 ve_server_open(u8 fun_sel)
{
    if (!__this->video_engine) {
        __this->video_engine = server_open("video_engine_server", NULL);
        if (!__this->video_engine) {
            puts("video_engine_server:faild\n");
            return -1;
        }

        server_register_event_handler(__this->video_engine, NULL, ve_server_event_handler);

        struct video_engine_req ve_req;
        ve_req.module = 0;
        ve_req.md_mode = 0;
        ve_req.cmd = 0;
        ve_req.hint_info.hint = (1 << VE_MODULE_MOTION_DETECT);
#ifdef CONFIG_LANE_DETECT_ENABLE
        ve_req.hint_info.hint |= (1 << VE_MODULE_LANE_DETECT);
        if (fun_sel) {
            ve_req.hint_info.hint = (1 << VE_MODULE_LANE_DETECT);
        }
#endif

#ifdef CONFIG_VE_MOTION_DETECT_MODE_ISP
        ve_req.hint_info.mode_hint0 = (VE_MOTION_DETECT_MODE_ISP
                                       << (VE_MODULE_MOTION_DETECT * 4));
#else
        ve_req.hint_info.mode_hint0 = (VE_MOTION_DETECT_MODE_NORMAL
                                       << (VE_MODULE_MOTION_DETECT * 4));
#endif

        ve_req.hint_info.mode_hint1 = 0;
        server_request(__this->video_engine, VE_REQ_SET_HINT, &ve_req);
    }

    if (fun_sel) {
        ve_lane_det_start(1);
    } else {
        __this->car_head_y = db_select("lan") & 0x0000ffff;
        __this->vanish_y   = (db_select("lan") >> 16) & 0x0000ffff;
        ve_mdet_start();
        ve_lane_det_start(0);
    }

    return 0;
}

static s32 ve_server_close()
{
    if (__this->video_engine) {

        if (!__this->lan_det_setting) {
            ve_mdet_stop();
        }
        ve_lane_det_stop(0);

        server_close(__this->video_engine);

        __this->video_engine = NULL;
    }
    return 0;
}

static int ve_mdet_start()
{
    struct video_engine_req ve_req;

    if ((__this->video_engine == NULL) || !db_select("mot")) {
        return -EINVAL;
    }

    ve_req.module = VE_MODULE_MOTION_DETECT;
#ifdef CONFIG_VE_MOTION_DETECT_MODE_ISP
    ve_req.md_mode = VE_MOTION_DETECT_MODE_ISP;
#else
    ve_req.md_mode = VE_MOTION_DETECT_MODE_NORMAL;
#endif
    ve_req.cmd = 0;

    server_request(__this->video_engine, VE_REQ_MODULE_OPEN, &ve_req);


    server_request(__this->video_engine, VE_REQ_MODULE_GET_PARAM, &ve_req);


    /*
    *移动侦测的检测启动时间和检测静止的时候
    **/
    ve_req.md_params.level = 2;
    ve_req.md_params.move_delay_ms = MOTION_START_SEC * 1000;
    ve_req.md_params.still_delay_ms = MOTION_STOP_SEC * 1000;
    server_request(__this->video_engine, VE_REQ_MODULE_SET_PARAM, &ve_req);

    server_request(__this->video_engine, VE_REQ_MODULE_START, &ve_req);

    return 0;
}


static int ve_mdet_stop()
{
    struct video_engine_req ve_req;

    if ((__this->video_engine == NULL) || !db_select("mot")) {
        return -EINVAL;
    }

    ve_req.module = VE_MODULE_MOTION_DETECT;
#ifdef CONFIG_VE_MOTION_DETECT_MODE_ISP
    ve_req.md_mode = VE_MOTION_DETECT_MODE_ISP;
#else
    ve_req.md_mode = VE_MOTION_DETECT_MODE_NORMAL;
#endif

    ve_req.cmd = 0;
    server_request(__this->video_engine, VE_REQ_MODULE_STOP, &ve_req);

    server_request(__this->video_engine, VE_REQ_MODULE_CLOSE, &ve_req);
    return 0;
}

static void ve_mdet_reset()
{
    ve_mdet_stop();
    ve_mdet_start();
}


static int ve_lane_det_start(u8 fun_sel)
{
    struct video_engine_req ve_req;

#ifdef CONFIG_LANE_DETECT_ENABLE
    if (!fun_sel) {
        if ((__this->video_engine == NULL) || !db_select("lan")) {
            return -EINVAL;
        }
    }

    ve_req.module = VE_MODULE_LANE_DETECT;
#ifdef CONFIG_VE_MOTION_DETECT_MODE_ISP
    ve_req.md_mode = VE_MOTION_DETECT_MODE_ISP;
#else
    ve_req.md_mode = VE_MOTION_DETECT_MODE_NORMAL;
#endif

    ve_req.cmd = 0;
    server_request(__this->video_engine, VE_REQ_MODULE_OPEN, &ve_req);

    server_request(__this->video_engine, VE_REQ_MODULE_GET_PARAM, &ve_req);

    /**
     *轨道偏移 配置车头 位置，视线结束为止，以及车道宽度
     * */
    /* ve_req.lane_detect_params.car_head_y = 230; */
    /* ve_req.lane_detect_params.vanish_y = 170; */
    /* ve_req.lane_detect_params.len_factor = 0; */
    ve_req.lane_detect_params.car_head_y = __this->car_head_y;
    ve_req.lane_detect_params.vanish_y = __this->vanish_y;
    ve_req.lane_detect_params.len_factor = 0;

    server_request(__this->video_engine, VE_REQ_MODULE_SET_PARAM, &ve_req);

    server_request(__this->video_engine, VE_REQ_MODULE_START, &ve_req);

#endif

    return 0;
}


static int ve_lane_det_stop(u8 fun_sel)
{
    struct video_engine_req ve_req;

#ifdef CONFIG_LANE_DETECT_ENABLE
    if (!fun_sel) {
        if ((__this->video_engine == NULL) || !db_select("lan")) {
            return -EINVAL;
        }
    }

    ve_req.module = VE_MODULE_LANE_DETECT;
#ifdef CONFIG_VE_MOTION_DETECT_MODE_ISP
    ve_req.md_mode = VE_MOTION_DETECT_MODE_ISP;
#else
    ve_req.md_mode = VE_MOTION_DETECT_MODE_NORMAL;
#endif

    ve_req.cmd = 0;

    server_request(__this->video_engine, VE_REQ_MODULE_STOP, &ve_req);

    server_request(__this->video_engine, VE_REQ_MODULE_CLOSE, &ve_req);
#endif

    return 0;
}

void ve_lane_det_reset()
{
    ve_lane_det_stop(0);
    ve_lane_det_start(0);
}

void ve_server_reopen()
{
    ve_mdet_stop();
    ve_lane_det_stop(0);

    ve_server_close();
    ve_server_open(0);
}

/*
 * 判断SD卡是否挂载成功和簇大小是否大于32K
 */
static int storage_device_available()
{
    struct vfs_partition *part;

    if (storage_device_ready() == 0) {
        if (!dev_online(SDX_DEV)) {
            video_rec_post_msg("noCard");
        } else {
            video_rec_post_msg("fsErr");
        }
        return false;
    } else {
        part = fget_partition(CONFIG_ROOT_PATH);
        printf("part_fs_attr: %x\n", part->fs_attr);
        if (part->clust_size < 32 || (part->fs_attr & F_ATTR_RO)) {
            video_rec_post_msg("fsErr");
            return false;
        }
        __this->total_size = part->total_size;
    }

    return true;
}

void video_rec_get_remain_time(void)
{
    static char retime_buf[30];
    int err;
    u32 cur_space = 0;
    u32 one_pic_size;
    int hour = 0, min = 0, sec = 0;
    int second = 0;
    u32 gap_time = db_select("gap");
    u8 fps = video_rec_get_fps();
    if (!fps) {
        fps = 25;
    }

    if (__this->state == VIDREC_STA_START) { /* 允许ui打开菜单 */
        return;
    }

    /*
     * 这里填入SD卡剩余录像时间
     */
    if (storage_device_available()) {
        log_d("calc_free_space...\n");
        err = fget_free_space(CONFIG_ROOT_PATH, &cur_space);
        if (err) {
            if (fget_err_code(CONFIG_ROOT_PATH) == -EIO) {
                video_rec_post_msg("fsErr");
            }
        } else {
            u32 res = db_select("res");
            if (res == VIDEO_RES_1080P) {
                one_pic_size = (0x21000 + 0xa000) / 1024;
            } else if (res == VIDEO_RES_720P) {
                one_pic_size = (0x13000 + 0xa000) / 1024;
            } else {
                one_pic_size = (0xa000 + 0xa000) / 1024;
            }
            printf("cur_space:%d one_pic_size:%d fps:%d", cur_space, one_pic_size, fps);
            hour = (cur_space / one_pic_size) / fps / 60 / 60;
            min = (cur_space / one_pic_size) / fps / 60 % 60;
            sec = (cur_space / one_pic_size) / fps % 60;
            second = hour * 3600 + min * 60 + sec;

            printf("retime_buf %02d:%02d:%02d", hour, min, sec);

            video_rec_post_msg("Remain:s=%4", ((u32)(second)));
        }
        log_d("calc_free_space_exit\n");
    } else {
        video_rec_post_msg("Remain:s=%4", 0);
    }
}

/*码率控制，根据具体分辨率设置*/
int video_rec_get_abr(u32 width)
{
    if (width <= 720) {
        /* return 8000; */
        return 4000;
    } else if (width <= 1280) {
        return 8000;
        /* return 10000; */
    } else if (width <= 1920) {
        return 10000;
    } else {
        return 8000;
    }
}

/*动态设置码流*/
static void video_rec_set_bitrate(struct server *video_server, unsigned int bits_rate)
{
    union video_req req = {0};

    /*录像默认使用0通道*/
    req.rec.channel = 0;

    req.rec.state = VIDEO_STATE_RESET_BITS_RATE;
    req.rec.abr_kbps = bits_rate;

    server_request(video_server, VIDEO_REQ_REC, &req);
}




/*
 *根据录像不同的时间和分辨率，设置不同的录像文件大小
 */
static u32 video_rec_get_fsize(u8 cycle_time, u16 vid_width, int format)
{
    u32 fsize;

    if (cycle_time == 0) {
        cycle_time = 5;
    }

    fsize = video_rec_get_abr(vid_width) * cycle_time * 10000;
    if (format == VIDEO_FMT_AVI) {
        fsize = fsize + fsize / 4;
    }

    return fsize;
}

static int video_rec_cmp_fname(void *afile, void *bfile)
{
    int alen, blen;
    char afname[MAX_FILE_NAME_LEN];
    char bfname[MAX_FILE_NAME_LEN];

    if ((afile == NULL) || (bfile == NULL)) {
        return 0;
    }
    printf("video_rec_cmp_fname: %p, %p\n", afile, bfile);

    alen = fget_name(afile, (u8 *)afname, MAX_FILE_NAME_LEN);
    if (alen <= 0) {
        log_e("fget_name: afile=%x\n", afile);
        return 0;
    }
    ASCII_ToUpper(afname, alen);

    blen = fget_name(bfile, (u8 *)bfname, MAX_FILE_NAME_LEN);
    if (blen <= 0) {
        log_e("fget_name: bfile=%x\n", bfile);
        return 0;
    }
    ASCII_ToUpper(bfname, blen);

    printf("afname: %s, bfname: %s\n", afname, bfname);

    if (alen == blen && !strcmp(afname, bfname)) {
        return 1;
    }

    return 0;
}

static void video_rec_fscan_release(int lock_dir)
{
    printf("video_rec_fscan_release: %d\n", lock_dir);
    for (int i = 0; i < CONFIG_VIDEO_REC_NUM; i++) {
        if (__this->fscan[lock_dir][i]) {
            fscan_release(__this->fscan[lock_dir][i]);
            __this->fscan[lock_dir][i] = NULL;
        }
    }
}

static void video_rec_fscan_dir(int id, int lock_dir, const char *path)
{
    const char *str;
#ifdef CONFIG_EMR_DIR_ENABLE
    str = "-tMOVAVIMP4 -sn";
#else
    str = lock_dir ? "-tMOVAVIMP4 -sn -ar" : "-tMOVAVIMP4 -sn -a/r";
#endif
    if (__this->fscan[lock_dir][id]) {
        if (__this->old_file_number[lock_dir][id] == 0) {
            puts("--------delete_all_scan_file\n");
            fscan_release(__this->fscan[lock_dir][id]);
            __this->fscan[lock_dir][id] = NULL;
        }
    }

    if (!__this->fscan[lock_dir][id]) {
        __this->fscan[lock_dir][id] = fscan(path, str, 9);
        if (!__this->fscan[lock_dir][id]) {
            __this->old_file_number[lock_dir][id] = 0;
        } else {
            __this->old_file_number[lock_dir][id] = __this->fscan[lock_dir][id]->file_number;
        }
        __this->file_number[lock_dir][id] = __this->old_file_number[lock_dir][id];
        printf("fscan_dir: %d, file_number = %d\n", id, __this->file_number[lock_dir][id]);
    }
}

static FILE *video_rec_get_first_file(int id)
{
    int max_index = -1;
    int max_file_number = 0;
    int persent = __this->lock_fsize * 100 / __this->total_size;
    int lock_dir = !!(persent > LOCK_FILE_PERCENT);

    log_d("lock_file_persent: %d, %d, size: %dMB\n", id, persent, __this->lock_fsize / 1024);

#ifdef CONFIG_VIDEO0_ENABLE
    video_rec_fscan_dir(0, lock_dir, rec_path[0][lock_dir]);
#endif
#ifdef CONFIG_VIDEO1_ENABLE
    video_rec_fscan_dir(1, lock_dir, rec_path[1][lock_dir]);
#endif
#ifdef CONFIG_VIDEO2_ENABLE
    video_rec_fscan_dir(2, lock_dir, rec_path[2][lock_dir]);
#endif
#ifdef CONFIG_VIDEO3_ENABLE
    video_rec_fscan_dir(3, lock_dir, rec_path[2][lock_dir]);
#endif
#ifdef CONFIG_VIDEO5_ENABLE
    video_rec_fscan_dir(4, lock_dir, rec_path[4][lock_dir]);
#endif
#ifdef CONFIG_VIDEO5_ENABLE
    video_rec_fscan_dir(5, lock_dir, rec_path[5][lock_dir]);
#endif


    for (int i = 0; i < CONFIG_VIDEO_REC_NUM; i++) {
        if (__this->fscan[lock_dir][i]) {
            if (max_file_number < __this->file_number[lock_dir][i]) {
                max_file_number = __this->file_number[lock_dir][i];
                max_index = i;
            }
        }
    }

    if (max_index < 0) {
        return NULL;
    }
    if (max_index != id && id >= 0) {
        /* 查看优先删除的文件夹是否满足删除条件 */
        if (__this->file_number[lock_dir][id] + 3 > __this->file_number[lock_dir][max_index]) {
            max_index = id;
        }
    }


    log_d("fselect file from dir %d, %d\n", lock_dir, max_index);


    if (__this->fscan[lock_dir][max_index]) {
        FILE *f = fselect(__this->fscan[lock_dir][max_index], FSEL_FIRST_FILE, 0);
        if (f) {

            if (lock_dir == 0) {
                if (video_rec_cmp_fname(__this->file[max_index], f)) {
                    fclose(f);
                    return NULL;
                }
            } else {
                __this->lock_fsize -= flen(f) / 1024;
                log_d("lock fsize - = %d\n", __this->lock_fsize);
            }

            __this->file_number[lock_dir][max_index]--;
            __this->old_file_number[lock_dir][max_index]--;
            if (__this->old_file_number[lock_dir][max_index] == 0) {
                video_rec_fscan_release(lock_dir);
            }
        }
#ifdef CONFIG_NET_ENABLE
        video_rec_delect_notify(f, -1);
#endif
        return f;
    } else {
        log_e("fscan[%d][%d] err", lock_dir, max_index);
        return NULL;
    }
    return NULL;
}


static void video_rec_rename_file(int id, FILE *file, int fsize, int format)
{
    char file_name[32];

    __this->new_file[id] = NULL;

#if 0
    int err = fcheck(file);
    if (err) {
        puts("fcheck fail\n");
#ifdef CONFIG_NET_ENABLE
        video_rec_delect_notify(file, -1);
#endif
        fdelete(file);
        return;
    }
#endif

    int size = flen(file);
    int persent = (size / 1024) * 100 / (fsize / 1024);

    printf("rename file: persent=%d, %d,%d\n", persent, size >> 20, fsize >> 20);

    if (persent >= 90 && persent <= 110) {
        sprintf(file_name, "%s%s", rec_dir[id][0], rec_file_name(format));

        printf("fmove: %d, %d, %s\n", id, format, file_name);

#ifdef CONFIG_NET_ENABLE
        video_rec_delect_notify(file, -1);
#endif

        int err = fmove(file, file_name, &__this->new_file[id], 1, strlen(file_name));
        if (err == 0) {
            fseek(__this->new_file[id], fsize, SEEK_SET);
            fseek(__this->new_file[id], 0, SEEK_SET);
            return;
        }
        puts("fmove_file_faild\n");
    }
#ifdef CONFIG_NET_ENABLE
    video_rec_delect_notify(file, -1);
#endif

    fdelete(file);
}

static int video_rec_create_file(int id, u32 fsize, int format, const char *path)
{
    FILE *file;
    int try_cnt = 0;
    char file_path[64];
    sprintf(file_path, "%s%s", path, rec_file_name(format));

    printf("fopen: %s, %dMB\n", file_path, fsize >> 20);

    do {
        file = fopen(file_path, "w+");
        if (!file) {
            log_e("fopen faild\n");
            break;
        }
        if (!fseek(file, fsize, SEEK_SET)) {
            goto __exit;
        }
        log_e("fseek faild\n");
        fdelete(file);

    } while (++try_cnt < 2);

    return -EIO;

__exit:
    fseek(file, 0, SEEK_SET);
    __this->new_file_size[id] = fsize;
    __this->new_file[id] = file;
    printf("id:%d size:%d file:%x", id, __this->new_file_size[id], __this->new_file[id]);

    return 0;
}

static int video_rec_del_old_file()
{
    int i;
    int err;
    FILE *file;
    int fsize[] = {0, 0, 0, 0, 0, 0};
    u32 cur_space;
    u32 need_space = 0;
    u32 gap_time = db_select("gap");
    int cyc_time = db_select("cyc");
    int format[] = { VIDEO0_REC_FORMAT, VIDEO1_REC_FORMAT, VIDEO2_REC_FORMAT, VIDEO3_REC_FORMAT, VIDEO4_REC_FORMAT, VIDEO5_REC_FORMAT};

#ifdef CONFIG_VIDEO0_ENABLE
    if (!__this->new_file[0]) {
        fsize[0] =  video_rec_get_fsize(cyc_time, rec_pix_w[db_select("res")], VIDEO0_REC_FORMAT);
        if (gap_time) {
            fsize[0] = fsize[0] / (30 * gap_time / 1000);
        }
        need_space += fsize[0];
    }
#endif
#ifdef CONFIG_VIDEO4_ENABLE
    if (!__this->new_file[0]) {
        fsize[4] =  video_rec_get_fsize(cyc_time, rec_pix_w[db_select("res")], VIDEO4_REC_FORMAT);
        if (gap_time) {
            fsize[4] = fsize[4] / (30 * gap_time / 1000);
        }
        need_space += fsize[4];
    }
#endif

    if (db_select("two")) {
#ifdef CONFIG_VIDEO1_ENABLE
        if (__this->video_online[1] && !__this->new_file[1]) {
            fsize[1] =  video_rec_get_fsize(cyc_time, rec_pix_w[db_select("res")], VIDEO1_REC_FORMAT);
            if (gap_time) {
                fsize[1] = fsize[1] / (30 * gap_time / 1000);
            }
            need_space += fsize[1];
        }
#endif

#ifdef CONFIG_VIDEO2_ENABLE
        if (__this->video_online[2] && !__this->new_file[2]) {
            fsize[2] =  video_rec_get_fsize(cyc_time, UVC_ENC_WIDTH, VIDEO2_REC_FORMAT);
            //数字后拉不支持间隔录像
            /* if(gap_time){ */
            /* } */
            need_space += fsize[2];
        }
#endif

#ifdef CONFIG_VIDEO3_ENABLE
        if (__this->video_online[3] && !__this->new_file[3]) {
            fsize[3] =  video_rec_get_fsize(cyc_time, rec_pix_w[db_select("res")], VIDEO3_REC_FORMAT);
            if (gap_time) {
                fsize[3] = fsize[3] / (30 * gap_time / 1000);
            }
            need_space += fsize[3];
        }
#endif

#ifdef CONFIG_VIDEO5_ENABLE
        if (__this->video_online[5] && !__this->new_file[5]) {
            fsize[5] =  video_rec_get_fsize(cyc_time, rec_pix_w[db_select("res")], VIDEO5_REC_FORMAT);
            if (gap_time) {
                fsize[5] = fsize[5] / (30 * gap_time / 1000);
            }
            need_space += fsize[5];
        }
#endif
    }



    err = fget_free_space(CONFIG_ROOT_PATH, &cur_space);
    if (err) {
        return err;
    }

    printf("space: %dMB, %dMB\n", cur_space / 1024, need_space / 1024 / 1024);

    if (cur_space >= 3 * (need_space / 1024)) {
        for (i = 0; i < CONFIG_VIDEO_REC_NUM; i++) {
            if (fsize[i] != 0) {
                printf("fsize:%d  format:%d  rec_path:%s", fsize[i], format[i], rec_path[i][0]);
                err = video_rec_create_file(i, fsize[i], format[i], rec_path[i][0]);
                if (err) {
                    return err;
                }
            }
        }
        return 0;
    }


    while (1) {
        if (cur_space >= (need_space / 1024) * 2) {
            break;
        }
        for (i = 0; i < CONFIG_VIDEO_REC_NUM; i++) {
            if (fsize[i] != 0) {
                file = video_rec_get_first_file(i);
                if (!file) {
                    return -ENOMEM;
                }
                fdelete(file);
            }
        }
        fget_free_space(CONFIG_ROOT_PATH, &cur_space);
        printf("Delete file ,remain space:%d", cur_space);
    }

    for (i = 0; i < CONFIG_VIDEO_REC_NUM; i++) {
        if (fsize[i] != 0) {
            file = video_rec_get_first_file(i);
            if (file) {
                video_rec_rename_file(i, file, fsize[i], format[i]);
            }
            if (!__this->new_file[i]) {
                err = video_rec_create_file(i, fsize[i], format[i], rec_path[i][0]);
                if (err) {
                    return err;
                }
            }
        }
    }

    return 0;
}



static int video_rec_scan_lock_file()
{
#ifdef CONFIG_EMR_DIR_ENABLE
    const char *str = "-tMOVAVIMP4 -sn";
#else
    const char *str = "-tMOVAVIMP4 -sn -ar";
#endif

    __this->lock_fsize = 0;
    for (int i = 0; i < ARRAY_SIZE(rec_path); i++) {
        __this->fscan[1][i] = fscan(rec_path[i][1], str, 9);
        if (__this->fscan[1][i] == NULL) {
            continue;
        }
        int sel_mode = FSEL_FIRST_FILE;
        while (1) {
            FILE *file = fselect(__this->fscan[1][i], sel_mode, 0);
            if (!file) {
                break;
            }
            __this->lock_fsize += (flen(file) / 1024);
            sel_mode = FSEL_NEXT_FILE;
            fclose(file);
        }
    }
    log_d("lock_file_size: %dMB\n", __this->lock_fsize / 1024);

    return 0;
}

/*
 *设置保护文件，必须要在关闭文件之前调用
 */
static int video_rec_lock_file(void *file, u8 lock)
{
    int attr;

    if (!file) {
        puts("lock file null\n");
        return -1;
    }

    fget_attr(file, &attr);

    if (lock) {
        if (attr & F_ATTR_RO) {
            return 0;
        }
        attr |= F_ATTR_RO;
    } else {
        if (!(attr & F_ATTR_RO)) {
            return 0;
        }
        attr &= ~F_ATTR_RO;
    }
    fset_attr(file, attr);

    return 0;
}

static void find_lock_file_to_move(int dev_id)
{
    char fpath[128];
    sprintf(fpath, CONFIG_ROOT_PATH"%s", rec_dir[dev_id][0]);
    struct vfscan *fs = fscan(fpath, "-tAVI -sn -ar", 9);
    FILE *file = NULL;
    if (fs) {
        file = fselect(fs, FSEL_LAST_FILE, 0);
        if (file) {
            int err = fmove(file, rec_dir[dev_id][1], NULL, 0, strlen(rec_dir[dev_id][1]));
            if (!err) {
                log_d("fmove sucess\n");
            }
        }
    }

    fscan_release(fs);
}
static void video_rec_close_file(int dev_id)
{
    if (!__this->file[dev_id]) {
        return;
    }
#ifdef CONFIG_NET_ENABLE
    char is_emf = 0;

    char *path = video_rec_finish_get_name(__this->file[dev_id], dev_id, is_emf);
#endif

    if (__this->gsen_lock & BIT(dev_id)) {
        __this->gsen_lock &= ~BIT(dev_id);
        __this->lock_fsize += flen(__this->file[dev_id]) / 1024;
        video_rec_lock_file(__this->file[dev_id], 1);
        video_rec_post_msg("unlockREC");

#ifdef CONFIG_NET_ENABLE
#ifdef CONFIG_EMR_DIR_ENABLE
        is_emf = TRUE;
#endif
        path = video_rec_finish_get_name(__this->file[dev_id], dev_id, is_emf);
#endif
        fclose(__this->file[dev_id]);
        __this->file[dev_id] = NULL;
#ifdef CONFIG_EMR_DIR_ENABLE
        find_lock_file_to_move(dev_id);
#endif
    }
    if (__this->file[dev_id]) {
        fclose(__this->file[dev_id]);
    }
    __this->file[dev_id] = NULL;

#ifdef CONFIG_NET_ENABLE
    if (path) { //必须关闭文件之后才能调用，否则在读取文件信息不全！！！
        video_rec_finish_notify(path);
    }
#endif
}



#ifdef CONFIG_VIDEO0_ENABLE
/******* 不要单独调用这些子函数 ********/
static int video0_rec_start()
{
    int err;
    union video_req req = {0};
    struct video_text_osd text_osd;
    struct video_graph_osd graph_osd;
    u16 max_one_line_strnum;
    u16 osd_line_num;
    u16 osd_max_heigh;

    puts("start_video_rec0\n");
    if (!__this->video_rec0) {
        __this->video_rec0 = server_open("video_server", "video0.0");
        if (!__this->video_rec0) {
            return VREC_ERR_V0_SERVER_OPEN;
        }

        server_register_event_handler(__this->video_rec0, (void *)0, rec_dev_server_event_handler);
    }

    u32 res = db_select("res");
    /*
     *通道号，分辨率，封装格式，写卡的路径
     */

    req.rec.online      = 1;
    req.rec.enable_dri  = 0;
    req.rec.channel     = 0;
    req.rec.camera_type = VIDEO_CAMERA_NORMAL;
    req.rec.width 	    = rec_pix_w[res];
    req.rec.height 	    = rec_pix_h[res];
    req.rec.format 	    = VIDEO0_REC_FORMAT;
    req.rec.state 	    = VIDEO_STATE_START;
    req.rec.file        = __this->file[0];
#ifdef CONFIG_NET_ENABLE
    req.rec.fsize = __this->new_file_size[0];
#endif


    /*
     *帧率为0表示使用摄像头的帧率
     */
    req.rec.quality     = VIDEO_MID_Q;
#ifdef CONFIG_NET_ENABLE
    req.rec.fps 	    = 0;
    req.rec.real_fps 	= video_rec_get_fps();
#else
    req.rec.fps 	    = 0;
    req.rec.real_fps 	= 0;
#endif

    /*
     *采样率，通道数，录像音量，音频使用的循环BUF,录不录声音
     */
#ifdef CONFIG_NET_ENABLE
    req.rec.audio.sample_rate = video_rec_get_audio_sampel_rate();
#else
    req.rec.audio.sample_rate = 8000;
#endif
    req.rec.audio.channel 	= 1;
    req.rec.audio.volume    = AUDIO_VOLUME;
    req.rec.audio.buf = __this->audio_buf[0];
    req.rec.audio.buf_len = AUDIO0_BUF_SIZE;

    /* req.rec.audio.sample_source = "mic"; */
    //req.rec.audio.sample_source = "user";
    /* req.rec.audio.fmt_format = AUDIO_FMT_PCM; */
    req.rec.pkg_mute.aud_mute = !db_select("mic");

    /*
     *码率，I帧和P帧比例，必须是偶数（当录MOV的时候才有效）,
     *roio_xy :值表示宏块坐标， [6:0]左边x坐标 ，[14:8]右边x坐标，[22:16]上边y坐标，[30:24]下边y坐标,写0表示1个宏块有效
     * roio_ratio : 区域比例系数
     */
    req.rec.abr_kbps = video_rec_get_abr(req.rec.width);

#if defined __CPU_AC5401__
    req.rec.IP_interval = 0;
#elif defined __CPU_AC5601__
    if (req.rec.height > 720) {
        req.rec.IP_interval = 1;
    } else {
        req.rec.IP_interval = 0;
    }
#endif

    /*感兴趣区域为下方 中间 2/6 * 4/6 区域，可以调整
    	感兴趣区域qp 为其他区域的 70% ，可以调整
    */
    /* req.rec.roi.roio_xy = (req.rec.height * 5 / 6 / 16) << 24 | (req.rec.height * 3 / 6 / 16) << 16 | (req.rec.width * 5 / 6 / 16) << 8 | (req.rec.width) * 1 / 6 / 16; */
    /* req.rec.roi.roi1_xy = (req.rec.height * 11 / 12 / 16) << 24 | (req.rec.height * 4 / 12 / 16) << 16 | (req.rec.width * 11 / 12 / 16) << 8 | (req.rec.width) * 1 / 12 / 16; */
    /* req.rec.roi.roi2_xy = 0; */
    /* req.rec.roi.roi3_xy = (1 << 24) | (0 << 16) | ((req.rec.width / 16) << 8) | 0; */
    /* req.rec.roi.roio_ratio = 256 * 70 / 100 ; */
    /* req.rec.roi.roio_ratio1 = 256 * 90 / 100; */
    /* req.rec.roi.roio_ratio2 = 0; */
    /* req.rec.roi.roio_ratio3 = 256 * 80 / 100; */


    /*
     * osd 相关的参数，注意坐标位置，x要64对齐，y要16对齐,底下例子是根据图像大小偏移到右下
     */
    text_osd.font_w = 16;
    text_osd.font_h = 32;

    max_one_line_strnum = strlen(video_rec_osd_buf);//21;
    osd_line_num = 1;
    if (db_select("num")) {
        osd_line_num = 2;
    }
    osd_max_heigh = (req.rec.height == 1088) ? 1080 : req.rec.height ;
    text_osd.x = (req.rec.width - max_one_line_strnum * text_osd.font_w) / 64 * 64;
    text_osd.y = (osd_max_heigh - text_osd.font_h * osd_line_num) / 16 * 16;
    text_osd.color[0] = 0x057d88;
    text_osd.color[1] = 0xe20095;
    text_osd.color[2] = 0xe20095;
    text_osd.bit_mode = 2;
    text_osd.text_format = video_rec_osd_buf;
    text_osd.font_matrix_table = osd_str_total;
    text_osd.font_matrix_base = osd2_str_matrix;
    text_osd.font_matrix_len = sizeof(osd2_str_matrix);
    text_osd.direction = 1;





#ifdef CONFIG_OSD_LOGO
    graph_osd.bit_mode = 16;//2bit的osd需要配置3个color
    graph_osd.x = 0;
    graph_osd.y = 0;
    graph_osd.width = 256;
    graph_osd.height = 256;
    graph_osd.icon = icon_16bit_data;
    graph_osd.icon_size = sizeof(icon_16bit_data);
#endif
    req.rec.text_osd = NULL;
    req.rec.graph_osd = NULL;
    if (db_select("dat")) {
        req.rec.text_osd = &text_osd;
#ifdef CONFIG_OSD_LOGO
        req.rec.graph_osd = &graph_osd;
#endif
    }

    /*
     *慢动作倍数(与延时摄影互斥,无音频); 延时录像的间隔ms(与慢动作互斥,无音频)
     */
    req.rec.slow_motion = 0;
    if (req.rec.camera_type != VIDEO_CAMERA_UVC) {
        req.rec.tlp_time = db_select("gap");
        if (req.rec.tlp_time) {
            req.rec.real_fps = 1000 / req.rec.tlp_time;
            req.rec.pkg_fps = video_rec_get_fps();
        }
    } else {
        req.rec.tlp_time = 0;
    }

    if (req.rec.slow_motion || req.rec.tlp_time) {
        req.rec.audio.sample_rate = 0;
        req.rec.audio.channel 	= 0;
        req.rec.audio.volume    = 0;
        req.rec.audio.buf = 0;
        req.rec.audio.buf_len = 0;
    }
    req.rec.buf = __this->video_buf[0];
    req.rec.buf_len = VREC0_FBUF_SIZE;

#ifdef CONFIG_FILE_PREVIEW_ENABLE
    req.rec.rec_small_pic 	= 1;
#else
    req.rec.rec_small_pic 	= 0;
#endif

    /*
     *循环录像时间，文件大小
     */
    req.rec.cycle_time = db_select("cyc");
    if (req.rec.cycle_time == 0) {
        req.rec.cycle_time = 5;
    }

#ifdef DIGITAL_SCALE
    req.rec.src_crop_enable = VIDEO_CROP_ENABNLE | VIDEO_LARGE_IMAGE;////支持通过配置比例对IMC源数据进行裁剪,使用数字变焦需要写1
    req.rec.sca.x = (1280 - x_offset) / 2;
    req.rec.sca.y = (720 - y_offset) / 2;
    req.rec.sca.tar_w = x_offset;
    req.rec.sca.tar_h = y_offset;
#else
    /* req.rec.src_crop_enable = 0; */
#endif
#if THREE_WAY_DOUBLE_RAW
    req.rec.double_raw      = 1;
#endif

    req.rec.cycle_time = req.rec.cycle_time * 60;

#if CAMERA_THUMBNAIL_ENABLE
#endif

    err = server_request(__this->video_rec0, VIDEO_REQ_REC, &req);
    if (err != 0) {
        puts("\n\n\nstart rec err\n\n\n");
        return VREC_ERR_V0_REQ_START;
    }

    __this->state = VIDREC_STA_START;

    return 0;
}

static int video0_rec_aud_mute()
{
    union video_req req;

    if (!__this->video_rec0) {
        return -EINVAL;
    }

    req.rec.channel = 0;
    req.rec.state 	= VIDEO_STATE_PKG_MUTE;
    req.rec.pkg_mute.aud_mute = !db_select("mic");

    return server_request(__this->video_rec0, VIDEO_REQ_REC, &req);
}

static int video0_rec_set_dr()
{
    union video_req req = {0};

    if (!__this->video_rec0) {
        return -EINVAL;
    }

    req.rec.real_fps = 7;
    req.rec.channel = 0;
    req.rec.state 	= VIDEO_STATE_SET_DR;

    return server_request(__this->video_rec0, VIDEO_REQ_REC, &req);

}


static int video0_rec_stop(u8 close)
{
    union video_req req;
    int err;

    log_d("video0_rec_stop\n");


    if (__this->video_rec0) {
        req.rec.channel = 0;
        req.rec.state = VIDEO_STATE_STOP;
        err = server_request(__this->video_rec0, VIDEO_REQ_REC, &req);
        if (err != 0) {
            printf("\nstop rec err 0x%x\n", err);
            return VREC_ERR_V0_REQ_STOP;
        }
    }

    video_rec_close_file(0);

    if (close) {
        if (__this->video_rec0) {
            server_close(__this->video_rec0);
            __this->video_rec0 = NULL;
        }
    }

    if (__this->thumb_buf) {
        free(__this->thumb_buf);
        __this->thumb_buf = NULL;
    }

    return 0;
}

/*
 *注意：循环录像的时候，虽然要重新传参，但是要和start传的参数保持一致！！！
 */
static int video0_rec_savefile()
{
    union video_req req = {0};
    int err;

    if (!__this->file[0]) {
        return -ENOENT;
    }

    u32 res = db_select("res");

    req.rec.channel = 0;
    req.rec.width 	= rec_pix_w[res];
    req.rec.height 	= rec_pix_h[res];
    req.rec.format 	= VIDEO0_REC_FORMAT;
    req.rec.state 	= VIDEO_STATE_SAVE_FILE;
    req.rec.file    = __this->file[0];

#ifdef CONFIG_NET_ENABLE
    req.rec.fps 	    = 0;
    req.rec.real_fps 	= video_rec_get_fps();
#else
    req.rec.fps 	    = 0;
    req.rec.real_fps 	= 0;
#endif

    req.rec.cycle_time = db_select("cyc");
    if (req.rec.cycle_time == 0) {
        req.rec.cycle_time = 5;
    }

    req.rec.cycle_time = req.rec.cycle_time * 60;

#if CAMERA_THUMBNAIL_ENABLE
    struct jpg_thumbnail thumbnails;
    err = camera_take_thumbnail(0);
    if (!err) {
        log_i("add thumbnail\n");
        thumbnails.enable = 1;
        thumbnails.buf = __this->thumbnail_img_buf;
        thumbnails.len = __this->thumbnail_img_size;
        req.rec.thumbnails = &thumbnails;
    }
#endif



    /*
     *采样率，通道数，录像音量，音频使用的循环BUF,录不录声音
     */
#ifdef CONFIG_NET_ENABLE
    req.rec.audio.sample_rate = video_rec_get_audio_sampel_rate();
#else
    req.rec.audio.sample_rate = 8000;
#endif
    req.rec.audio.channel 	= 1;
    req.rec.audio.volume    = AUDIO_VOLUME;
    req.rec.pkg_mute.aud_mute = !db_select("mic");

    req.rec.tlp_time = db_select("gap");
    if (req.rec.tlp_time) {
        req.rec.real_fps = 1000 / req.rec.tlp_time;
        req.rec.pkg_fps = video_rec_get_fps();
    }



    err = server_request(__this->video_rec0, VIDEO_REQ_REC, &req);
    if (err != 0) {
        log_e("rec0_save_file: err=%d\n", err);
        return err;
    }

    return 0;
}

static void video0_rec_close()
{
    if (__this->video_rec0) {
        server_close(__this->video_rec0);
        __this->video_rec0 = NULL;
    }
}

/*
 *必须在启动录像之后才可调用该函数，并且确保启动录像时已经打开了osd
 *新设置的osd的整体结构要和启动录像时一样，只是内容改变!!!
 */
static int video0_rec_set_osd_str(char *str)
{
    union video_req req;
    int err;
    if (!__this->video_rec0) {
        return -1;
    }

    req.rec.channel = 0;
    req.rec.state 	= VIDEO_STATE_SET_OSD_STR;
    req.rec.new_osd_str = str;
    err = server_request(__this->video_rec0, VIDEO_REQ_REC, &req);
    if (err != 0) {
        printf("\nset osd rec0 str err 0x%x\n", err);
        return -1;
    }

    return 0;
}

static int video0_rec_osd_ctl(u8 onoff)
{
    union video_req req;
    struct video_text_osd text_osd;
    int err;

    if (__this->video_rec0) {

        u32 res = db_select("res");
        req.rec.width 	= rec_pix_w[res];
        req.rec.height 	= rec_pix_h[res];

        text_osd.font_w = 16;
        text_osd.font_h = 32;
        text_osd.x = 0;//(req.rec.width - strlen(video_rec_osd_buf) * text_osd.font_w) / 64 * 64;
        text_osd.y = (req.rec.height - text_osd.font_h) / 16 * 16;
        /* text_osd.osd_yuv = 0xe20095; */
        /* text_osd.osd_yuv = 0xe20095; */
        text_osd.color[0] = 0xe20095;
        /* text_osd.color[0] = 0x057d88; */
        /* text_osd.color[1] = 0xe20095; */
        /* text_osd.color[2] = 0xe20095; */
        text_osd.bit_mode = 1;


        text_osd.text_format = video_rec_osd_buf;
        text_osd.font_matrix_table = osd_str_total;
        text_osd.font_matrix_base = osd_str_matrix;
        text_osd.font_matrix_len = sizeof(osd_str_matrix);
#ifdef __CPU_AC521x__
        text_osd.direction = 1;
#else
        text_osd.direction = 0;
#endif
        req.rec.text_osd = 0;
        if (onoff) {
            req.rec.text_osd = &text_osd;
        }
        req.rec.channel = 0;
        req.rec.state 	= VIDEO_STATE_SET_OSD;

        err = server_request(__this->video_rec0, VIDEO_REQ_REC, &req);
        if (err != 0) {
            printf("\nset osd rec0 err 0x%x\n", err);
            return -1;
        }
    }

    return 0;
}
#endif









/******* 不要单独调用这些子函数 ********/
#ifdef CONFIG_VIDEO1_ENABLE
static int video1_rec_start()
{
    int err;
    union video_req req = {0};
    struct video_text_osd text_osd;
    struct video_graph_osd graph_osd;
    u16 max_one_line_strnum;
    u16 osd_line_num;
    u16 osd_max_heigh;

    puts("start_video_rec1 \n");
    if (!__this->video_rec1) {
        __this->video_rec1 = server_open("video_server", "video1.0");
        if (!__this->video_rec1) {
            return VREC_ERR_V1_SERVER_OPEN;
        }

        server_register_event_handler(__this->video_rec1, (void *)1, rec_dev_server_event_handler);
    }

    u32 res = db_select("res");

    req.rec.online  = 1;
    req.rec.enable_dri  = 0;
    req.rec.channel = 0;
    req.rec.camera_type = VIDEO_CAMERA_NORMAL;
    req.rec.width 	    = rec_pix_w[res];
    req.rec.height 	    = rec_pix_h[res];
    req.rec.format 	= VIDEO1_REC_FORMAT;
    req.rec.state 	= VIDEO_STATE_START;
    req.rec.file    = __this->file[1];
    req.rec.quality = VIDEO_LOW_Q;
#ifdef CONFIG_NET_ENABLE
    req.rec.fps 	    = 0;
    req.rec.real_fps 	= video_rec_get_fps();
#else
    req.rec.fps 	    = 0;
    req.rec.real_fps 	= 0;
#endif



#ifdef CONFIG_NET_ENABLE

    req.rec.audio.sample_rate = video_rec_get_audio_sampel_rate();
#else

    req.rec.audio.sample_rate = 8000;
#endif
    req.rec.audio.channel 	= 1;
    req.rec.audio.volume    = 100;
    req.rec.audio.buf = __this->audio_buf[1];
    req.rec.audio.buf_len = AUDIO1_BUF_SIZE;
    req.rec.pkg_mute.aud_mute = !db_select("mic");

    req.rec.abr_kbps = video_rec_get_abr(req.rec.width);
    req.rec.IP_interval = 0;

    /*感兴趣区域为下方 中间 2/6 * 4/6 区域，可以调整
    	感兴趣区域qp 为其他区域的 70% ，可以调整
    */
    /* req.rec.roi.roio_xy = (req.rec.height * 5 / 6 / 16) << 24 | (req.rec.height * 3 / 6 / 16) << 16 | (req.rec.width * 5 / 6 / 16) << 8 | (req.rec.width) * 1 / 6 / 16; */
    /* req.rec.roi.roio_ratio = 256 * 70 / 100 ; */
    /* req.rec.roi.roi1_xy = 0; */
    /* req.rec.roi.roi2_xy = 0; */
    /* req.rec.roi.roi3_xy = (1 << 24) | (0 << 16) | ((req.rec.width / 16) << 8) | 0; */
    /* req.rec.roi.roio_ratio1 = 0; */
    /* req.rec.roi.roio_ratio2 = 0; */
    /* req.rec.roi.roio_ratio3 = 256 * 80 / 100; */

    /*
     * osd 相关的参数，注意坐标位置，x要64对齐，y要16对齐,底下例子是根据图像大小偏移到右下
     */
    text_osd.font_w = 16;
    text_osd.font_h = 32;
    max_one_line_strnum = strlen(video_rec_osd_buf);//21;

    osd_line_num = 1;
    if (db_select("num")) {
        osd_line_num = 2;
    }

    osd_max_heigh = (req.rec.height == 1088) ? 1080 : req.rec.height ;
    text_osd.x = (req.rec.width - max_one_line_strnum * text_osd.font_w) / 64 * 64;
    text_osd.y = (osd_max_heigh - text_osd.font_h * osd_line_num) / 16 * 16;
    text_osd.color[0] = 0x057d88;
    text_osd.color[1] = 0xe20095;
    text_osd.color[2] = 0xe20095;
    text_osd.bit_mode = 2;
    text_osd.text_format = video_rec_osd_buf;
    text_osd.font_matrix_table = osd_str_total;
    text_osd.font_matrix_base = osd2_str_matrix;
    text_osd.font_matrix_len = sizeof(osd2_str_matrix);
    text_osd.direction = 1;





#ifdef CONFIG_OSD_LOGO
    graph_osd.bit_mode = 16;//2bit的osd需要配置3个color
    graph_osd.x = 0;
    graph_osd.y = 0;
    graph_osd.width = 256;
    graph_osd.height = 256;
    graph_osd.icon = icon_16bit_data;
    graph_osd.icon_size = sizeof(icon_16bit_data);
#endif
    req.rec.text_osd = NULL;
    req.rec.graph_osd = NULL;
    if (db_select("dat")) {
        req.rec.text_osd = &text_osd;
#ifdef CONFIG_OSD_LOGO
        req.rec.graph_osd = &graph_osd;
#endif
    }




    req.rec.slow_motion = 0;
    if (req.rec.camera_type != VIDEO_CAMERA_UVC) {
        req.rec.tlp_time = db_select("gap");
        if (req.rec.tlp_time) {
            req.rec.real_fps = 1000 / req.rec.tlp_time;
            req.rec.pkg_fps = video_rec_get_fps();
        }
    } else {
        req.rec.tlp_time = 0;
    }

    if (req.rec.slow_motion || req.rec.tlp_time) {
        req.rec.audio.sample_rate = 0;
        req.rec.audio.channel 	= 0;
        req.rec.audio.volume    = 0;
        req.rec.audio.buf = 0;
        req.rec.audio.buf_len = 0;
    }
#ifdef CONFIG_PSRAM_ENABLE
    req.rec.extbuf_dev.enable = 1;
    req.rec.extbuf_dev.name = "spiram";
    req.rec.extbuf_dev.size = __PSRAM_SIZE__ / 2;
    req.rec.extbuf_dev.addr = __PSRAM_SIZE__ / 2;
#endif
    req.rec.buf = __this->video_buf[1];
    req.rec.buf_len = VREC1_FBUF_SIZE;

#ifdef CONFIG_FILE_PREVIEW_ENABLE
    req.rec.rec_small_pic 	= 1;
#else
    req.rec.rec_small_pic 	= 0;
#endif

#if (DOUBLE_720 == 1)
    req.rec.double720 = true;
#endif

#if (DOUBLE_720_SMALL_SCR == 1)
    req.rec.double720_small_scr = true;
#endif


    req.rec.cycle_time = db_select("cyc");
    if (req.rec.cycle_time == 0) {
        req.rec.cycle_time = 5;
    }

    req.rec.cycle_time = req.rec.cycle_time * 60;

#if CAMERA_THUMBNAIL_ENABLE
    struct jpg_thumbnail thumbnails;
    err = camera_take_thumbnail(1);
    if (!err) {
        log_i("add thumbnail\n");
        thumbnails.enable = 1;
        thumbnails.buf = __this->thumbnail_img_buf;
        thumbnails.len = __this->thumbnail_img_size;
        req.rec.thumbnails = &thumbnails;
    }
#endif
#if THREE_WAY_DOUBLE_RAW
    req.rec.double_raw      = 1;
#endif


    err = server_request(__this->video_rec1, VIDEO_REQ_REC, &req);
    if (err != 0) {
        puts("\n\n\nstart rec2 err\n\n\n");
        return VREC_ERR_V1_REQ_START;
    }

    return 0;
}

static int video1_rec_aud_mute()
{
    union video_req req;

    if (!__this->video_rec1) {
        return -EINVAL;
    }

    req.rec.channel = 0;
    req.rec.state 	= VIDEO_STATE_PKG_MUTE;
    req.rec.pkg_mute.aud_mute = !db_select("mic");

    return server_request(__this->video_rec1, VIDEO_REQ_REC, &req);
}

static int video1_rec_set_dr()
{
    union video_req req = {0};

    if (!__this->video_rec1) {
        return -EINVAL;
    }

    req.rec.real_fps = 7;
    req.rec.channel = 0;
    req.rec.state 	= VIDEO_STATE_SET_DR;

    return server_request(__this->video_rec1, VIDEO_REQ_REC, &req);

}


static int video1_rec_stop(u8 close)
{
    union video_req req;
    int err;

    log_d("video1_rec_stop\n");

    if (__this->video_rec1) {
        req.rec.channel = 0;
        req.rec.state = VIDEO_STATE_STOP;
        err = server_request(__this->video_rec1, VIDEO_REQ_REC, &req);
        if (err != 0) {
            printf("\nstop rec2 err 0x%x\n", err);
            return VREC_ERR_V1_REQ_STOP;
        }
    }

    video_rec_close_file(1);

    if (close) {
        if (__this->video_rec1) {
            server_close(__this->video_rec1);
            __this->video_rec1 = NULL;
        }
    }

    if (__this->thumb_buf) {
        free(__this->thumb_buf);
        __this->thumb_buf = NULL;
    }


    return 0;
}

static int video1_rec_savefile()
{
    union video_req req = {0};
    int err;

    if (!__this->file[1]) {
        return -ENOENT;
    }

    u32 res = db_select("res");

    req.rec.channel = 0;
    req.rec.width 	= rec_pix_w[res];
    req.rec.height 	= rec_pix_h[res];
    req.rec.format 	= VIDEO0_REC_FORMAT;
    req.rec.state 	= VIDEO_STATE_SAVE_FILE;
    req.rec.file    = __this->file[1];

#ifdef CONFIG_NET_ENABLE
    req.rec.fps 	    = 0;
    req.rec.real_fps 	= video_rec_get_fps();
#else
    req.rec.fps 	    = 0;
    req.rec.real_fps 	= 0;
#endif

    req.rec.cycle_time = db_select("cyc");
    if (req.rec.cycle_time == 0) {
        req.rec.cycle_time = 5;
    }

    req.rec.cycle_time = req.rec.cycle_time * 60;

#if CAMERA_THUMBNAIL_ENABLE
    struct jpg_thumbnail thumbnails;
    err = camera_take_thumbnail(1);
    if (!err) {
        log_i("add thumbnail\n");
        thumbnails.enable = 1;
        thumbnails.buf = __this->thumbnail_img_buf;
        thumbnails.len = __this->thumbnail_img_size;
        req.rec.thumbnails = &thumbnails;
    }
#endif



    /*
     *采样率，通道数，录像音量，音频使用的循环BUF,录不录声音
     */
#ifdef CONFIG_NET_ENABLE
    req.rec.audio.sample_rate = video_rec_get_audio_sampel_rate();
#else
    req.rec.audio.sample_rate = 8000;
#endif
    req.rec.audio.channel 	= 1;
    req.rec.audio.volume    = AUDIO_VOLUME;
    req.rec.pkg_mute.aud_mute = !db_select("mic");

    req.rec.tlp_time = db_select("gap");
    if (req.rec.tlp_time) {
        req.rec.real_fps = 1000 / req.rec.tlp_time;
        req.rec.pkg_fps = video_rec_get_fps();
    }

    err = server_request(__this->video_rec1, VIDEO_REQ_REC, &req);
    if (err != 0) {
        log_e("rec0_save_file: err=%d\n", err);
        return err;
    }

    return 0;
}

static void video1_rec_close()
{
    if (__this->video_rec1) {
        server_close(__this->video_rec1);
        __this->video_rec1 = NULL;
    }
}


/*
 *必须在启动录像之后才可调用该函数，并且确保启动录像时已经打开了osd
 *新设置的osd的整体结构要和启动录像时一样，只是内容改变!!!
 */
static int video1_rec_set_osd_str(char *str)
{
    union video_req req;
    int err;
    if (!__this->video_rec1) {
        return -1;
    }

    req.rec.channel = 0;
    req.rec.state 	= VIDEO_STATE_SET_OSD_STR;
    req.rec.new_osd_str = str;
    err = server_request(__this->video_rec1, VIDEO_REQ_REC, &req);
    if (err != 0) {
        printf("\nset osd rec1 str err 0x%x\n", err);
        return -1;
    }

    return 0;
}

static int video1_rec_osd_ctl(u8 onoff)
{
    union video_req req;
    struct video_text_osd text_osd;
    int err;

    if (__this->video_rec1) {
        u32 res = db_select("res");
        req.rec.width 	    = rec_pix_w[res];
        req.rec.height 	    = rec_pix_h[res];

        text_osd.font_w = 16;
        text_osd.font_h = 32;
        text_osd.x = 0;//(req.rec.width - strlen(osd_str_buf) * text_osd.font_w) / 64 * 64;
        text_osd.y = (req.rec.height - text_osd.font_h) / 16 * 16;
        /* text_osd.osd_yuv = 0xe20095; */
        text_osd.color[0] = 0xe20095;
        /* text_osd.color[0] = 0x057d88; */
        /* text_osd.color[1] = 0xe20095; */
        /* text_osd.color[2] = 0xe20095; */
        text_osd.bit_mode = 1;


        text_osd.text_format = osd_str_buf;
        text_osd.font_matrix_table = osd_str_total;
        text_osd.font_matrix_base = osd_str_matrix;
        text_osd.font_matrix_len = sizeof(osd_str_matrix);
#ifdef __CPU_AC521x__
        text_osd.direction = 1;
#else
        text_osd.direction = 0;
#endif
        req.rec.text_osd = 0;
        if (onoff) {
            req.rec.text_osd = &text_osd;
        }
        req.rec.channel = 0;
        req.rec.state 	= VIDEO_STATE_SET_OSD;

        err = server_request(__this->video_rec1, VIDEO_REQ_REC, &req);
        if (err != 0) {
            printf("\nset osd rec1 err 0x%x\n", err);
            return -1;
        }
    }

    return 0;
}
#endif






#ifdef CONFIG_VIDEO2_ENABLE

static int video2_rec_start()
{
    int err;
    union video_req req = {0};
    struct video_text_osd text_osd;
    struct video_graph_osd graph_osd;
    u16 max_one_line_strnum;
    u16 osd_line_num;
    u16 osd_max_heigh;
    char name[12];

    void *uvc_fd;
    struct uvc_capability uvc_cap;

    puts("start_video_rec2 \n");
    if (!__this->video_rec2) {
        sprintf(name, "video1%d.%d", __this->uvc_id, __this->uvc_id);
        __this->video_rec2 = server_open("video_server", name);
        if (!__this->video_rec2) {
            return -EINVAL;
        }

        server_register_event_handler(__this->video_rec2, (void *)2, rec_dev_server_event_handler);
    }

    req.rec.online  = 0;//编码不联动
    req.rec.enable_dri  = 0;


    req.rec.channel = 0;
    req.rec.camera_type = VIDEO_CAMERA_UVC;
#if THREE_WAY_ENABLE
    req.rec.three_way_type = VIDEO_THREE_WAY_JPEG;
    req.rec.IP_interval = 99;
#else
    req.rec.three_way_type = 0;
    req.rec.IP_interval = 0;
#endif
    if (db_select("res") == VIDEO_RES_1080P) {
        req.rec.three_way_type = VIDEO_THREE_WAY_JPEG;
    }
    req.rec.format 	= VIDEO2_REC_FORMAT;
    req.rec.width 	= UVC_ENC_WIDTH;
    req.rec.height 	= UVC_ENC_HEIGH;
    req.rec.src_w   = __this->src_width[2];
    req.rec.src_h   = __this->src_height[2];
    __this->uvc_width = req.rec.width;
    __this->uvc_height = req.rec.height;
    printf("\n\nuvc size %d, %d\n\n", req.rec.width, req.rec.height);
    req.rec.state 	= VIDEO_STATE_START;
    req.rec.file    = __this->file[2];
#ifdef CONFIG_NET_ENABLE
    req.rec.fsize = __this->new_file_size[2];
#endif
    req.rec.uvc_id = __this->uvc_id;
    req.rec.quality = VIDEO_LOW_Q;

#ifdef CONFIG_NET_ENABLE
    req.rec.fps 	    = 0;
    req.rec.real_fps 	= video_rec_get_fps();
    req.rec.audio.sample_rate = video_rec_get_audio_sampel_rate();
#else
    req.rec.fps 		= 0;
    req.rec.real_fps 	= 0;
    req.rec.audio.sample_rate = video_rec_get_audio_sampel_rate();
#endif
    req.rec.audio.channel 	= 1;
    req.rec.audio.volume    = AUDIO_VOLUME;
    req.rec.audio.buf = __this->audio_buf[2];
    req.rec.audio.buf_len = AUDIO2_BUF_SIZE;
    req.rec.pkg_mute.aud_mute = !db_select("mic");

    req.rec.abr_kbps = video_rec_get_abr(req.rec.width);


    /*感兴趣区域为下方 中间 2/6 * 4/6 区域，可以调整
    	感兴趣区域qp 为其他区域的 70% ，可以调整
    */
    /* req.rec.roi.roio_xy = (req.rec.height * 5 / 6 / 16) << 24 | (req.rec.height * 3 / 6 / 16) << 16 | (req.rec.width * 5 / 6 / 16) << 8 | (req.rec.width) * 1 / 6 / 16; */
    /* req.rec.roi.roio_ratio = 256 * 70 / 100 ; */
    /* req.rec.roi.roi1_xy = 0; */
    /* req.rec.roi.roi2_xy = 0; */
    /* req.rec.roi.roi3_xy = (1 << 24) | (0 << 16) | ((req.rec.width / 16) << 8) | 0; */
    /* req.rec.roi.roio_ratio1 = 0; */
    /* req.rec.roi.roio_ratio2 = 0; */
    /* req.rec.roi.roio_ratio3 = 256 * 80 / 100; */

    /*
     * osd 相关的参数，注意坐标位置，x要64对齐，y要16对齐,底下例子是根据图像大小偏移到右下
     */
#if THREE_WAY_ENABLE
    text_osd.font_w = 16;
    text_osd.font_h = 32;

    max_one_line_strnum = strlen(video_rec_osd_buf);//20;
    osd_line_num = 2;
    text_osd.x = (req.rec.width - max_one_line_strnum * text_osd.font_w) / 64 * 64;
    text_osd.y = (req.rec.height - text_osd.font_h * osd_line_num) / 16 * 16;

    text_osd.direction = 1;
    text_osd.color[0] = 0xe20095;
    text_osd.bit_mode = 1;
    text_osd.text_format = video_rec_osd_buf;
    text_osd.font_matrix_table = osd_str_total;
    text_osd.font_matrix_base = osd_mimc_str_matrix;
    text_osd.font_matrix_len = sizeof(osd_mimc_str_matrix);

#else
    text_osd.font_w = 16;
    text_osd.font_h = 32;
    max_one_line_strnum = strlen(video_rec_osd_buf);//21;

    osd_line_num = 1;
    if (db_select("num")) {
        osd_line_num = 2;
    }
    osd_max_heigh = (req.rec.height == 1088) ? 1080 : req.rec.height ;
    text_osd.x = (req.rec.width - max_one_line_strnum * text_osd.font_w) / 64 * 64;
    text_osd.y = (osd_max_heigh - text_osd.font_h * osd_line_num) / 16 * 16;
    text_osd.color[0] = 0x057d88;
    text_osd.color[1] = 0xe20095;
    text_osd.color[2] = 0xe20095;
    text_osd.bit_mode = 2;
    text_osd.text_format = video_rec_osd_buf;
    text_osd.font_matrix_table = osd_str_total;
    text_osd.font_matrix_base = osd2_str_matrix;
    text_osd.font_matrix_len = sizeof(osd2_str_matrix);
    text_osd.direction = 1;
#endif



#ifdef CONFIG_OSD_LOGO
    graph_osd.bit_mode = 16;//2bit的osd需要配置3个color
    graph_osd.x = 0;
    graph_osd.y = 0;
    graph_osd.width = 256;
    graph_osd.height = 256;
    graph_osd.icon = icon_16bit_data;
    graph_osd.icon_size = sizeof(icon_16bit_data);
#endif
    req.rec.text_osd = NULL;
    req.rec.graph_osd = NULL;
    if (db_select("dat")) {
        req.rec.text_osd = &text_osd;
#ifdef CONFIG_OSD_LOGO
        req.rec.graph_osd = &graph_osd;
#endif
    }

    req.rec.slow_motion = 0;
#if THREE_WAY_ENABLE
    req.rec.tlp_time = 0;
#else
    req.rec.tlp_time = db_select("gap");
    if (req.rec.tlp_time) {
        req.rec.real_fps = 1000 / req.rec.tlp_time;
        req.rec.pkg_fps = video_rec_get_fps();
    }
#endif
    if (req.rec.slow_motion || req.rec.tlp_time) {
        req.rec.audio.sample_rate = 0;
        req.rec.audio.channel 	= 0;
        req.rec.audio.volume    = 0;
        req.rec.audio.buf = 0;
        req.rec.audio.buf_len = 0;
    }
#ifdef CONFIG_PSRAM_ENABLE
    req.rec.extbuf_dev.enable = 1;
    req.rec.extbuf_dev.name = "spiram";
    req.rec.extbuf_dev.size = __PSRAM_SIZE__ / 2;
    req.rec.extbuf_dev.addr = __PSRAM_SIZE__ / 2;
#endif
    req.rec.buf = __this->video_buf[2];
    req.rec.buf_len = VREC2_FBUF_SIZE;
#ifdef CONFIG_FILE_PREVIEW_ENABLE
    req.rec.rec_small_pic 	= 1;
#else
    req.rec.rec_small_pic 	= 0;
#endif


    req.rec.cycle_time = db_select("cyc");
    if (req.rec.cycle_time == 0) {
        req.rec.cycle_time = 5;
    }
    req.rec.cycle_time = req.rec.cycle_time * 60;

#if CAMERA_THUMBNAIL_ENABLE
    struct jpg_thumbnail thumbnails;
    err = camera_take_thumbnail(2);
    if (!err) {
        log_i("add thumbnail\n");
        thumbnails.enable = 1;
        thumbnails.buf = __this->thumbnail_img_buf;
        thumbnails.len = __this->thumbnail_img_size;
        req.rec.thumbnails = &thumbnails;
    }
#endif

    err = server_request(__this->video_rec2, VIDEO_REQ_REC, &req);
    if (err != 0) {
        puts("\n\n\nstart rec2 err\n\n\n");
        return -EINVAL;
    }

    return 0;
}

static int video2_rec_aud_mute()
{
    union video_req req;

    if (!__this->video_rec2) {
        return -EINVAL;
    }

    req.rec.channel = 0;
    req.rec.state 	= VIDEO_STATE_PKG_MUTE;
    req.rec.pkg_mute.aud_mute = !db_select("mic");

    return server_request(__this->video_rec2, VIDEO_REQ_REC, &req);
}


static int video2_rec_stop(u8 close)
{
    union video_req req;
    int err;

    log_d("video2_rec_stop\n");

    if (__this->video_rec2) {
        req.rec.channel = 0;
        req.rec.state = VIDEO_STATE_STOP;
        err = server_request(__this->video_rec2, VIDEO_REQ_REC, &req);
        if (err != 0) {
            printf("\nstop rec2 err 0x%x\n", err);
            return -EINVAL;
        }
    }

    video_rec_close_file(2);

    if (close) {
        if (__this->video_rec2) {
            server_close(__this->video_rec2);
            __this->video_rec2 = NULL;
        }
    }

    if (__this->thumb_buf) {
        free(__this->thumb_buf);
        __this->thumb_buf = NULL;
    }

    return 0;
}

static int video2_rec_savefile()
{
    union video_req req = {0};
    int err;

    if (__this->video_rec2) {

        if (!__this->file[2]) {
            return -ENOENT;
        }

        req.rec.channel = 0;
#if THREE_WAY_ENABLE
        req.rec.width 	= __this->uvc_width;
        req.rec.height 	= __this->uvc_height;
#else
        req.rec.width 	= UVC_ENC_WIDTH;
        req.rec.height 	= UVC_ENC_HEIGH;
#endif
        req.rec.format 	= VIDEO2_REC_FORMAT;
        req.rec.state 	= VIDEO_STATE_SAVE_FILE;
        req.rec.file    = __this->file[2];

#ifdef CONFIG_NET_ENABLE
        req.rec.fps 	    = 0;
        req.rec.real_fps 	= video_rec_get_fps();
#else
        req.rec.fps 	    = 0;
        req.rec.real_fps 	= 0;
#endif

#if THREE_WAY_ENABLE
        req.rec.tlp_time = 0;
#else
        req.rec.tlp_time = db_select("gap");
        if (req.rec.tlp_time) {
            req.rec.real_fps = 1000 / req.rec.tlp_time;
            req.rec.pkg_fps = video_rec_get_fps();
        }
#endif

        req.rec.cycle_time = db_select("cyc");
        if (req.rec.cycle_time == 0) {
            req.rec.cycle_time = 5;
        }

        req.rec.cycle_time = req.rec.cycle_time * 60;



#if CAMERA_THUMBNAIL_ENABLE
        struct jpg_thumbnail thumbnails;
        err = camera_take_thumbnail(2);
        if (!err) {
            log_i("add thumbnail\n");
            thumbnails.enable = 1;
            thumbnails.buf = __this->thumbnail_img_buf;
            thumbnails.len = __this->thumbnail_img_size;
            req.rec.thumbnails = &thumbnails;
        }
#endif



#ifdef CONFIG_NET_ENABLE
        req.rec.audio.sample_rate = video_rec_get_audio_sampel_rate();
#else
        req.rec.audio.sample_rate = 8000;
#endif
        req.rec.audio.channel 	= 1;
        req.rec.pkg_mute.aud_mute = !db_select("mic");

        err = server_request(__this->video_rec2, VIDEO_REQ_REC, &req);
        if (err != 0) {
            log_e("rec2_save_file: err=%d\n", err);
            return -EINVAL;
        }
    }

    return 0;
}

static void video2_rec_close()
{
    if (__this->video_rec2) {
        server_close(__this->video_rec2);
        __this->video_rec2 = NULL;
    }
}


/*
 *必须在启动录像之后才可调用该函数，并且确保启动录像时已经打开了osd
 *新设置的osd的整体结构要和启动录像时一样，只是内容改变!!!
 */
static int video2_rec_set_osd_str(char *str)
{
    union video_req req;
    int err;
    if (!__this->video_rec2) {
        return -1;
    }

    req.rec.channel = 0;
    req.rec.state 	= VIDEO_STATE_SET_OSD_STR;
    req.rec.new_osd_str = str;
    err = server_request(__this->video_rec2, VIDEO_REQ_REC, &req);
    if (err != 0) {
        printf("\nset osd rec2 str err 0x%x\n", err);
        return -1;
    }

    return 0;
}

static int video2_rec_osd_ctl(u8 onoff)
{
    union video_req req;
    struct video_text_osd text_osd;
    int err;

    if (__this->video_rec2) {
        req.rec.width 	= UVC_ENC_WIDTH;
        req.rec.height 	= UVC_ENC_HEIGH;

        text_osd.font_w = 16;
        text_osd.font_h = 32;
        text_osd.x = 0;//(req.rec.width - strlen(osd_str_buf) * text_osd.font_w) / 64 * 64;
        text_osd.y = (req.rec.height - text_osd.font_h) / 16 * 16;
        /* text_osd.osd_yuv = 0xe20095; */
        /* text_osd.osd_yuv = 0xe20095; */
        text_osd.color[0] = 0xe20095;
        /* text_osd.color[0] = 0x057d88; */
        /* text_osd.color[1] = 0xe20095; */
        /* text_osd.color[2] = 0xe20095; */
        text_osd.bit_mode = 1;


        text_osd.text_format = osd_str_buf;
        text_osd.font_matrix_table = osd_str_total;
        text_osd.font_matrix_base = osd_str_matrix;
        text_osd.font_matrix_len = sizeof(osd_str_matrix);
        text_osd.direction = 1;
        req.rec.text_osd = 0;
        if (onoff) {
            req.rec.text_osd = &text_osd;
        }
        req.rec.channel = 0;
        req.rec.state 	= VIDEO_STATE_SET_OSD;

        err = server_request(__this->video_rec2, VIDEO_REQ_REC, &req);
        if (err != 0) {
            printf("\nset osd rec2 err 0x%x\n", err);
            return -1;
        }
    }

    return 0;
}

#endif


/******* 不要单独调用这些子函数 ********/
#ifdef CONFIG_VIDEO3_ENABLE
static int video3_rec_start()
{
    int err;
    union video_req req = {0};
    struct video_text_osd text_osd;
    struct video_graph_osd graph_osd;
    u16 max_one_line_strnum;
    u16 osd_line_num;
    u16 osd_max_heigh;

    puts("start_video_rec3 \n");
    if (!__this->video_rec3) {
        __this->video_rec3 = server_open("video_server", "video3.0");
        if (!__this->video_rec3) {
            return VREC_ERR_V3_SERVER_OPEN;
        }

        server_register_event_handler(__this->video_rec3, (void *)3, rec_dev_server_event_handler);
    }

    u32 res = db_select("res");

    req.rec.online  = 1;
    req.rec.channel = 0;
    req.rec.enable_dri  = 0;
    req.rec.camera_type = VIDEO_CAMERA_NORMAL;
    req.rec.width 	    = rec_pix_w[res];
    req.rec.height 	    = rec_pix_h[res];
    req.rec.format 	= VIDEO3_REC_FORMAT;
    req.rec.state 	= VIDEO_STATE_START;
    req.rec.file    = __this->file[3];
    req.rec.quality = VIDEO_LOW_Q;
#ifdef CONFIG_NET_ENABLE
    req.rec.fps 	    = 0;
    req.rec.real_fps 	= video_rec_get_fps();
#else
    req.rec.fps 	    = 0;
    req.rec.real_fps 	= 0;
#endif




    req.rec.audio.sample_rate = video_rec_get_audio_sampel_rate();
    req.rec.audio.channel 	= 1;
    req.rec.audio.volume    = 100;
    req.rec.audio.buf = __this->audio_buf[3];
    req.rec.audio.buf_len = AUDIO3_BUF_SIZE;
    req.rec.pkg_mute.aud_mute = !db_select("mic");

    req.rec.abr_kbps = video_rec_get_abr(req.rec.width);
    req.rec.IP_interval = 0;

    /*感兴趣区域为下方 中间 2/6 * 4/6 区域，可以调整
    	感兴趣区域qp 为其他区域的 70% ，可以调整
    */
    /* req.rec.roi.roio_xy = (req.rec.height * 5 / 6 / 16) << 24 | (req.rec.height * 3 / 6 / 16) << 16 | (req.rec.width * 5 / 6 / 16) << 8 | (req.rec.width) * 1 / 6 / 16; */
    /* req.rec.roi.roio_ratio = 256 * 70 / 100 ; */
    /* req.rec.roi.roi1_xy = 0; */
    /* req.rec.roi.roi2_xy = 0; */
    /* req.rec.roi.roi3_xy = (1 << 24) | (0 << 16) | ((req.rec.width / 16) << 8) | 0; */
    /* req.rec.roi.roio_ratio1 = 0; */
    /* req.rec.roi.roio_ratio2 = 0; */
    /* req.rec.roi.roio_ratio3 = 256 * 80 / 100; */

    /*
     * osd 相关的参数，注意坐标位置，x要64对齐，y要16对齐,底下例子是根据图像大小偏移到右下
     */
    text_osd.font_w = 16;
    text_osd.font_h = 32;
    max_one_line_strnum = strlen(video_rec_osd_buf);//21;

    osd_line_num = 1;
    if (db_select("num")) {
        osd_line_num = 2;
    }

    osd_max_heigh = (req.rec.height == 1088) ? 1080 : req.rec.height ;
    text_osd.x = (req.rec.width - max_one_line_strnum * text_osd.font_w) / 64 * 64;
    text_osd.y = (osd_max_heigh - text_osd.font_h * osd_line_num) / 16 * 16;
    text_osd.color[0] = 0x057d88;
    text_osd.color[1] = 0xe20095;
    text_osd.color[2] = 0xe20095;
    text_osd.bit_mode = 2;
    text_osd.text_format = video_rec_osd_buf;
    text_osd.font_matrix_table = osd_str_total;
    text_osd.font_matrix_base = osd2_str_matrix;
    text_osd.font_matrix_len = sizeof(osd2_str_matrix);
    text_osd.direction = 1;

#ifdef CONFIG_OSD_LOGO
    graph_osd.bit_mode = 16;//2bit的osd需要配置3个color
    graph_osd.x = 0;
    graph_osd.y = 0;
    graph_osd.width = 256;
    graph_osd.height = 256;
    graph_osd.icon = icon_16bit_data;
    graph_osd.icon_size = sizeof(icon_16bit_data);
#endif
    req.rec.text_osd = NULL;
    req.rec.graph_osd = NULL;
    if (db_select("dat")) {
        req.rec.text_osd = &text_osd;
#ifdef CONFIG_OSD_LOGO
        req.rec.graph_osd = &graph_osd;
#endif
    }


    req.rec.slow_motion = 0;
    if (req.rec.camera_type != VIDEO_CAMERA_UVC) {
        req.rec.tlp_time = db_select("gap");
        if (req.rec.tlp_time) {
            req.rec.real_fps = 1000 / req.rec.tlp_time;
            req.rec.pkg_fps = video_rec_get_fps();
        }
    } else {
        req.rec.tlp_time = 0;
    }

    if (req.rec.slow_motion || req.rec.tlp_time) {
        req.rec.audio.sample_rate = 0;
        req.rec.audio.channel 	= 0;
        req.rec.audio.volume    = 0;
        req.rec.audio.buf = 0;
        req.rec.audio.buf_len = 0;
    }
    req.rec.buf = __this->video_buf[3];
    req.rec.buf_len = VREC3_FBUF_SIZE;

#ifdef CONFIG_FILE_PREVIEW_ENABLE
    req.rec.rec_small_pic 	= 1;
#else
    req.rec.rec_small_pic 	= 0;
#endif

#if (DOUBLE_720 == 1)
    req.rec.double720 = true;
#endif

#if (DOUBLE_720_SMALL_SCR == 1)
    req.rec.double720_small_scr = true;
#endif


    req.rec.cycle_time = db_select("cyc");
    if (req.rec.cycle_time == 0) {
        req.rec.cycle_time = 5;
    }

    req.rec.cycle_time = req.rec.cycle_time * 60;

#if CAMERA_THUMBNAIL_ENABLE
    struct jpg_thumbnail thumbnails;
    err = camera_take_thumbnail(3);
    if (!err) {
        log_i("add thumbnail\n");
        thumbnails.enable = 1;
        thumbnails.buf = __this->thumbnail_img_buf;
        thumbnails.len = __this->thumbnail_img_size;
        req.rec.thumbnails = &thumbnails;
    }
#endif


    err = server_request(__this->video_rec3, VIDEO_REQ_REC, &req);
    if (err != 0) {
        puts("\n\n\nstart rec3 err\n\n\n");
        return VREC_ERR_V3_REQ_START;
    }

    return 0;
}

static int video3_rec_aud_mute()
{
    union video_req req;

    if (!__this->video_rec3) {
        return -EINVAL;
    }

    req.rec.channel = 0;
    req.rec.state 	= VIDEO_STATE_PKG_MUTE;
    req.rec.pkg_mute.aud_mute = !db_select("mic");

    return server_request(__this->video_rec3, VIDEO_REQ_REC, &req);
}

static int video3_rec_set_dr()
{
    union video_req req = {0};

    if (!__this->video_rec3) {
        return -EINVAL;
    }

    req.rec.real_fps = 7;
    req.rec.channel = 0;
    req.rec.state 	= VIDEO_STATE_SET_DR;

    return server_request(__this->video_rec3, VIDEO_REQ_REC, &req);

}


static int video3_rec_stop(u8 close)
{
    union video_req req;
    int err;

    log_d("video3_rec_stop\n");

    if (__this->video_rec3) {
        req.rec.channel = 0;
        req.rec.state = VIDEO_STATE_STOP;
        err = server_request(__this->video_rec3, VIDEO_REQ_REC, &req);
        if (err != 0) {
            printf("\nstop rec3 err 0x%x\n", err);
            return VREC_ERR_V1_REQ_STOP;
        }
    }

    video_rec_close_file(3);

    if (close) {
        if (__this->video_rec3) {
            server_close(__this->video_rec3);
            __this->video_rec3 = NULL;
        }
    }

    if (__this->thumb_buf) {
        free(__this->thumb_buf);
        __this->thumb_buf = NULL;
    }

    return 0;
}

static int video3_rec_savefile()
{
    union video_req req = {0};
    int err;

    if (!__this->file[3]) {
        return -ENOENT;
    }

    u32 res = db_select("res");

    req.rec.channel = 0;
    req.rec.width 	= rec_pix_w[res];
    req.rec.height 	= rec_pix_h[res];
    req.rec.format 	= VIDEO0_REC_FORMAT;
    req.rec.state 	= VIDEO_STATE_SAVE_FILE;
    req.rec.file    = __this->file[3];

#ifdef CONFIG_NET_ENABLE
    req.rec.fps 	    = 0;
    req.rec.real_fps 	= video_rec_get_fps();
#else
    req.rec.fps 	    = 0;
    req.rec.real_fps 	= 0;
#endif

    req.rec.cycle_time = db_select("cyc");
    if (req.rec.cycle_time == 0) {
        req.rec.cycle_time = 5;
    }

    req.rec.cycle_time = req.rec.cycle_time * 60;

#if CAMERA_THUMBNAIL_ENABLE
    struct jpg_thumbnail thumbnails;
    err = camera_take_thumbnail(3);
    if (!err) {
        log_i("add thumbnail\n");
        thumbnails.enable = 1;
        thumbnails.buf = __this->thumbnail_img_buf;
        thumbnails.len = __this->thumbnail_img_size;
        req.rec.thumbnails = &thumbnails;
    }
#endif


    /*
     *采样率，通道数，录像音量，音频使用的循环BUF,录不录声音
     */
#ifdef CONFIG_NET_ENABLE
    req.rec.audio.sample_rate = video_rec_get_audio_sampel_rate();
#else
    req.rec.audio.sample_rate = 8000;
#endif
    req.rec.audio.channel 	= 1;
    req.rec.audio.volume    = AUDIO_VOLUME;
    req.rec.pkg_mute.aud_mute = !db_select("mic");

    req.rec.tlp_time = db_select("gap");
    if (req.rec.tlp_time) {
        req.rec.real_fps = 1000 / req.rec.tlp_time;
        req.rec.pkg_fps = video_rec_get_fps();
    }

    err = server_request(__this->video_rec3, VIDEO_REQ_REC, &req);
    if (err != 0) {
        log_e("rec0_save_file: err=%d\n", err);
        return err;
    }
    return 0;
}

static void video3_rec_close()
{
    if (__this->video_rec3) {
        server_close(__this->video_rec3);
        __this->video_rec3 = NULL;
    }
}


/*
 *必须在启动录像之后才可调用该函数，并且确保启动录像时已经打开了osd
 *新设置的osd的整体结构要和启动录像时一样，只是内容改变!!!
 */
static int video3_rec_set_osd_str(char *str)
{
    union video_req req;
    int err;
    if (!__this->video_rec3) {
        return -1;
    }

    req.rec.channel = 0;
    req.rec.state 	= VIDEO_STATE_SET_OSD_STR;
    req.rec.new_osd_str = str;
    err = server_request(__this->video_rec3, VIDEO_REQ_REC, &req);
    if (err != 0) {
        printf("\nset osd rec3 str err 0x%x\n", err);
        return -1;
    }

    return 0;
}

static int video3_rec_osd_ctl(u8 onoff)
{
    union video_req req;
    struct video_text_osd text_osd;
    int err;

    if (__this->video_rec3) {
        u32 res = db_select("res");
        req.rec.width 	    = rec_pix_w[res];
        req.rec.height 	    = rec_pix_h[res];

        text_osd.font_w = 16;
        text_osd.font_h = 32;
        text_osd.x = 0;//(req.rec.width - strlen(osd_str_buf) * text_osd.font_w) / 64 * 64;
        text_osd.y = (req.rec.height - text_osd.font_h) / 16 * 16;
        /* text_osd.osd_yuv = 0xe20095; */
        text_osd.color[0] = 0xe20095;
        /* text_osd.color[0] = 0x057d88; */
        /* text_osd.color[1] = 0xe20095; */
        /* text_osd.color[2] = 0xe20095; */
        text_osd.bit_mode = 1;


        text_osd.text_format = osd_str_buf;
        text_osd.font_matrix_table = osd_str_total;
        text_osd.font_matrix_base = osd_str_matrix;
        text_osd.font_matrix_len = sizeof(osd_str_matrix);
#ifdef __CPU_AC521x__
        text_osd.direction = 1;
#else
        text_osd.direction = 0;
#endif
        req.rec.text_osd = 0;
        if (onoff) {
            req.rec.text_osd = &text_osd;
        }
        req.rec.channel = 0;
        req.rec.state 	= VIDEO_STATE_SET_OSD;

        err = server_request(__this->video_rec3, VIDEO_REQ_REC, &req);
        if (err != 0) {
            printf("\nset osd rec3 err 0x%x\n", err);
            return -1;
        }
    }

    return 0;
}
#endif

#ifdef CONFIG_VIDEO4_ENABLE
static int video4_rec_start()
{
    int err;
    union video_req req = {0};
    struct video_text_osd text_osd;
    struct video_graph_osd graph_osd;
    u16 max_one_line_strnum;
    u16 osd_line_num;
    u16 osd_max_heigh;

    puts("start_video_rec4 \n");
    if (!__this->video_rec4) {
        __this->video_rec4 = server_open("video_server", "video4.0");
        if (!__this->video_rec4) {
            return VREC_ERR_V4_SERVER_OPEN;
        }

        server_register_event_handler(__this->video_rec4, (void *)4, rec_dev_server_event_handler);
    }

    u32 res = db_select("res");

    req.rec.online  = 1;
    req.rec.channel = 0;
    req.rec.enable_dri  = 0;
    req.rec.camera_type = VIDEO_CAMERA_NORMAL;
    req.rec.width 	    = rec_pix_w[res];
    req.rec.height 	    = rec_pix_h[res];
    req.rec.format 	= VIDEO4_REC_FORMAT;
    req.rec.state 	= VIDEO_STATE_START;
    req.rec.file    = __this->file[4];
    req.rec.quality = VIDEO_LOW_Q;
#ifdef CONFIG_NET_ENABLE
    req.rec.fps 	    = 0;
    req.rec.real_fps 	= video_rec_get_fps();
#else
    req.rec.fps 	    = 0;
    req.rec.real_fps 	= 0;
#endif




    req.rec.audio.sample_rate = video_rec_get_audio_sampel_rate();
    req.rec.audio.channel 	= 1;
    req.rec.audio.volume    = 100;
    req.rec.audio.buf = __this->audio_buf[4];
    req.rec.audio.buf_len = AUDIO4_BUF_SIZE;
    req.rec.pkg_mute.aud_mute = !db_select("mic");

    req.rec.abr_kbps = video_rec_get_abr(req.rec.width);
    req.rec.IP_interval = 0;

    /*感兴趣区域为下方 中间 2/6 * 4/6 区域，可以调整
    	感兴趣区域qp 为其他区域的 70% ，可以调整
    */
    /* req.rec.roi.roio_xy = (req.rec.height * 5 / 6 / 16) << 24 | (req.rec.height * 3 / 6 / 16) << 16 | (req.rec.width * 5 / 6 / 16) << 8 | (req.rec.width) * 1 / 6 / 16; */
    /* req.rec.roi.roio_ratio = 256 * 70 / 100 ; */
    /* req.rec.roi.roi1_xy = 0; */
    /* req.rec.roi.roi2_xy = 0; */
    /* req.rec.roi.roi3_xy = (1 << 24) | (0 << 16) | ((req.rec.width / 16) << 8) | 0; */
    /* req.rec.roi.roio_ratio1 = 0; */
    /* req.rec.roi.roio_ratio2 = 0; */
    /* req.rec.roi.roio_ratio3 = 256 * 80 / 100; */

    /*
     * osd 相关的参数，注意坐标位置，x要64对齐，y要16对齐,底下例子是根据图像大小偏移到右下
     */
    text_osd.font_w = 16;
    text_osd.font_h = 32;
    max_one_line_strnum = strlen(video_rec_osd_buf);//21;

    osd_line_num = 1;
    if (db_select("num")) {
        osd_line_num = 2;
    }

    osd_max_heigh = (req.rec.height == 1088) ? 1080 : req.rec.height ;
    text_osd.x = (req.rec.width - max_one_line_strnum * text_osd.font_w) / 64 * 64;
    text_osd.y = (osd_max_heigh - text_osd.font_h * osd_line_num) / 16 * 16;
    text_osd.color[0] = 0x057d88;
    text_osd.color[1] = 0xe20095;
    text_osd.color[2] = 0xe20095;
    text_osd.bit_mode = 2;
    text_osd.text_format = video_rec_osd_buf;
    text_osd.font_matrix_table = osd_str_total;
    text_osd.font_matrix_base = osd2_str_matrix;
    text_osd.font_matrix_len = sizeof(osd2_str_matrix);
    text_osd.direction = 1;

#ifdef CONFIG_OSD_LOGO
    graph_osd.bit_mode = 16;//2bit的osd需要配置3个color
    graph_osd.x = 0;
    graph_osd.y = 0;
    graph_osd.width = 256;
    graph_osd.height = 256;
    graph_osd.icon = icon_16bit_data;
    graph_osd.icon_size = sizeof(icon_16bit_data);
#endif
    req.rec.text_osd = NULL;
    req.rec.graph_osd = NULL;
    if (db_select("dat")) {
        req.rec.text_osd = &text_osd;
#ifdef CONFIG_OSD_LOGO
        req.rec.graph_osd = &graph_osd;
#endif
    }


    req.rec.slow_motion = 0;
    if (req.rec.camera_type != VIDEO_CAMERA_UVC) {
        req.rec.tlp_time = db_select("gap");
        if (req.rec.tlp_time) {
            req.rec.real_fps = 1000 / req.rec.tlp_time;
            req.rec.pkg_fps = video_rec_get_fps();
        }
    } else {
        req.rec.tlp_time = 0;
    }

    if (req.rec.slow_motion || req.rec.tlp_time) {
        req.rec.audio.sample_rate = 0;
        req.rec.audio.channel 	= 0;
        req.rec.audio.volume    = 0;
        req.rec.audio.buf = 0;
        req.rec.audio.buf_len = 0;
    }
    req.rec.buf = __this->video_buf[4];
    req.rec.buf_len = VREC4_FBUF_SIZE;

#ifdef CONFIG_FILE_PREVIEW_ENABLE
    req.rec.rec_small_pic 	= 1;
#else
    req.rec.rec_small_pic 	= 0;
#endif

#if (DOUBLE_720 == 1)
    req.rec.double720 = true;
#endif

#if (DOUBLE_720_SMALL_SCR == 1)
    req.rec.double720_small_scr = true;
#endif


    req.rec.cycle_time = db_select("cyc");
    if (req.rec.cycle_time == 0) {
        req.rec.cycle_time = 5;
    }

    req.rec.cycle_time = req.rec.cycle_time * 60;

#if CAMERA_THUMBNAIL_ENABLE
    struct jpg_thumbnail thumbnails;
    err = camera_take_thumbnail(4);
    if (!err) {
        log_i("add thumbnail\n");
        thumbnails.enable = 1;
        thumbnails.buf = __this->thumbnail_img_buf;
        thumbnails.len = __this->thumbnail_img_size;
        req.rec.thumbnails = &thumbnails;
    }
#endif



    err = server_request(__this->video_rec4, VIDEO_REQ_REC, &req);
    if (err != 0) {
        puts("\n\n\nstart rec4 err\n\n\n");
        return VREC_ERR_V4_REQ_START;
    }

    return 0;
}

static int video4_rec_aud_mute()
{
    union video_req req;

    if (!__this->video_rec4) {
        return -EINVAL;
    }

    req.rec.channel = 0;
    req.rec.state 	= VIDEO_STATE_PKG_MUTE;
    req.rec.pkg_mute.aud_mute = !db_select("mic");

    return server_request(__this->video_rec4, VIDEO_REQ_REC, &req);
}

static int video4_rec_set_dr()
{
    union video_req req = {0};

    if (!__this->video_rec4) {
        return -EINVAL;
    }

    req.rec.real_fps = 7;
    req.rec.channel = 0;
    req.rec.state 	= VIDEO_STATE_SET_DR;

    return server_request(__this->video_rec4, VIDEO_REQ_REC, &req);

}


static int video4_rec_stop(u8 close)
{
    union video_req req;
    int err;

    log_d("video4_rec_stop\n");

    if (__this->video_rec4) {
        req.rec.channel = 0;
        req.rec.state = VIDEO_STATE_STOP;
        err = server_request(__this->video_rec4, VIDEO_REQ_REC, &req);
        if (err != 0) {
            printf("\nstop rec4 err 0x%x\n", err);
            return VREC_ERR_V1_REQ_STOP;
        }
    }

    video_rec_close_file(4);

    if (close) {
        if (__this->video_rec4) {
            server_close(__this->video_rec4);
            __this->video_rec4 = NULL;
        }
    }

    if (__this->thumb_buf) {
        free(__this->thumb_buf);
        __this->thumb_buf = NULL;
    }


    return 0;
}

static int video4_rec_savefile()
{
    union video_req req = {0};
    int err;

    if (!__this->file[4]) {
        return -ENOENT;
    }

    u32 res = db_select("res");

    req.rec.channel = 0;
    req.rec.width 	= rec_pix_w[res];
    req.rec.height 	= rec_pix_h[res];
    req.rec.format 	= VIDEO0_REC_FORMAT;
    req.rec.state 	= VIDEO_STATE_SAVE_FILE;
    req.rec.file    = __this->file[4];

#ifdef CONFIG_NET_ENABLE
    req.rec.fps 	    = 0;
    req.rec.real_fps 	= video_rec_get_fps();
#else
    req.rec.fps 	    = 0;
    req.rec.real_fps 	= 0;
#endif

    req.rec.cycle_time = db_select("cyc");
    if (req.rec.cycle_time == 0) {
        req.rec.cycle_time = 5;
    }

    req.rec.cycle_time = req.rec.cycle_time * 60;

#if CAMERA_THUMBNAIL_ENABLE
    struct jpg_thumbnail thumbnails;
    err = camera_take_thumbnail(4);
    if (!err) {
        log_i("add thumbnail\n");
        thumbnails.enable = 1;
        thumbnails.buf = __this->thumbnail_img_buf;
        thumbnails.len = __this->thumbnail_img_size;
        req.rec.thumbnails = &thumbnails;
    }
#endif


    /*
     *采样率，通道数，录像音量，音频使用的循环BUF,录不录声音
     */
#ifdef CONFIG_NET_ENABLE
    req.rec.audio.sample_rate = video_rec_get_audio_sampel_rate();
#else
    req.rec.audio.sample_rate = 8000;
#endif
    req.rec.audio.channel 	= 1;
    req.rec.audio.volume    = AUDIO_VOLUME;
    req.rec.pkg_mute.aud_mute = !db_select("mic");

    req.rec.tlp_time = db_select("gap");
    if (req.rec.tlp_time) {
        req.rec.real_fps = 1000 / req.rec.tlp_time;
        req.rec.pkg_fps = video_rec_get_fps();
    }

    err = server_request(__this->video_rec4, VIDEO_REQ_REC, &req);
    if (err != 0) {
        log_e("rec0_save_file: err=%d\n", err);
        return err;
    }

    return 0;
}

static void video4_rec_close()
{
    if (__this->video_rec4) {
        server_close(__this->video_rec4);
        __this->video_rec4 = NULL;
    }
}


/*
 *必须在启动录像之后才可调用该函数，并且确保启动录像时已经打开了osd
 *新设置的osd的整体结构要和启动录像时一样，只是内容改变!!!
 */
static int video4_rec_set_osd_str(char *str)
{
    union video_req req;
    int err;
    if (!__this->video_rec4) {
        return -1;
    }

    req.rec.channel = 0;
    req.rec.state 	= VIDEO_STATE_SET_OSD_STR;
    req.rec.new_osd_str = str;
    err = server_request(__this->video_rec4, VIDEO_REQ_REC, &req);
    if (err != 0) {
        printf("\nset osd rec4 str err 0x%x\n", err);
        return -1;
    }

    return 0;
}

static int video4_rec_osd_ctl(u8 onoff)
{
    union video_req req;
    struct video_text_osd text_osd;
    int err;

    if (__this->video_rec4) {
        u32 res = db_select("res");
        req.rec.width 	    = rec_pix_w[res];
        req.rec.height 	    = rec_pix_h[res];

        text_osd.font_w = 16;
        text_osd.font_h = 32;
        text_osd.x = 0;//(req.rec.width - strlen(osd_str_buf) * text_osd.font_w) / 64 * 64;
        text_osd.y = (req.rec.height - text_osd.font_h) / 16 * 16;
        /* text_osd.osd_yuv = 0xe20095; */
        text_osd.color[0] = 0xe20095;
        /* text_osd.color[0] = 0x057d88; */
        /* text_osd.color[1] = 0xe20095; */
        /* text_osd.color[2] = 0xe20095; */
        text_osd.bit_mode = 1;


        text_osd.text_format = osd_str_buf;
        text_osd.font_matrix_table = osd_str_total;
        text_osd.font_matrix_base = osd_str_matrix;
        text_osd.font_matrix_len = sizeof(osd_str_matrix);
#ifdef __CPU_AC521x__
        text_osd.direction = 1;
#else
        text_osd.direction = 0;
#endif
        req.rec.text_osd = 0;
        if (onoff) {
            req.rec.text_osd = &text_osd;
        }
        req.rec.channel = 0;
        req.rec.state 	= VIDEO_STATE_SET_OSD;

        err = server_request(__this->video_rec4, VIDEO_REQ_REC, &req);
        if (err != 0) {
            printf("\nset osd rec4 err 0x%x\n", err);
            return -1;
        }
    }

    return 0;
}
#endif


#ifdef CONFIG_VIDEO5_ENABLE
static int video5_rec_start()
{
    int err;
    union video_req req = {0};
    struct video_text_osd text_osd;
    struct video_graph_osd graph_osd;
    u16 max_one_line_strnum;
    u16 osd_line_num;
    u16 osd_max_heigh;

    puts("start_video_rec5 \n");
    if (!__this->video_rec5) {
        __this->video_rec5 = server_open("video_server", "video5.0");
        if (!__this->video_rec5) {
            return VREC_ERR_V5_SERVER_OPEN;
        }

        server_register_event_handler(__this->video_rec5, (void *)5, rec_dev_server_event_handler);
    }

    u32 res = db_select("res");

    req.rec.online  = 1;
    req.rec.channel = 0;
    req.rec.enable_dri  = 0;
    req.rec.camera_type = VIDEO_CAMERA_NORMAL;
    req.rec.width 	    = rec_pix_w[res];
    req.rec.height 	    = rec_pix_h[res];
    req.rec.format 	= VIDEO5_REC_FORMAT;
    req.rec.state 	= VIDEO_STATE_START;
    req.rec.file    = __this->file[5];
    req.rec.quality = VIDEO_LOW_Q;
#ifdef CONFIG_NET_ENABLE
    req.rec.fps 	    = 0;
    req.rec.real_fps 	= video_rec_get_fps();
#else
    req.rec.fps 	    = 0;
    req.rec.real_fps 	= 0;
#endif




    req.rec.audio.sample_rate = video_rec_get_audio_sampel_rate();
    req.rec.audio.channel 	= 1;
    req.rec.audio.volume    = 100;
    req.rec.audio.buf = __this->audio_buf[5];
    req.rec.audio.buf_len = AUDIO5_BUF_SIZE;
    req.rec.pkg_mute.aud_mute = !db_select("mic");

    req.rec.abr_kbps = video_rec_get_abr(req.rec.width);
    req.rec.IP_interval = 0;

    /*感兴趣区域为下方 中间 2/6 * 4/6 区域，可以调整
    	感兴趣区域qp 为其他区域的 70% ，可以调整
    */
    /* req.rec.roi.roio_xy = (req.rec.height * 5 / 6 / 16) << 24 | (req.rec.height * 3 / 6 / 16) << 16 | (req.rec.width * 5 / 6 / 16) << 8 | (req.rec.width) * 1 / 6 / 16; */
    /* req.rec.roi.roio_ratio = 256 * 70 / 100 ; */
    /* req.rec.roi.roi1_xy = 0; */
    /* req.rec.roi.roi2_xy = 0; */
    /* req.rec.roi.roi3_xy = (1 << 24) | (0 << 16) | ((req.rec.width / 16) << 8) | 0; */
    /* req.rec.roi.roio_ratio1 = 0; */
    /* req.rec.roi.roio_ratio2 = 0; */
    /* req.rec.roi.roio_ratio3 = 256 * 80 / 100; */

    /*
     * osd 相关的参数，注意坐标位置，x要64对齐，y要16对齐,底下例子是根据图像大小偏移到右下
     */
    text_osd.font_w = 16;
    text_osd.font_h = 32;
    max_one_line_strnum = strlen(video_rec_osd_buf);//21;

    osd_line_num = 1;
    if (db_select("num")) {
        osd_line_num = 2;
    }

    osd_max_heigh = (req.rec.height == 1088) ? 1080 : req.rec.height ;
    text_osd.x = (req.rec.width - max_one_line_strnum * text_osd.font_w) / 64 * 64;
    text_osd.y = (osd_max_heigh - text_osd.font_h * osd_line_num) / 16 * 16;
    text_osd.color[0] = 0x057d88;
    text_osd.color[1] = 0xe20095;
    text_osd.color[2] = 0xe20095;
    text_osd.bit_mode = 2;
    text_osd.text_format = video_rec_osd_buf;
    text_osd.font_matrix_table = osd_str_total;
    text_osd.font_matrix_base = osd2_str_matrix;
    text_osd.font_matrix_len = sizeof(osd2_str_matrix);
    text_osd.direction = 1;

#ifdef CONFIG_OSD_LOGO
    graph_osd.bit_mode = 16;//2bit的osd需要配置3个color
    graph_osd.x = 0;
    graph_osd.y = 0;
    graph_osd.width = 256;
    graph_osd.height = 256;
    graph_osd.icon = icon_16bit_data;
    graph_osd.icon_size = sizeof(icon_16bit_data);
#endif
    req.rec.text_osd = NULL;
    req.rec.graph_osd = NULL;
    if (db_select("dat")) {
        req.rec.text_osd = &text_osd;
#ifdef CONFIG_OSD_LOGO
        req.rec.graph_osd = &graph_osd;
#endif
    }


    req.rec.slow_motion = 0;
    if (req.rec.camera_type != VIDEO_CAMERA_UVC) {
        req.rec.tlp_time = db_select("gap");
        if (req.rec.tlp_time) {
            req.rec.real_fps = 1000 / req.rec.tlp_time;
            req.rec.pkg_fps = video_rec_get_fps();
        }
    } else {
        req.rec.tlp_time = 0;
    }

    if (req.rec.slow_motion || req.rec.tlp_time) {
        req.rec.audio.sample_rate = 0;
        req.rec.audio.channel 	= 0;
        req.rec.audio.volume    = 0;
        req.rec.audio.buf = 0;
        req.rec.audio.buf_len = 0;
    }
    req.rec.buf = __this->video_buf[5];
    req.rec.buf_len = VREC5_FBUF_SIZE;

#ifdef CONFIG_FILE_PREVIEW_ENABLE
    req.rec.rec_small_pic 	= 1;
#else
    req.rec.rec_small_pic 	= 0;
#endif

#if (DOUBLE_720 == 1)
    req.rec.double720 = true;
#endif

#if (DOUBLE_720_SMALL_SCR == 1)
    req.rec.double720_small_scr = true;
#endif


    req.rec.cycle_time = db_select("cyc");
    if (req.rec.cycle_time == 0) {
        req.rec.cycle_time = 5;
    }

    req.rec.cycle_time = req.rec.cycle_time * 60;

#if CAMERA_THUMBNAIL_ENABLE
    struct jpg_thumbnail thumbnails;
    err = camera_take_thumbnail(5);
    if (!err) {
        log_i("add thumbnail\n");
        thumbnails.enable = 1;
        thumbnails.buf = __this->thumbnail_img_buf;
        thumbnails.len = __this->thumbnail_img_size;
        req.rec.thumbnails = &thumbnails;
    }
#endif



    err = server_request(__this->video_rec5, VIDEO_REQ_REC, &req);
    if (err != 0) {
        puts("\n\n\nstart rec5 err\n\n\n");
        return VREC_ERR_V5_REQ_START;
    }

    return 0;
}

static int video5_rec_aud_mute()
{
    union video_req req;

    if (!__this->video_rec5) {
        return -EINVAL;
    }

    req.rec.channel = 0;
    req.rec.state 	= VIDEO_STATE_PKG_MUTE;
    req.rec.pkg_mute.aud_mute = !db_select("mic");

    return server_request(__this->video_rec5, VIDEO_REQ_REC, &req);
}

static int video5_rec_set_dr()
{
    union video_req req = {0};

    if (!__this->video_rec5) {
        return -EINVAL;
    }

    req.rec.real_fps = 7;
    req.rec.channel = 0;
    req.rec.state 	= VIDEO_STATE_SET_DR;

    return server_request(__this->video_rec5, VIDEO_REQ_REC, &req);

}


static int video5_rec_stop(u8 close)
{
    union video_req req;
    int err;

    log_d("video5_rec_stop\n");

    if (__this->video_rec5) {
        req.rec.channel = 0;
        req.rec.state = VIDEO_STATE_STOP;
        err = server_request(__this->video_rec5, VIDEO_REQ_REC, &req);
        if (err != 0) {
            printf("\nstop rec5 err 0x%x\n", err);
            return VREC_ERR_V1_REQ_STOP;
        }
    }

    video_rec_close_file(5);

    if (close) {
        if (__this->video_rec5) {
            server_close(__this->video_rec5);
            __this->video_rec5 = NULL;
        }
    }

    if (__this->thumb_buf) {
        free(__this->thumb_buf);
        __this->thumb_buf = NULL;
    }


    return 0;
}

static int video5_rec_savefile()
{
    union video_req req = {0};
    int err;

    if (!__this->file[5]) {
        return -ENOENT;
    }

    u32 res = db_select("res");

    req.rec.channel = 0;
    req.rec.width 	= rec_pix_w[res];
    req.rec.height 	= rec_pix_h[res];
    req.rec.format 	= VIDEO0_REC_FORMAT;
    req.rec.state 	= VIDEO_STATE_SAVE_FILE;
    req.rec.file    = __this->file[5];

#ifdef CONFIG_NET_ENABLE
    req.rec.fps 	    = 0;
    req.rec.real_fps 	= video_rec_get_fps();
#else
    req.rec.fps 	    = 0;
    req.rec.real_fps 	= 0;
#endif

    req.rec.cycle_time = db_select("cyc");
    if (req.rec.cycle_time == 0) {
        req.rec.cycle_time = 5;
    }

    req.rec.cycle_time = req.rec.cycle_time * 60;

#if CAMERA_THUMBNAIL_ENABLE
    struct jpg_thumbnail thumbnails;
    err = camera_take_thumbnail(5);
    if (!err) {
        log_i("add thumbnail\n");
        thumbnails.enable = 1;
        thumbnails.buf = __this->thumbnail_img_buf;
        thumbnails.len = __this->thumbnail_img_size;
        req.rec.thumbnails = &thumbnails;
    }
#endif


    /*
     *采样率，通道数，录像音量，音频使用的循环BUF,录不录声音
     */
#ifdef CONFIG_NET_ENABLE
    req.rec.audio.sample_rate = video_rec_get_audio_sampel_rate();
#else
    req.rec.audio.sample_rate = 8000;
#endif
    req.rec.audio.channel 	= 1;
    req.rec.audio.volume    = AUDIO_VOLUME;
    req.rec.pkg_mute.aud_mute = !db_select("mic");

    req.rec.tlp_time = db_select("gap");
    if (req.rec.tlp_time) {
        req.rec.real_fps = 1000 / req.rec.tlp_time;
        req.rec.pkg_fps = video_rec_get_fps();
    }

    err = server_request(__this->video_rec5, VIDEO_REQ_REC, &req);
    if (err != 0) {
        log_e("rec0_save_file: err=%d\n", err);
        return err;
    }

    return 0;
}

static void video5_rec_close()
{
    if (__this->video_rec5) {
        server_close(__this->video_rec5);
        __this->video_rec5 = NULL;
    }
}


/*
 *必须在启动录像之后才可调用该函数，并且确保启动录像时已经打开了osd
 *新设置的osd的整体结构要和启动录像时一样，只是内容改变!!!
 */
static int video5_rec_set_osd_str(char *str)
{
    union video_req req;
    int err;
    if (!__this->video_rec5) {
        return -1;
    }

    req.rec.channel = 0;
    req.rec.state 	= VIDEO_STATE_SET_OSD_STR;
    req.rec.new_osd_str = str;
    err = server_request(__this->video_rec5, VIDEO_REQ_REC, &req);
    if (err != 0) {
        printf("\nset osd rec5 str err 0x%x\n", err);
        return -1;
    }

    return 0;
}

static int video5_rec_osd_ctl(u8 onoff)
{
    union video_req req;
    struct video_text_osd text_osd;
    int err;

    if (__this->video_rec5) {
        u32 res = db_select("res");
        req.rec.width 	    = rec_pix_w[res];
        req.rec.height 	    = rec_pix_h[res];

        text_osd.font_w = 16;
        text_osd.font_h = 32;
        text_osd.x = 0;//(req.rec.width - strlen(osd_str_buf) * text_osd.font_w) / 64 * 64;
        text_osd.y = (req.rec.height - text_osd.font_h) / 16 * 16;
        /* text_osd.osd_yuv = 0xe20095; */
        text_osd.color[0] = 0xe20095;
        /* text_osd.color[0] = 0x057d88; */
        /* text_osd.color[1] = 0xe20095; */
        /* text_osd.color[2] = 0xe20095; */
        text_osd.bit_mode = 1;


        text_osd.text_format = osd_str_buf;
        text_osd.font_matrix_table = osd_str_total;
        text_osd.font_matrix_base = osd_str_matrix;
        text_osd.font_matrix_len = sizeof(osd_str_matrix);
#ifdef __CPU_AC521x__
        text_osd.direction = 1;
#else
        text_osd.direction = 0;
#endif
        req.rec.text_osd = 0;
        if (onoff) {
            req.rec.text_osd = &text_osd;
        }
        req.rec.channel = 0;
        req.rec.state 	= VIDEO_STATE_SET_OSD;

        err = server_request(__this->video_rec5, VIDEO_REQ_REC, &req);
        if (err != 0) {
            printf("\nset osd rec5 err 0x%x\n", err);
            return -1;
        }
    }

    return 0;
}
#endif



static int video_rec_buf_alloc(void)
{
    int abuf_size[] = {AUDIO0_BUF_SIZE, AUDIO1_BUF_SIZE, AUDIO2_BUF_SIZE, AUDIO3_BUF_SIZE};
    for (int i = 0; i < ARRAY_SIZE(abuf_size); i++) {
        if (abuf_size[i]) {
            if (!__this->audio_buf[i]) {
                __this->audio_buf[i] = malloc(abuf_size[i]);
                if (__this->audio_buf[i] == NULL) {
                    log_i("err maloo\n");
                    while (1);
                }
            }
        } else {
            __this->audio_buf[i] = NULL;
        }
    }


    int buf_size[] = {VREC0_FBUF_SIZE, VREC1_FBUF_SIZE, VREC2_FBUF_SIZE, VREC3_FBUF_SIZE};
    for (int i = 0; i < ARRAY_SIZE(buf_size); i++) {
        if (buf_size[i]) {
            if (!__this->video_buf[i]) {
                __this->video_buf[i] = malloc(buf_size[i]);
                if (!__this->video_buf[i]) {
                    log_d(">>>>>>>>>> videobuf alloc err>>>>>>\n");
                    return -ENOMEM;
                }
            }
        } else {
            __this->video_buf[i] = NULL;
        }
    }
    return 0;
}
static int video_rec_start()
{
    int err;
    u32 clust;
    u8 state = __this->state;
    int buf_size[] = {VREC0_FBUF_SIZE, VREC1_FBUF_SIZE, VREC2_FBUF_SIZE, VREC3_FBUF_SIZE};
    int abuf_size[] = {AUDIO0_BUF_SIZE, AUDIO1_BUF_SIZE, AUDIO2_BUF_SIZE, AUDIO3_BUF_SIZE};

    err = video_rec_buf_alloc();
    if (err) {
        return err;
    }

    __this->char_wait = 0;
    __this->need_restart_rec = 0;

    if (__this->state == VIDREC_STA_START) {
        return 0;
    }

    log_d("(((((( video_rec_start: in\n");

    if (!storage_device_available()) {
        return 0;
    }

    /*
     * 申请录像所需要的音频和视频帧buf
     */

    for (int i = 0; i < ARRAY_SIZE(abuf_size); i++) {
        if (abuf_size[i]) {
            if (!__this->audio_buf[i]) {
                __this->audio_buf[i] = malloc(abuf_size[i]);
                if (__this->audio_buf[i] == NULL) {
                    log_i("err maloo\n");
                    while (1);
                }
            }
        } else {
            __this->audio_buf[i] = NULL;
        }
    }




    for (int i = 0; i < ARRAY_SIZE(buf_size); i++) {
        if (buf_size[i]) {
            if (!__this->video_buf[i]) {
                __this->video_buf[i] = malloc(buf_size[i]);
                if (!__this->video_buf[i]) {
                    log_d(">>>>>>>>>> videobuf alloc err>>>>>>\n");
                }
            }
        } else {
            __this->video_buf[i] = NULL;
        }
    }

    /*
     * 判断SD卡空间，删除旧文件并创建新文件
     */
    err = video_rec_del_old_file();
    if (err) {
        log_e("start free space err\n");
        video_rec_post_msg("fsErr");
        return VREC_ERR_START_FREE_SPACE;
    }

    for (int i = 0; i < CONFIG_VIDEO_REC_NUM; i++) {
        __this->file[i] = __this->new_file[i];
        __this->new_file[i] = NULL;
    }


#ifdef CONFIG_VIDEO0_ENABLE
    err = video0_rec_start();
    if (err) {
        video_rec_post_msg("recErr");
        video0_rec_stop(0);
        return err;
    }
#endif


    video_rec_post_msg("onREC");
    video_parking_post_msg("onREC");

#ifdef CONFIG_VIDEO1_ENABLE
    if (__this->video_online[1] && db_select("two")) {
        err = video1_rec_start();
    }
#endif


#ifdef CONFIG_VIDEO2_ENABLE
    if (__this->video_online[2] && db_select("two")) {
        err = video2_rec_start();
    }
#endif

#ifdef CONFIG_VIDEO3_ENABLE
    if (__this->video_online[3] && db_select("two")) {
        err = video3_rec_start();
    }
#endif

#ifdef CONFIG_VIDEO4_ENABLE
    if (__this->video_online[4] && db_select("two")) {
        err = video4_rec_start();
    }
#endif

#ifdef CONFIG_VIDEO5_ENABLE
    if (__this->video_online[5] && db_select("two")) {
        err = video5_rec_start();
    }
#endif



    if (__this->gsen_lock == 0xff) {
        video_rec_post_msg("lockREC");
    } else {
        video_rec_post_msg("unlockREC");
    }

#ifndef CONFIG_DISPLAY_ENABLE
    //video_rec_set_white_balance();
    video_rec_set_exposure(db_select("exp"));
#endif
    sys_power_auto_shutdown_pause();
    __this->state = VIDREC_STA_START;

    log_d("video_rec_start: out )))))))\n");
    malloc_stats();

    return 0;
}


static int video_rec_aud_mute()
{

    if (db_select("mic")) {
        puts("mic on\n");
        video_rec_post_msg("onMIC");
    } else {
        puts("mic off\n");
        video_rec_post_msg("offMIC");
    }

    if (__this->state != VIDREC_STA_START) {
        return 0;
    }

#ifdef CONFIG_VIDEO0_ENABLE
    video0_rec_aud_mute();
#endif

#ifdef CONFIG_VIDEO1_ENABLE
    video1_rec_aud_mute();
#endif

#ifdef CONFIG_VIDEO2_ENABLE
    video2_rec_aud_mute();
#endif

#ifdef CONFIG_VIDEO3_ENABLE
    video3_rec_aud_mute();
#endif

#ifdef CONFIG_VIDEO4_ENABLE
    video4_rec_aud_mute();
#endif

#ifdef CONFIG_VIDEO4_ENABLE
    video4_rec_aud_mute();
#endif


    return 0;
}

u8 rec_park_flag = 0;
static int video_rec_stop(u8 close)
{
    int err;
    __this->need_restart_rec = 0;

    if (__this->state != VIDREC_STA_START) {
        return 0;
    }

    puts("\nvideo_rec_stop\n");

    __this->state = VIDREC_STA_STOPING;
    rec_park_flag = 0;

#ifdef CONFIG_VIDEO0_ENABLE
    err = video0_rec_stop(close);
    if (err) {
        puts("\nstop0 err\n");
    }
#endif

#ifdef CONFIG_VIDEO1_ENABLE
    err = video1_rec_stop(close);
    if (err) {
        puts("\nstop1 err\n");
    }
#endif

#ifdef CONFIG_VIDEO2_ENABLE
    err = video2_rec_stop(close);
    if (err) {
        puts("\nstop2 err\n");
    }
#endif

#ifdef CONFIG_VIDEO3_ENABLE
    err = video3_rec_stop(close);
    if (err) {
        puts("\nstop3 err\n");
    }
#endif

#ifdef CONFIG_VIDEO4_ENABLE
    err = video4_rec_stop(close);
    if (err) {
        puts("\nstop4 err\n");
    }
#endif

#ifdef CONFIG_VIDEO5_ENABLE
    err = video5_rec_stop(close);
    if (err) {
        puts("\nstop5 err\n");
    }
#endif

    if (__this->disp_state == DISP_BACK_WIN) {
        video_rec_post_msg("HlightOff"); //后视停录像关闭前照灯
    }

    __this->state = VIDREC_STA_STOP;
    __this->gsen_lock = 0;
    sys_power_auto_shutdown_resume();

    video_home_post_msg("offREC");
    video_parking_post_msg("offREC");
    video_rec_post_msg("offREC");

    video_rec_get_remain_time();


    puts("video_rec_stop: exit\n");
    return 0;
}




static int video_rec_close()
{
#ifdef CONFIG_VIDEO0_ENABLE
    video0_rec_close();
#endif

#ifdef CONFIG_VIDEO1_ENABLE
    video1_rec_close();
#endif

#ifdef CONFIG_VIDEO2_ENABLE
    video2_rec_close();
#endif

#ifdef CONFIG_VIDEO3_ENABLE
    video3_rec_close();
#endif

#ifdef CONFIG_VIDEO4_ENABLE
    video4_rec_close();
#endif

#ifdef CONFIG_VIDEO4_ENABLE
    video4_rec_close();
#endif



    return 0;
}

static int video_rec_change_source_reso(int dev_id, u16 width, u16 height)
{
#ifdef CONFIG_VIDEO2_ENABLE
    if (dev_id == 2) {
        __this->src_width[2] = width;
        __this->src_height[2] = height;
        if (__this->video_online[2]) {
            log_d("video12.* change source reso to %d x %d\n", width, height);
            int rec_state = __this->state;
            int disp_state = __this->disp_state;
            if (rec_state == VIDREC_STA_START) {
                video_rec_stop(0);
            }
            if (disp_state == DISP_MAIN_WIN ||
                disp_state == DISP_HALF_WIN ||
                disp_state == DISP_BACK_WIN ||
                disp_state == DISP_PARK_WIN) {
                if (__this->second_disp_dev == 2) {
                    video_disp_stop(2);
                }
            }
            //video2.* record and display must be closed before source reso change
            if (rec_state == VIDREC_STA_START) {
                video_rec_start();
            }
            if (disp_state == DISP_MAIN_WIN ||
                disp_state == DISP_HALF_WIN ||
                disp_state == DISP_BACK_WIN ||
                disp_state == DISP_PARK_WIN) {
                if (__this->second_disp_dev == 2) {
                    video_disp_start(2, &disp_window[disp_state][1]);
                }
            }
        }
    } else
#endif
    {
        //do not remove this brace
    }
    return 0;
}

static void key_and_touch_enable(void *priv)
{
    key_event_enable();
    /* touch_event_enable(); */
}



static int video_rec_savefile(int dev_id)
{
    int i;
    int err;
    int post_msg = 0;
    union video_req req;

    if (__this->state != VIDREC_STA_START) {
        return 0;
    }

    if (__this->need_restart_rec) {
        log_d("need restart rec");
        video_rec_stop(0);
        video_rec_start();
        return 0;
    }

    log_d("\nvideo_rec_start_new_file: %d\n", dev_id);

    video_rec_close_file(dev_id);

    if (__this->new_file[dev_id] == NULL) {
        err = video_rec_del_old_file();
        if (err) {
            video_rec_post_msg("fsErr");
            goto __err;
        }
        key_event_disable();
        /* touch_event_disable(); */
        sys_timeout_add(NULL, key_and_touch_enable, 2000);
        post_msg = 1;
    }
    __this->file[dev_id]     = __this->new_file[dev_id];
    __this->new_file[dev_id] = NULL;

#ifdef CONFIG_VIDEO0_ENABLE
    if (dev_id == 0) {
        err = video0_rec_savefile();
        if (err) {
            goto __err;
        }
    }
#endif
#ifdef CONFIG_VIDEO4_ENABLE
    if (__this->video_online[4] && (dev_id == 4)) {
        err = video4_rec_savefile();
        if (err) {
            goto __err;
        }
    }
#endif


    if (post_msg) {
        video_rec_post_msg("saveREC");
        video_home_post_msg("saveREC");//录像切到后台,ui消息由主界面响应
        video_parking_post_msg("saveREC");
    }

#ifdef CONFIG_VIDEO1_ENABLE
    if (__this->video_online[1] && (dev_id == 1)) {
        err = video1_rec_savefile();
        if (err) {
            goto __err;
        }
    }
#endif

    /* puts("\n\n------save2\n\n"); */
#ifdef CONFIG_VIDEO2_ENABLE
    if (__this->video_online[2] && (dev_id == 2)) {
        err = video2_rec_savefile();
        if (err) {
            goto __err;
        }
    }
#endif

#ifdef CONFIG_VIDEO3_ENABLE
    if (__this->video_online[3] && (dev_id == 3)) {
        err = video3_rec_savefile();
        if (err) {
            goto __err;
        }
    }
#endif

#ifdef CONFIG_VIDEO5_ENABLE
    if (__this->video_online[5] && (dev_id == 5)) {
        err = video5_rec_savefile();
        if (err) {
            goto __err;
        }
    }
#endif



    __this->state = VIDREC_STA_START;

#ifdef CONFIG_NET_ENABLE
    video_rec_state_notify();
#endif

    malloc_stats();


    return 0;


__err:

#ifdef CONFIG_VIDEO5_ENABLE
    err = video5_rec_stop(0);
    if (err) {
        printf("\nsave wrong5 %x\n", err);
    }
#endif

#ifdef CONFIG_VIDEO4_ENABLE
    err = video4_rec_stop(0);
    if (err) {
        printf("\nsave wrong4 %x\n", err);
    }
#endif

#ifdef CONFIG_VIDEO3_ENABLE
    err = video3_rec_stop(0);
    if (err) {
        printf("\nsave wrong3 %x\n", err);
    }
#endif

#ifdef CONFIG_VIDEO2_ENABLE
    err = video2_rec_stop(0);
    if (err) {
        printf("\nsave wrong2 %x\n", err);
    }
#endif

#ifdef CONFIG_VIDEO1_ENABLE
    err = video1_rec_stop(0);
    if (err) {
        printf("\nsave wrong1 %x\n", err);
    }
#endif

#ifdef CONFIG_VIDEO0_ENABLE
    err = video0_rec_stop(0);
    if (err) {
        printf("\nsave wrong0 %x\n", err);
    }
#endif


    video_rec_post_msg("offREC");
    video_home_post_msg("offREC");//录像切到后台,ui消息由主界面响应
    video_parking_post_msg("offREC");
    __this->state = VIDREC_STA_STOP;

    return -EFAULT;

}

//设置日期水印
static int set_label_config(u16 image_width, u16 image_height, u32 font_color,
                            struct video_text_osd *label)
{
    if (!label) {
        return 0;
    }

    label->direction = 0;

    label->font_w = 16;
    label->font_h = 32;
    label->text_format = label_format;
    label->font_matrix_table = (char *)osd_str_total;

    label->color[0] = 0x057d88;
    label->color[1] = 0xe20095;
    label->color[2] = 0xe20095;
    label->bit_mode = 2;
    label->font_matrix_base = osd2_str_matrix;
    label->font_matrix_len = sizeof(osd2_str_matrix);

    label->x = (1280 - strlen(label_format) * label->font_w) / 64 * 64;
    label->y = (720 - label->font_h - 16) / 16 * 16;

    return 0;
}

static int video_rec_capture(int id)
{
    struct server *server;
    union video_req req = {0};
    char video_name[10];
    int err;
    static struct video_text_osd label;
    int cap_buf_size = 400 * 1024;

    log_i(" video%d capture ", id);

    if (id == 2) {
        req.icap.camera_type = VIDEO_CAMERA_UVC;
        req.icap.uvc_id = uvc_host_online();
        sprintf(video_name, "video%d.0", 10);
    } else {
        sprintf(video_name, "video%d.0", id);
    }
    server = server_open("video_server", video_name);
    if (!server) {
        log_e("video_server open fail");
        return -EFAULT;
    }

    if (__this->cap_buf == NULL) {
        __this->cap_buf = malloc(cap_buf_size);

        if (!__this->cap_buf) {
            puts("\ntake photo no mem\n");
            return -ENOMEM;
        }
    }
    if (id == 0) {
        req.icap.width = pic_pix_w[0];
        req.icap.height = pic_pix_h[0];
    } else {
        req.icap.width = 640;
        req.icap.height = 480;
    }
    req.icap.src_w = req.icap.width;
    req.icap.src_h = req.icap.height;
#if THREE_WAY_DOUBLE_RAW
    req.icap.double_raw = 1;
#endif

    req.icap.quality = VIDEO_MID_Q;
    req.icap.text_label = &label;
    req.icap.image_state = 0;
    req.icap.online = 0;
    set_label_config(req.icap.width, req.icap.height, 0xe20095, req.icap.text_label);
    req.icap.buf = __this->cap_buf;
    req.icap.buf_size = 400 * 1024;
    if (id == 0) {
        req.icap.path = CAMERA0_CAP_PATH"img_****.jpg";
    } else if (id == 1) {
        req.icap.path = CAMERA1_CAP_PATH"img_****.jpg";
    } else if (id == 2) {
        req.icap.path = CAMERA1_CAP_PATH"img_****.jpg";
    } else {
        req.icap.path = CAMERA2_CAP_PATH"img_****.jpg";
    }

#if CAMERA_THUMBNAIL_ENABLE
    log_i("add thumbnail");
    struct jpg_thumbnail thumbnails = {0};
    thumbnails.enable = 1;
    thumbnails.buf = __this->cap_buf + cap_buf_size - IMAGE_THUMB_BUF_SIZE;
    thumbnails.len = IMAGE_THUMB_BUF_SIZE;
    req.icap.thumbnails = &thumbnails;
#endif

    err = server_request(server, VIDEO_REQ_IMAGE_CAPTURE, &req);
    if (err != 0) {
        log_e("\ntake photo err: %d\n", err);
        return -EINVAL;
    }
    if (__this->cap_buf) {
        free(__this->cap_buf);
        __this->cap_buf = NULL;
    }
    server_close(server);

    return 0;
}

/*
 * 录像时会直接抽取当前录像帧进行保存,如果没录像会进入拍照流程
 * 拍照流程会需要至少1.5M + 400K的空间，请根据实际情况来使用
 */
static int video_rec_take_photo(void)
{
    union video_req req = {0};
    int err = -1;

#if THREE_WAY_ENABLE
    //8M三路版本,内存不够不支持走拍照流程。
    if (__this->state != VIDREC_STA_START) {
        log_w("capture fail !!! please start record !!");
        return -1;
    }

    if (__this->video_rec0) {
        req.rec.rec_save_path = CAMERA0_CAP_PATH"img_****.jpg";
        err = server_request(__this->video_rec0, VIDEO_REQ_SAVE_FRAME, &req);
        log_i("video0 save frame");
    }
    if (__this->video_rec1) {
        req.rec.rec_save_path = CAMERA1_CAP_PATH"img_****.jpg";
        err = server_request(__this->video_rec1, VIDEO_REQ_SAVE_FRAME, &req);
        log_i("video1 save frame");
    }
    if (__this->video_rec2) {
        req.rec.rec_save_path = CAMERA2_CAP_PATH"img_****.jpg";
        err = server_request(__this->video_rec2, VIDEO_REQ_SAVE_FRAME, &req);
        log_i("video2 save frame");
    }
#else
    if (__this->state != VIDREC_STA_START) {
        //
        //拍照流程会需要至少1.5M + 400K的空间，请根据实际情况来使用
        //

        //释放内存,录像时申请的内存
        for (int i = 0; i < CONFIG_VIDEO_REC_NUM; i++) {
            if (__this->video_buf[i]) {
                free(__this->video_buf[i]);
                __this->video_buf[i] = NULL;
            }
        }
        malloc_stats();
#ifdef CONFIG_VIDEO0_ENABLE
        err = video_rec_capture(0);
#endif
#ifdef CONFIG_VIDEO2_ENABLE
        err = video_rec_capture(2);
#endif
#ifdef CONFIG_VIDEO4_ENABLE
        err = video_rec_capture(4);
#endif
#ifdef CONFIG_VIDEO5_ENABLE
        err = video_rec_capture(5);
#endif
    } else {
        if (__this->video_rec0) {
            req.rec.rec_save_path = CAMERA0_CAP_PATH"img_****.jpg";
            err = server_request(__this->video_rec0, VIDEO_REQ_SAVE_FRAME, &req);
            log_i("video0 save frame");
        }
        if (__this->video_rec2) {
            req.rec.rec_save_path = CAMERA1_CAP_PATH"img_****.jpg";
            err = server_request(__this->video_rec2, VIDEO_REQ_SAVE_FRAME, &req);
            log_i("video2 save frame");
        }
        if (__this->video_rec4) {
            req.rec.rec_save_path = CAMERA4_CAP_PATH"img_****.jpg";
            err = server_request(__this->video_rec4, VIDEO_REQ_SAVE_FRAME, &req);
            log_i("video4 save frame");
        }
        if (__this->video_rec5) {
            req.rec.rec_save_path = CAMERA5_CAP_PATH"img_****.jpg";
            err = server_request(__this->video_rec5, VIDEO_REQ_SAVE_FRAME, &req);
            log_i("video5 save frame");
        }
    }
#endif /* THREE_WAY_ENABLE */

    return err;
}


int video_rec_set_exposure(u32 exp)
{
    union video_req req;

    if (!__this->exposure_set) {
        return 0;
    }

    req.camera.mode = ISP_MODE_IMAGE_CAPTURE;
    req.camera.ev = exp;
    req.camera.cmd = SET_CAMERA_EV;

    if (__this->video_display[0]) {
        server_request(__this->video_display[0], VIDEO_REQ_CAMERA_EFFECT, &req);
    } else if (__this->video_rec0 && (__this->state == VIDREC_STA_START)) {
        server_request(__this->video_rec0, VIDEO_REQ_CAMERA_EFFECT, &req);
    } else {
        return 1;
    }

    __this->exposure_set = 0;

    return 0;
}

/*
使用限制 !!!
1.延时录像时不能使用
2.两路编码都是摄像头0时不能使用
3.使用智能引擎（除了只使用普通的移动侦测模式 VE_MOTION_DETECT_MODE_ISP 这种情况）时不能使用
*/
#if 1
/* int video_rec_digital_zoom(u8 big_small) */
/* { */
/* #if 0 //def CONFIG_UI_ENABLE */
/* union video_req req; */
/* struct video_crop_sca *crop_result; */

/* if ((!__this->video_display[0]) || db_select("gap")) { */
/* return 1; */
/* } */

/* req.sca.sca_modify = big_small; */
/* req.sca.step = 16;//4 align */
/* req.sca.max_sca = 5; */
/* crop_result = (struct video_crop_sca *)server_request(__this->video_display[0], VIDEO_REQ_CAMERA_SCA, &req); */
/* if (crop_result) { */
/* printf("src_w %d, src_h %d, crop_w %d, crop_h %d\n", */
/* crop_result->src_w, crop_result->src_h, crop_result->crop_w, crop_result->crop_h); */
/* } */
/* #endif */
/* return 0; */
/* } */
#endif




#ifdef PHOTO_STICKER_ENABLE_SMALL_MEM
extern u8 stk_name[64];
static int get_sticker()
{
    FILE *fp;
    int size = 0;
    /* __this->sticker_num = 2; */
    /******测试使用文件, 以实际使用为准*******/
    if (__this->sticker_num == 0) {
        return -1;
    }
    /* if (__this->sticker_num == __this->prev_sticker) { */
    /* return &__this->sticker; */
    /* } */
    __this->sticker_name  = (u8 *)stk_name;
    sprintf(__this->sticker_name, STICKER_PATH, __this->sticker_num);
    log_d("__this->sticker_name: %s\n", __this->sticker_name);
    __this->prev_sticker = __this->sticker_num;
    return 0;
}


static void switch_sticker()
{
    union video_req req = {0};
    if (!__this->video_display[0]) {
        return ;
    }

    if (__this->prev_sticker == 0) {
        //stop disp
        //start disp
        //stop rec
        //start rec
#ifdef CONFIG_VIDEO0_ENABLE
        get_sticker();
        video_disp_stop(0);
        u8 tmp = 0;
        if (__this->state == VIDREC_STA_START) {
            video_rec_stop(0);
            tmp = 1;
        }

        video_disp_start(0, &disp_window[DISP_MAIN_WIN][0]);
        if (tmp) {
            video_rec_start();
        }
#elif (defined CONFIG_VIDEO4_ENABLE)
        get_sticker();
        video_disp_stop(4);
        u8 tmp = 0;
        if (__this->state == VIDREC_STA_START) {
            video_rec_stop(4);
            tmp = 1;
        }

        video_disp_start(4, &disp_window[DISP_MAIN_WIN][0]);
        if (tmp) {
            video_rec_start();
        }
#endif
        return;
    } else if (__this->sticker_num == 0) {
        __this->prev_sticker = __this->sticker_num;
#ifdef CONFIG_VIDEO0_ENABLE
        video_disp_stop(0);
        u8 tmp = 0;
        if (__this->state == VIDREC_STA_START) {
            video_rec_stop(0);
            tmp = 1;
        }
        __this->sticker_name  = NULL;
        video_disp_start(0, &disp_window[DISP_MAIN_WIN][0]);

        if (tmp) {
            video_rec_start();
        }

#elif (defined CONFIG_VIDEO4_ENABLE)
        video_disp_stop(4);
        u8 tmp = 0;
        if (__this->state == VIDREC_STA_START) {
            video_rec_stop(4);
            tmp = 1;
        }
        __this->sticker_name  = NULL;
        video_disp_start(4, &disp_window[DISP_MAIN_WIN][0]);

        if (tmp) {
            video_rec_start();
        }

#endif
        return;
    }

    if (!get_sticker()) {
        if (__this->sticker_name) {
            req.display.sticker_name      = __this->sticker_name;
        }

        server_request(__this->video_display[0], VIDEO_REQ_STICKER_SWICTH, &req);
    }


}
#endif

static int video_rec_storage_device_ready(void *p)
{
    __this->sd_wait = 0;

    video_rec_scan_lock_file();

    if ((int)p == 1) {
#ifdef CONFIG_NET_ENABLE
        video_rec_start_notify();//先停止网络实时流再录像,录像完毕再通知APP
#else
        video_rec_start();
#endif
    }

    return 0;
}


static int video_rec_sd_in()
{
#ifdef CONFIG_NET_ENABLE
    video_rec_sd_event_ctp_notify(1);
#endif

    if (__this->state == VIDREC_STA_IDLE) {
        video_rec_get_remain_time();
        ve_mdet_stop();
        ve_lane_det_stop(0);
    }

    if (__this->menu_inout == 0) {
        ve_mdet_start();
        ve_lane_det_start(0);
    }

#ifdef CONFIG_NET_ENABLE
    net_video_rec_status_notify();
#endif

    __this->lock_fsize_count = 0;

    return 0;
}

static int video_rec_sd_out()
{
    ve_mdet_stop();
    ve_lane_det_stop(0);

    video_rec_fscan_release(0);
    video_rec_fscan_release(1);

    if (__this->sd_wait == 0) {
        __this->sd_wait = wait_completion(storage_device_ready,
                                          video_rec_storage_device_ready, (void *)1, NULL);
    }
#ifdef CONFIG_NET_ENABLE
    video_rec_sd_event_ctp_notify(0);
#endif
    return 0;
}

static void video_rec_park_call_back(void *priv)
{
    if (__this->state == VIDREC_STA_START) {
        puts("video_rec_park\n");
        video_rec_stop(0);
        gpio_direction_input(TCFG_USB_POWER_CHECK_IO);
        gpio_set_die(TCFG_USB_POWER_CHECK_IO, 1);
        if (gpio_read(TCFG_USB_POWER_CHECK_IO) && (__this->state == VIDREC_STA_STOP)) {
            video_rec_start();
        } else {
            puts("park rec off power close\n");
            sys_power_shutdown();
        }
    }
}

static int video_rec_park_wait(void *priv)
{
    int err = 0;

    puts("video_rec_park_wait\n");

    if (__this->state != VIDREC_STA_START) {
        puts("park_rec_start\n");
        err = video_rec_start();
    }

    if (err == 0) {
        sys_timeout_add(NULL, video_rec_park_call_back, 30 * 1000);
        if (__this->park_wait_timeout) {
            sys_timeout_del(__this->park_wait_timeout);
            __this->park_wait_timeout = 0;
        }
    }

    return 0;
}

static void video_rec_park_wait_timeout(void *priv)
{
    if (__this->state == VIDREC_STA_START) {
        return;
    }
    puts("park wait timeout power close\n");
    sys_power_shutdown();
}

static void check_usb_gpio_state(void)
{
    static u8 prev_gpio_state;
    u8 gpio_state;
    gpio_direction_input(TCFG_USB_POWER_CHECK_IO);
    gpio_set_die(TCFG_USB_POWER_CHECK_IO, 1);
    gpio_state = gpio_read(TCFG_USB_POWER_CHECK_IO);
    if (gpio_state) {
        if (gpio_state != prev_gpio_state) {
            video_rec_post_msg("batIcon", 110);
        }
    } else {
        int vbat_level = get_vbat_percent();
        video_rec_post_msg("batIcon", vbat_level);
    }
    prev_gpio_state = gpio_state;
}

static int video_rec_init()
{
    int err = 0;

    if (!__this->check_usb_power_timer) {
        __this->check_usb_power_timer = sys_timer_add_to_task("app_core", NULL, check_usb_gpio_state, 1000);
    }

    if (!__this->get_isp_lv_timer) {
        __this->get_isp_lv_timer = sys_timer_add_to_task("app_core", NULL, get_isp_lv_timer_cb, 1500);
    }

#if (CONFIG_VIDEO_PARK_DECT == 1)
    __this->disp_park_sel = 1;
#elif (CONFIG_VIDEO_PARK_DECT == 3)
    __this->disp_park_sel = 2;
#elif (CONFIG_VIDEO_PARK_DECT == 4)
    __this->disp_park_sel = 3;
#else
    __this->disp_park_sel = 0;
#endif


#ifdef PHOTO_STICKER_ENABLE_SMALL_MEM
    __this->sticker_num = db_select("stk");
    __this->prev_sticker = __this->sticker_num;
    get_sticker();
#endif
#ifdef CONFIG_VIDEO0_ENABLE
    __this->video_online[0] = 1;
    /* err = video_disp_start(0, &disp_window[DISP_MAIN_WIN][0]); */
#endif

#ifdef CONFIG_VIDEO1_ENABLE
    __this->video_online[1] = 1;
#endif


#ifdef CONFIG_VIDEO2_ENABLE
    __this->video_online[2] = dev_online("uvc");
    if (__this->video_online[2]) {
        __this->uvc_id = uvc_host_online();
    }
#endif

#ifdef CONFIG_VIDEO3_ENABLE
    __this->video_online[3] = 1;
#endif

#ifdef CONFIG_VIDEO4_ENABLE
    __this->video_online[4] = 1;
#endif

#ifdef CONFIG_VIDEO5_ENABLE
    __this->video_online[5] = 1;
#endif

    __this->disp_state = DISP_MAIN_WIN;
    __this->second_disp_dev = 0;


#ifdef CONFIG_PARK_ENABLE
    if (get_parking_status()) {
        //UI TODO
    } else {
        //UI TODO
    }
#endif
    video_rec_get_remain_time();


#ifdef CONFIG_GSENSOR_ENABLE
    if (sys_power_get_wakeup_reason()) {
        if (db_select("par")) {
            __this->gsen_lock = 0xff;
            __this->park_wait_timeout = sys_timeout_add(NULL, video_rec_park_wait_timeout, 10 * 1000);
            __this->park_wait = wait_completion(storage_device_ready,
                                                video_rec_park_wait, NULL, NULL);
        }
    } else {
        __this->sd_wait = wait_completion(storage_device_ready,
                                          video_rec_storage_device_ready, 0, 0);
    }
#else
    __this->sd_wait = wait_completion(storage_device_ready,
                                      video_rec_storage_device_ready, 0, 0);
#endif

    if (get_parking_status()) {
        video_disp_win_switch(DISP_WIN_SW_SHOW_PARKING, 0);
    } else {
        video_disp_win_switch(DISP_WIN_SW_SHOW_SMALL, 0);
    }


    ve_server_open(0);

#if (defined CONFIG_VIDEO0_ENABLE && defined CONFIG_VIDEO1_ENABLE)
    if (__this->video_online[0] && __this->video_online[1])
#elif (defined CONFIG_VIDEO4_ENABLE && defined CONFIG_VIDEO5_ENABLE)
    if (__this->video_online[4] && __this->video_online[5])
#endif
    {
        video_rec_post_msg("swWinicon", 1);
    }

    return err;
}




static int video_rec_uninit()
{
    int err;
    union video_req req;

    if (__this->check_usb_power_timer) {
        sys_timer_del(__this->check_usb_power_timer);
        __this->check_usb_power_timer = NULL;
    }
    if (__this->get_isp_lv_timer) {
        sys_timer_del(__this->get_isp_lv_timer);
        __this->get_isp_lv_timer = NULL;
    }

    if (__this->state == VIDREC_STA_START) {
        return -EFAULT;
    }
    if (__this->park_wait) {
        wait_completion_del(__this->park_wait);
        __this->park_wait = 0;
    }
    if (__this->sd_wait) {
        wait_completion_del(__this->sd_wait);
        __this->sd_wait = 0;
    }
    if (__this->char_wait) {
        wait_completion_del(__this->char_wait);
        __this->char_wait = 0;
    }
    if (__this->park_wait_timeout) {
        sys_timeout_del(__this->park_wait_timeout);
        __this->park_wait_timeout = 0;
    }


    ve_server_close();

    if (__this->state == VIDREC_STA_START) {
        err = video_rec_stop(1);
    }
    video_rec_close();


    video_rec_fscan_release(0);
    video_rec_fscan_release(1);

    for (int i = 0; i < CONFIG_VIDEO_REC_NUM; i++) {
        video_disp_stop(i);
    }

    __this->disp_state = DISP_FORBIDDEN;
    __this->state = VIDREC_STA_FORBIDDEN;
    __this->lan_det_setting = 0;

    return 0;

}


static int video_rec_mode_sw()
{
    if (__this->state != VIDREC_STA_FORBIDDEN) {
        return -EFAULT;
    }

    for (int i = 0; i < CONFIG_VIDEO_REC_NUM; i++) {
        if (__this->video_buf[i]) {
            free(__this->video_buf[i]);
            __this->video_buf[i] = NULL;
        }
    }

    for (int i = 0; i < CONFIG_VIDEO_REC_NUM; i++) {
        if (__this->audio_buf[i]) {
            free(__this->audio_buf[i]);
            __this->audio_buf[i] = NULL;
        }
    }

    if (__this->cap_buf) {
        free(__this->cap_buf);
        __this->cap_buf = NULL;
    }

    return 0;
}

static void get_isp_lv_timer_cb(void *p)
{
    static int last_scene;
    int scene;
    scene = user_isp_get_current_scene(0);

    //有效值0/1/2/3, 夜晚:0
    switch (scene) {
    case 0:
        if (db_select("hlw") == 0) {
            return;
        }
        video_rec_post_msg("HlightOn");
        break;
    case 1:
    case 2:
    case 3:
        if (!last_scene) {
            video_rec_post_msg("HlightOff");
        }
        break;
    default:
        log_w("err isp scene val:%d", scene);
        break;
    }
    last_scene = scene;
}

/*
 *菜单相关的函数
 */
static int video_rec_change_status(struct intent *it)
{
    if (!strcmp(it->data, "opMENU:")) { /* ui要求打开rec菜单 */
        puts("ui ask me to opMENU:.\n");

        if ((__this->state != VIDREC_STA_START) && (__this->state != VIDREC_STA_FORBIDDEN)) { /* 允许ui打开菜单 */
            __this->menu_inout = 1;
            if (db_select("mot")) {
                ve_mdet_stop();
            }
            if (db_select("lan")) {
                ve_lane_det_stop(0);
            }

            it->data = "opMENU:en";
        } else { /* 禁止ui打开菜单 */
            it->data = "opMENU:dis";
        }

    } else if (!strcmp(it->data, "exitMENU")) { /* ui已经关闭rec菜单 */
        puts("ui tell me exitMENU.\n");
        __this->menu_inout = 0;
        video_rec_get_remain_time();
        video_rec_fun_restore();
        if (db_select("mot")) {
            ve_mdet_start();
        }
        if (db_select("lan")) {
            ve_lane_det_start(0);
        }
    } else if (!strcmp(it->data, "sdCard:")) {
        video_rec_get_remain_time();
        if (storage_device_ready() == 0) {
            it->data = "offline";
        } else {
            it->data = "online";
        }
    } else {
        puts("unknow status ask by ui.\n");
    }

    return 0;
}

static int video_rec_get_status(struct intent *it)
{
    puts("ui ask me rec status\n");
    if (rec_park_flag) {
        it->action = ACTION_VIDEO_REC_PARKING;
        rec_park_flag--;
    }
    return 0;
}


/*
 *录像的状态机,进入录像app后就是跑这里
 */
static int video_rec_state_machine(struct application *app, enum app_state state, struct intent *it)
{
    int err = 0;
    int len;

    switch (state) {
    case APP_STA_CREATE:
        log_d("\n >>>>>>> video_rec: create\n");
        malloc_stats();
        memset(__this, 0, sizeof(struct video_rec_hdl));
        /* video_rec_buf_alloc(); */
        /* server_load(video_server); */
        video_set_disp_window();
        video_rec_config_init();
        __this->state = VIDREC_STA_IDLE;
        break;
    case APP_STA_START:
        puts("--------app_rec: APP_STA_START\n");
        break;
    case APP_STA_PAUSE:
        puts("--------app_rec: APP_STA_PAUSE\n");
        video_rec_fscan_release(0);
        video_rec_fscan_release(1);
        break;
    case APP_STA_RESUME:
        puts("--------app_rec: APP_STA_RESUME\n");
        break;
    case APP_STA_STOP:
        puts("--------app_rec: APP_STA_STOP\n");
        if (__this->state == VIDREC_STA_START) {
            video_rec_stop(0);
            ve_mdet_stop();
            ve_lane_det_stop(0);
        }
#ifdef CONFIG_NET_ENABLE
        video_rec_all_stop_notify();
#endif
        if (video_rec_uninit()) {
            err = 1;
            break;
        }
        break;
    case APP_STA_DESTROY:
        puts("--------app_rec: APP_STA_DESTROY\n");
        if (video_rec_mode_sw()) {
            err = 2;
            break;
        }
        f_free_cache(CONFIG_ROOT_PATH);
        malloc_stats();
        log_d("<<<<<<< video_rec: destroy\n");
        break;
    }
    return err;
}


/*
 *录像app的按键响应函数
 */
static int video_rec_key_event_handler(struct key_event *key)
{
    int err;

    printf("key value:%d ", key->value);
    switch (key->action) {
    case KEY_EVENT_CLICK:
        switch (key->value) {
        case KEY_OK:
            /* case 0: */
            app_send_message(APP_MSG_REC_STATE, 0);
            break;
        case KEY_MENU:
            break;
        case KEY_MODE:
            /* case 7: */
            app_send_message(APP_MSG_REC_MODE, 0);
            break;
        case KEY_UP:
            app_send_message(APP_MSG_REC_PHOTO, 0);
            break;
        case KEY_DOWN:
            app_send_message(APP_MSG_REC_MUTE, 0);
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }

    return false;
}

/*
 *设备响应函数
 */
static int video_rec_device_event_handler(struct sys_event *sys_eve)
{
    struct device_event *device_eve = (struct device_event *)sys_eve->payload;
    int dev_id = 0;
    char *type = NULL;
    static u32 uvc_offline = 0;
    /*
     * SD卡插拔处理
     */
    if (sys_eve->from == DEVICE_EVENT_FROM_SD) {
        dev_id = ((const char *)device_eve->arg)[2] - '0';

        switch (device_eve->event) {
        case DEVICE_EVENT_IN:

            video_rec_sd_in();
            if (storage_device_available()) {
                video_rec_post_msg("sdStatus", 1);
            }


            break;
        case DEVICE_EVENT_OUT:
            if (!fdir_exist(CONFIG_STORAGE_PATH)) {
                video_rec_sd_out();
                video_rec_post_msg("sdStatus", 0);
                video_rec_post_msg("Remain:s=%4", ((u32)(0)));

            }

            break;
        }
    } else if (sys_eve->from == DEVICE_EVENT_FROM_POWER) {
        if (device_eve->event == POWER_EVENT_POWER_CHANGE) {
            int battery_per = sys_power_get_battery_persent();
            video_rec_post_msg("batIcon", battery_per);
        } else if (device_eve->event == POWER_EVENT_POWER_WARNING) {
            int power_warn = sys_power_get_battery_persent();
            video_rec_post_msg("batIcon", power_warn);
        } else if (device_eve->event == POWER_EVENT_POWER_LOW) {
            int power_low = sys_power_get_battery_persent();
            video_rec_post_msg("batIcon", power_low);
        } else if (device_eve->event == POWER_EVENT_POWER_AUTOOFF) {
        } else if (device_eve->event == POWER_EVENT_POWER_CHARGE) {
            video_rec_post_msg("batIcon", 0xff);
        }

#ifdef CONFIG_VIDEO2_ENABLE
    } else if (sys_eve->from == DEVICE_EVENT_FROM_USB_HOST && !strncmp((const char *)device_eve->value, "uvc", 3)) {
        switch (device_eve->event) {
        case DEVICE_EVENT_IN:


#if TCFG_HOST_UVC_ENABLE
            __this->uvc_id  = uvc_host_online();
#endif
            if (!__this->video_online[2]) {
                __this->video_online[2] = true;
#ifndef CONFIG_UI_STYLE_LY_ENABLE
                video_rec_post_msg("swWinicon", 1);
#endif
                printf("UVC or msd_storage online : %s, id=%d\n", type, __this->uvc_id);

                if (__this->state == VIDREC_STA_START) {
                    video_rec_stop(0);
                    video_rec_start();
                }
                /* video_set_disp_window(); */
                video_disp_win_switch(DISP_WIN_SW_DEV_IN, 0);


            }
            break;
        case DEVICE_EVENT_OUT:
            puts("DEVICE_EVENT_OFFLINE: uvc\n");
            if (__this->video_online[2]) {
                __this->video_online[2] = false;
#ifndef CONFIG_UI_STYLE_LY_ENABLE
                if (__this->video_online[0] && __this->video_online[1]) {
                    video_rec_post_msg("swWinicon", 1);
                } else if (__this->video_online[0] || __this->video_online[1]) {
                    video_rec_post_msg("swWinicon", 0);
                }
#endif
                video_disp_win_switch(DISP_WIN_SW_DEV_OUT, 0);

                if (__this->state == VIDREC_STA_START) {
                    video_rec_stop(0);
                    video_rec_start();
                }
            }
            break;
        }
#endif

    } else if (sys_eve->from == DEVICE_EVENT_FROM_VIDEO && !strncmp((const char *)device_eve->arg, "parking", 7)) {
        /* } else if (!ASCII_StrCmp(event->arg, "parking", 7)) { */
        switch (device_eve->event) {
        case DEVICE_EVENT_IN:
            puts("parking on\n");	//parking on

            rec_park_flag++;
#ifdef CONFIG_UI_ENABLE
            extern void parking_page_show(int arg);
            lvgl_rpc_post_func(parking_page_show, 1, 0);
#endif
            video_disp_win_switch(DISP_WIN_SW_SHOW_PARKING, 0);
            sys_power_auto_shutdown_pause();

            return true;

        case DEVICE_EVENT_OUT://parking off
            puts("parking off\n");

#ifdef CONFIG_UI_ENABLE
            extern void parking_page_hide(int arg);
            lvgl_rpc_post_func(parking_page_hide, 1, 0);
#endif
            video_disp_win_switch(DISP_WIN_SW_HIDE_PARKING, 0);
            if (__this->state == VIDREC_STA_START) {
                sys_power_auto_shutdown_pause();
            }
            return true;
        }

    } else if (sys_eve->from == DEVICE_EVENT_FROM_VIDEO && !strncmp((const char *)device_eve->arg, "video_rec_time", 14)) {
        switch (device_eve->event) {
        case DEVICE_EVENT_CHANGE:
            video_rec_post_msg("videoRecTime:s=%4", ((u32)(device_eve->value)));
            return false;

        }
    }

#ifdef CONFIG_GSENSOR_ENABLE
    else if (sys_eve->from == DEVICE_EVENT_FROM_SENSOR && !strncmp((const char *)device_eve->arg, "gsen_lock", 9)) {
        switch (device_eve->event) {
        case DEVICE_EVENT_CHANGE:
            if (__this->state == VIDREC_STA_START) {
                if (db_select("gra")) {
                    __this->gsen_lock = 0xff;
                    video_rec_post_msg("lockREC");
                }
            }
            break;
        }
    }
#endif
    return false;
}

void in_app_stop_display(u8 state)
{
    printf("============ %s , state:%d\n", __func__, state);
}

void out_app_start_display()
{
    printf("============ %s\n", __func__);
}

u8 get_now_video_state()        //获取当前录像状态
{
    printf("============ state:%d\n", __this->state);
    return __this->state;
}


static int video_rec_msg_handler(struct application *app, int *msg)
{
    struct intent *it = NULL;
    int err = 0;
#ifdef UVC_CMA_GRAY
    // VGA UVC黑白切换功能
    void *uvc_test;
    static u32 flag = 0;
#endif
    switch (msg[0]) {
    case APP_MSG_SUSPEND:
        break;
    case APP_MSG_RESUME:
        break;
    case APP_MSG_STOP:
        break;
    case APP_MSG_REC_STATE:
        if (__this->state == VIDREC_STA_START) {
            err = video_rec_stop(0);
            ve_mdet_reset();
            ve_lane_det_reset();
            __this->user_rec = 0;
        } else {
            err = video_rec_start();
            __this->user_rec = 1;
        }
        break;
    case APP_MSG_REC_MODE:
        if ((__this->state != VIDREC_STA_STOP) && (__this->state != VIDREC_STA_IDLE)) {
            if (__this->state == VIDREC_STA_START) {
                if (!__this->gsen_lock) {
                    __this->gsen_lock = 0xff;
                    video_rec_post_msg("lockREC");
                } else {
                    __this->gsen_lock = 0;
                    video_rec_post_msg("unlockREC");
                }
            }
            return true;
        }
        break;
    case APP_MSG_REC_PHOTO:
        video_rec_take_photo();
        break;
    case APP_MSG_REC_MUTE:
#ifdef UVC_CMA_GRAY
        uvc_test = dev_open("uvc", 0);
        dev_ioctl(uvc_test, UVCIOC_SET_CUR_GRAY, flag);
        dev_close(uvc_test);
        flag = ~flag;
#endif
#ifdef PHOTO_STICKER_ENABLE_SMALL_MEM
        __this->sticker_num--;
        if (__this->sticker_num < 0) {
            __this->sticker_num = STICKER_NUM;
        }
        log_d("__this->sticker_num %d\n", __this->sticker_num);
        switch_sticker();
        db_update("stk", __this->sticker_num);
        /* db_flush(); */
        break;
#endif
        video_disp_win_switch(DISP_WIN_SW_SHOW_NEXT, 0);
        mic_set_toggle();
        video_rec_aud_mute();
        break;
    case APP_MSG_REC_MAIN:
        /* it = (struct intent *)msg[1]; */
        /* printf("it:%p", it); */
        printf("\n -[function] %s -[line] %d\n", __FUNCTION__, __LINE__);
        video_rec_init();
        break;
    case APP_MSG_REC_SET_CONFIG:
        it = (struct intent *)msg[1];
        video_rec_set_config(it);
        /* db_flush(); */
        if (it->data && !strcmp(it->data, "res")) {
            video_rec_get_remain_time();
        }
        if (it->data && !strcmp(it->data, "gap")) {
            __this->need_restart_rec = 1;
            video_rec_get_remain_time();
        }
        if (it->data && !strcmp(it->data, "mic")) {
            video_rec_aud_mute();
        }
        free(it);
        it = NULL;
        break;
    case APP_MSG_REC_CHANGE_STATUS:
        it = (struct intent *)msg[1];
        video_rec_change_status(it);
        free(it);
        it = NULL;
        break;
    case APP_MSG_REC_PARKING_STATUS:
        it = (struct intent *)msg[1];
        video_rec_get_status(it);
        free(it);
        it = NULL;
        break;
    case APP_MSG_REC_GET_REMAIN_TIME:
        video_rec_get_remain_time();
        break;
#ifdef USE_LVGL_V8_UI_DEMO
    case APP_MSG_REC_CONTROL:
        if (__this->state == VIDREC_STA_START) {

#ifdef CONFIG_NET_ENABLE
            net_video_rec_event_stop();
#endif

            video_rec_stop(0);
            ve_mdet_reset();
            ve_lane_det_reset();
        } else {

#ifdef CONFIG_NET_ENABLE
            net_video_rec_event_stop();
#endif

            video_rec_stop(0);
            video_rec_start();
#ifdef CONFIG_NET_ENABLE
            net_video_rec_event_start();
#endif

        }
#ifdef CONFIG_NET_ENABLE
        net_video_rec_status_notify();
#endif
        break;
    case APP_MSG_REC_LOCK_FILE:
        it = (struct intent *)msg[1];
        if (it->data && !strcmp(it->data, "get_lock_statu")) {
            it->exdata = !!__this->gsen_lock;
            break;
        }

        if (__this->state == VIDREC_STA_START) {
            __this->gsen_lock = it->exdata ? 0xff : 0;
        }
        free(it);
        it = NULL;
        break;
    case APP_MSG_REC_SWITCH_WIN:
        video_disp_win_switch(DISP_WIN_SW_SHOW_NEXT, 0);
        break;
#endif
    default:
        break;
    }
    return 0;
}

/*录像app的事件总入口*/

/*录像app的事件总入口*/
static int video_rec_event_handler(struct application *app, struct sys_event *event)
{
    switch (event->type) {
    case SYS_KEY_EVENT:
        return video_rec_key_event_handler((struct key_event *)event->payload);
    case SYS_DEVICE_EVENT:
        return video_rec_device_event_handler(event);
    default:
        return false;
    }
}


static const struct application_operation video_rec_ops = {
    .state_machine  = video_rec_state_machine,
    .event_handler 	= video_rec_event_handler,
    .msg_handler    = video_rec_msg_handler,
};

REGISTER_APPLICATION(app_video_rec) = {
    .name 	= "video_rec",
    .action	= ACTION_VIDEO_REC_MAIN,
    .ops 	= &video_rec_ops,
    .state  = APP_STA_DESTROY,
};


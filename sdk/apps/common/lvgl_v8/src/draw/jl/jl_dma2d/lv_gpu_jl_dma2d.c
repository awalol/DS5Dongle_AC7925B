/**
 * @file lv_gpu_jl_dma2d.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "asm/dma2d_driver.h"
#include "lv_gpu_jl_dma2d.h"
#include "../../../core/lv_refr.h"
#include "../../../extra/jl_extra/libs/rle/lv_rle.h"

#include "fs/fs.h"

/* 由硬件判断 colorkey(背景透明色) 开关，1:启用 0:回退软件判断逻辑 */
#ifndef LV_USE_HW_COLORKEY
#define LV_USE_HW_COLORKEY 1
#endif

#if LV_USE_GPU_JL_DMA2D

/*********************
 *      DEFINES
 *********************/

#if LV_COLOR_16_SWAP
#error "Can't use DMA2D with LV_COLOR_16_SWAP 1"
#endif

#if LV_COLOR_DEPTH == 8
#error "Can't use DMA2D with LV_COLOR_DEPTH == 8"
#endif

#if LV_COLOR_DEPTH == 16
#if LV_COLOR_DEPTH_EXTEN == 24
#define LV_DMA2D_COLOR_FORMAT JLDMA2D_FORMAT_ARGB8565
#else
#define LV_DMA2D_COLOR_FORMAT JLDMA2D_FORMAT_RGB565
#endif
#elif LV_COLOR_DEPTH == 32
#define LV_DMA2D_COLOR_FORMAT JLDMA2D_FORMAT_ARGB8888
#else
/*Can't use GPU with other formats*/
#endif

#define LV_JL_DMA2D_BE    0   //大小端配置

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

void lv_draw_jl_dma2d_blend(lv_draw_ctx_t *draw_ctx, const lv_draw_sw_blend_dsc_t *dsc);

static void lv_draw_jl_dma2d_buffer_copy(lv_draw_ctx_t *draw_ctx,
        void *dest_buf, lv_coord_t dest_stride, const lv_area_t *dest_area,
        void *src_buf, lv_coord_t src_stride, const lv_area_t *src_area);

static void lv_draw_jl_dma2d_blend_fill(lv_color_t *dest_buf, lv_coord_t dest_stride, const lv_area_t *fill_area,
                                        lv_color_t color);

static void lv_draw_jl_dma2d_blend_map(lv_color_t *dest_buf, const lv_area_t *dest_area, lv_coord_t dest_stride,
                                       const lv_color_t *src_buf, lv_coord_t src_stride, lv_opa_t opa, uint8_t compress_type, uint16_t start_x, uint16_t start_y);

static void lv_draw_jl_dma2d_blend_fill_argb8565(lv_color_t *dest_buf, lv_coord_t dest_stride, const lv_area_t *fill_area,
        lv_color_t color);

static void lv_draw_jl_dma2d_blend_map_argb8565(lv_color_t *dest_buf, const lv_area_t *dest_area, lv_coord_t dest_stride,
        const lv_color_t *src_buf, lv_coord_t src_stride, lv_opa_t opa, uint8_t compress_type, uint16_t start_x, uint16_t start_y);

static void lv_draw_jl_dma2d_img_decoded(lv_draw_ctx_t *draw, const lv_draw_img_dsc_t *dsc,
        const lv_area_t *coords, const uint8_t *map_p, lv_img_cf_t color_format);


static void lv_draw_jl_dma2d_blend_fill_with_mask(lv_color_t *dest_buf, lv_coord_t dest_stride, const lv_opa_t *mask, lv_coord_t mask_stride, const lv_area_t *fill_area,
        lv_color_t color, lv_opa_t opa);

static void lv_draw_jl_dma2d_blend_fill_with_mask_argb8565(lv_color_t *dest_buf, lv_coord_t dest_stride, const lv_opa_t *mask, lv_coord_t mask_stride, const lv_area_t *fill_area,
        lv_color_t color, lv_opa_t opa);

static void lv_draw_jl_dma2d_blend_fill_with_opa(lv_color_t *dest_buf, lv_coord_t dest_stride, const lv_area_t *fill_area,
        lv_color_t color, lv_opa_t opa);

static void lv_draw_jl_dma2d_blend_fill_with_opa_argb8565(lv_color_t *dest_buf, lv_coord_t dest_stride, const lv_area_t *fill_area,
        lv_color_t color, lv_opa_t opa);

static void lv_draw_jl_dma2d_blend_map_with_mask(lv_color_t *dest_buf, lv_coord_t dest_stride, lv_color_t *src_buf, lv_coord_t src_stride, const lv_opa_t *mask, lv_coord_t mask_stride, const lv_area_t *blend_area, lv_opa_t opa);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * Turn on the peripheral and set output color mode, this only needs to be done once
 */
void lv_flush_inv_dcache(uint8_t *buf, uint32_t w, uint32_t h, uint32_t stride, uint8_t color_depth)
{
    /* 区域刷新Cache */
    uint8_t *area_ptr = (uint8_t *)buf;
    uint32_t width_bytes = w * color_depth >> 3;
    uint32_t offset = stride * color_depth >> 3;
    for (int y = 0; y < h; y++) {
        DcuFlushinvRegion((u32 *)area_ptr, width_bytes);
        area_ptr += offset;
    }
}
void lv_draw_jl_dma2d_init(void)
{
    dma2d_init();
}

void lv_draw_jl_dma2d_ctx_init(lv_disp_drv_t *drv, lv_draw_ctx_t *draw_ctx)
{
    lv_draw_jl_dma2d_ctx_t *dma2d_draw_ctx = (lv_draw_sw_ctx_t *)draw_ctx;

    dma2d_draw_ctx->blend = lv_draw_jl_dma2d_blend;
    dma2d_draw_ctx->base_draw.wait_for_finish = lv_gpu_jl_dma2d_wait_cb;
    dma2d_draw_ctx->base_draw.buffer_copy = lv_draw_jl_dma2d_buffer_copy;
}

void lv_draw_jl_dma2d_ctx_deinit(lv_disp_drv_t *drv, lv_draw_ctx_t *draw_ctx)
{
    LV_UNUSED(drv);
    LV_UNUSED(draw_ctx);
}

static int dma2d_rgb_fpc(uint8_t *dest_bufc, uint32_t dest_stride, uint32_t dest_format, uint8_t *src_bufc, uint32_t src_stride, uint32_t src_format, uint32_t dest_w, uint32_t dest_h)
{
    dma2d_out_layer_params_t out_layer_param = {0};
    dma2d_input_layer_params_t fg_layer_param = {0};

    fg_layer_param.data = src_bufc;
    fg_layer_param.stride = src_stride;
    fg_layer_param.format = src_format;

    out_layer_param.data = dest_bufc;
    out_layer_param.stride = dest_stride;
    out_layer_param.format = dest_format;

    out_layer_param.w = dest_w;
    out_layer_param.h = dest_h;
    out_layer_param.endian = 0;
    out_layer_param.rbs = 0;
    out_layer_param.dither_en = 1;
    dma2d_set_fg_layer(&fg_layer_param);
    dma2d_set_out_layer(&out_layer_param);
    dma2d_set_mode(JLDMA2D_M2M_PFC);
    return 0;
}

static void dma2d_rle_copy(uint8_t *dest_bufc, uint32_t dest_stride, uint8_t *src_bufc, uint32_t format, uint32_t dest_w, uint32_t dest_h, uint16_t start_x, uint16_t start_y)
{
    dma2d_out_layer_params_t out_layer_param = {0};
    dma2d_input_layer_params_t fg_layer_param = {0};

    rle_info_t *rle_info;

    rle_info = (rle_info_t *)src_bufc;

    fg_layer_param.data = src_bufc + rle_info->addr;
    fg_layer_param.rle_en = 1;
    fg_layer_param.x = start_x;
    fg_layer_param.y = start_y;

    fg_layer_param.clut = src_bufc + rle_info->lut_addr;
    fg_layer_param.clut_size = rle_info->lut_len;
    fg_layer_param.clut_format = rle_info->lut_format;
    fg_layer_param.image_size = rle_info->len;

    fg_layer_param.stride = lv_rle_get_stride(rle_info, NULL);
    fg_layer_param.format = rle_info->format;
    fg_layer_param.alpha = 0;
    fg_layer_param.alpha_mode = JLDMA2D_NO_MODIF_ALPHA;
    fg_layer_param.rbs = 0;

    out_layer_param.format = format;
    out_layer_param.data = dest_bufc;
    out_layer_param.w = dest_w;
    out_layer_param.h = dest_h;
    out_layer_param.stride = dest_stride;
    out_layer_param.alpha_inv = 0;
    out_layer_param.rbs = 0;
    out_layer_param.dither_en = 1;

    dma2d_set_fg_layer(&fg_layer_param);
    dma2d_set_out_layer(&out_layer_param);
    dma2d_set_mode(JLDMA2D_M2M_PFC);

}

void lv_draw_jl_dma2d_blend(lv_draw_ctx_t *draw_ctx, const lv_draw_sw_blend_dsc_t *dsc)
{
    lv_area_t blend_area;
    uint16_t start_x = 0;
    uint16_t start_y = 0;
    if (!_lv_area_intersect(&blend_area, dsc->blend_area, draw_ctx->clip_area)) {
        return;
    }

    bool done = false;

    lv_disp_t *disp = _lv_refr_get_disp_refreshing();

    const lv_opa_t *mask;
    if (dsc->mask_buf == NULL) {
        mask = NULL;
    }
    if (dsc->mask_buf && dsc->mask_res == LV_DRAW_MASK_RES_TRANSP) {
        return;
    } else if (dsc->mask_res == LV_DRAW_MASK_RES_FULL_COVER) {
        mask = NULL;
    } else {
        mask = dsc->mask_buf;
    }

    if (dsc->blend_mode == LV_BLEND_MODE_NORMAL && disp->driver->set_px_cb == NULL /* && lv_area_get_size(&blend_area) > 100 */) {
        lv_coord_t dest_stride = lv_area_get_width(draw_ctx->buf_area);

#if LV_COLOR_DEPTH_EXTEN == 24
        typedef struct {
            uint8_t blue;
            uint8_t green;
            uint8_t red;
        } lv_color_exten_t;

        lv_color_exten_t *dest_buf = draw_ctx->buf;
#else
        lv_color_t *dest_buf = draw_ctx->buf;
#endif

        const lv_color_t *src_buf = dsc->src_buf;

#if LV_COLOR_SCREEN_TRANSP && LV_COLOR_DEPTH == 16
        if (disp->driver->screen_transp == 1) {
            //argb8565
            {
                uint8_t *dest_buf8 = (uint8_t *)dest_buf;
                dest_buf8 += dest_stride * (blend_area.y1 - draw_ctx->buf_area->y1) * LV_IMG_PX_SIZE_ALPHA_BYTE;
                dest_buf8 += (blend_area.x1 - draw_ctx->buf_area->x1) * LV_IMG_PX_SIZE_ALPHA_BYTE;
                dest_buf = (lv_color_t *)dest_buf8;
            }
            if (mask == NULL) {
                if (src_buf) {
                    lv_coord_t src_stride;
                    src_stride = lv_area_get_width(dsc->blend_area);
                    if (draw_ctx->compress_type != LV_COMPRESS_RLE) {
                        src_buf += src_stride * (blend_area.y1 - dsc->blend_area->y1) + (blend_area.x1 - dsc->blend_area->x1);
                    }
                    start_x = blend_area.x1 - dsc->blend_area->x1;
                    start_y = blend_area.y1 - dsc->blend_area->y1;
                    lv_area_move(&blend_area, -draw_ctx->buf_area->x1, -draw_ctx->buf_area->y1);
                    lv_draw_jl_dma2d_blend_map_argb8565(dest_buf, &blend_area, dest_stride, src_buf, src_stride, dsc->opa, draw_ctx->compress_type, start_x, start_y);
                    done = true;
                } else {
                    if (dsc->opa >= LV_OPA_MAX) {
                        lv_area_move(&blend_area, -draw_ctx->buf_area->x1, -draw_ctx->buf_area->y1);
                        lv_draw_jl_dma2d_blend_fill_argb8565(dest_buf, dest_stride, &blend_area, dsc->color);
                        done = true;
                    } else {
                        lv_area_move(&blend_area, -draw_ctx->buf_area->x1, -draw_ctx->buf_area->y1);
                        lv_draw_jl_dma2d_blend_fill_with_opa_argb8565(dest_buf, dest_stride, &blend_area, dsc->color, dsc->opa);
                        done = true;
                    }
                }
            } else {
                lv_coord_t mask_stride = lv_area_get_width(dsc->mask_area);
                mask += mask_stride * (blend_area.y1 - dsc->mask_area->y1) + (blend_area.x1 - dsc->mask_area->x1);

                if (!src_buf) {
                    lv_area_move(&blend_area, -draw_ctx->buf_area->x1, -draw_ctx->buf_area->y1);
                    lv_draw_jl_dma2d_blend_fill_with_mask_argb8565(dest_buf, dest_stride, mask, mask_stride, &blend_area, dsc->color, dsc->opa);

                    done = true;
                }

            }

        } else
#endif
        {
            dest_buf += dest_stride * (blend_area.y1 - draw_ctx->buf_area->y1) + (blend_area.x1 - draw_ctx->buf_area->x1);
            if (mask == NULL) {
                if (src_buf) {
                    lv_coord_t src_stride;
                    src_stride = lv_area_get_width(dsc->blend_area);
                    if (draw_ctx->compress_type != LV_COMPRESS_RLE) {
                        src_buf += src_stride * (blend_area.y1 - dsc->blend_area->y1) + (blend_area.x1 - dsc->blend_area->x1);
                    }

                    start_x = blend_area.x1 - dsc->blend_area->x1;
                    start_y = blend_area.y1 - dsc->blend_area->y1;
                    lv_area_move(&blend_area, -draw_ctx->buf_area->x1, -draw_ctx->buf_area->y1);
                    lv_draw_jl_dma2d_blend_map(dest_buf, &blend_area, dest_stride, src_buf, src_stride, dsc->opa, draw_ctx->compress_type, start_x, start_y);
                    done = true;
                } else {
                    if (dsc->opa >= LV_OPA_MAX) {
                        lv_area_move(&blend_area, -draw_ctx->buf_area->x1, -draw_ctx->buf_area->y1);
                        lv_draw_jl_dma2d_blend_fill(dest_buf, dest_stride, &blend_area, dsc->color);
                        done = true;
                    } else {
                        lv_area_move(&blend_area, -draw_ctx->buf_area->x1, -draw_ctx->buf_area->y1);
                        lv_draw_jl_dma2d_blend_fill_with_opa(dest_buf, dest_stride, &blend_area, dsc->color, dsc->opa);
                        done = true;
                    }
                }
            } else {
                lv_coord_t mask_stride = lv_area_get_width(dsc->mask_area);

                mask += mask_stride * (blend_area.y1 - dsc->mask_area->y1) + (blend_area.x1 - dsc->mask_area->x1);
                if (!src_buf) {
                    lv_area_move(&blend_area, -draw_ctx->buf_area->x1, -draw_ctx->buf_area->y1);
                    lv_draw_jl_dma2d_blend_fill_with_mask(dest_buf, dest_stride, mask, mask_stride, &blend_area, dsc->color, dsc->opa);
                    done = true;
                } else {
                    lv_coord_t src_stride;
                    src_stride = lv_area_get_width(dsc->blend_area);
                    src_buf += src_stride * (blend_area.y1 - dsc->blend_area->y1) + (blend_area.x1 - dsc->blend_area->x1);
                    lv_area_move(&blend_area, -draw_ctx->buf_area->x1, -draw_ctx->buf_area->y1);
                    lv_draw_jl_dma2d_blend_map_with_mask(dest_buf, dest_stride, src_buf, src_stride, mask, mask_stride, &blend_area, dsc->opa);
                    done = true;
                }
            }
        }
    }

    if (!done) {
        lv_draw_sw_blend_basic(draw_ctx, dsc);
    }
}

static void lv_draw_jl_dma2d_buffer_copy(lv_draw_ctx_t *draw_ctx, void *dest_buf, lv_coord_t dest_stride,
        const lv_area_t *dest_area, void *src_buf, lv_coord_t src_stride, const lv_area_t *src_area)
{
    LV_UNUSED(draw_ctx);
    uint16_t src_w = (int32_t)lv_area_get_width(src_area);
    uint16_t src_h = (int32_t)lv_area_get_height(src_area);
    uint16_t dest_w = (int32_t)lv_area_get_width(dest_area);
    uint16_t dest_h = (int32_t)lv_area_get_height(dest_area);
    lv_flush_inv_dcache(src_buf, src_w, src_h, src_stride, LV_COLOR_DEPTH_EXTEN);
    lv_flush_inv_dcache(dest_buf, dest_w, dest_h, dest_stride, LV_COLOR_DEPTH_EXTEN);

#if LV_COLOR_DEPTH_EXTEN == 24
    typedef struct {
        uint8_t blue;
        uint8_t green;
        uint8_t red;
    } lv_color_exten_t;
    lv_color_exten_t *dest_bufc = dest_buf;
    lv_color_exten_t *src_bufc = src_buf;
#else
    lv_color_t *dest_bufc =  dest_buf;
    lv_color_t *src_bufc =  src_buf;
#endif

    /*Got the first pixel of each buffer*/
    dest_bufc += dest_stride * dest_area->y1;
    dest_bufc += dest_area->x1;

    src_bufc += src_stride * src_area->y1;
    src_bufc += src_area->x1;
    dma2d_reset_all_regs();
    dma2d_create_task();
    dma2d_copy(dest_bufc, (dest_stride * LV_COLOR_DEPTH_EXTEN) >> 3, src_bufc, (src_stride * LV_COLOR_DEPTH_EXTEN) >> 3, LV_DMA2D_COLOR_FORMAT, dest_w, dest_h);
    dma2d_run();
    dma2d_wait_done();
    /* DcuInvalidRegion(dest_buf,(dest_w * dest_h * LV_COLOR_DEPTH) >> 3); */
}

static void lv_draw_jl_dma2d_img_decoded(lv_draw_ctx_t *draw_ctx, const lv_draw_img_dsc_t *dsc,
        const lv_area_t *coords, const uint8_t *map_p, lv_img_cf_t color_format)
{
    /*TODO basic ARGB8888 image can be handles here*/

    lv_draw_sw_img_decoded(draw_ctx, dsc, coords, map_p, color_format);
}

static void lv_draw_jl_dma2d_blend_fill(lv_color_t *dest_buf, lv_coord_t dest_stride, const lv_area_t *fill_area,
                                        lv_color_t color)
{
    /*Simply fill an area*/
    int32_t area_w = lv_area_get_width(fill_area);
    int32_t area_h = lv_area_get_height(fill_area);

#if 1
    uint32_t packed_color;

#if LV_COLOR_DEPTH == 16
    packed_color = dma2d_pack_pixel(0xff, color.ch.red << 3, LV_COLOR_GET_G16(color) << 2, color.ch.blue << 3, JLDMA2D_FORMAT_ARGB8888);
#else
    packed_color = dma2d_pack_pixel(color.ch.alpha, color.ch.red, color.ch.green, color.ch.blue, JLDMA2D_FORMAT_ARGB8888);
#endif

    lv_flush_inv_dcache(dest_buf, area_w, area_h, dest_stride, LV_COLOR_DEPTH_EXTEN);

    dma2d_reset_all_regs();
    dma2d_create_task();
    dma2d_fill(dest_buf, (dest_stride * LV_COLOR_DEPTH_EXTEN) >> 3, LV_DMA2D_COLOR_FORMAT, LV_JL_DMA2D_BE, packed_color, area_w, area_h);
    /* dma2d_dump_all_tasks(); */
    dma2d_run();
    dma2d_wait_done();

#else
    uint8_t *buf;
    for (uint32_t y = 0; y < area_h; y++) {
        for (uint32_t x = 0; x < area_w; x++) {
            /* dest_buf[y * dest_stride + x] = color; */
            buf = (uint8_t *)&dest_buf[y * dest_stride + x];
            buf[0] = (color.full) & 0xff;
            buf[1] = (color.full >> 8) & 0xff;
        }
    }
#endif
}

static void lv_draw_jl_dma2d_blend_fill_with_opa(lv_color_t *dest_buf, lv_coord_t dest_stride, const lv_area_t *fill_area,
        lv_color_t color, lv_opa_t opa)
{
    /*Simply fill an area*/
    int32_t area_w = lv_area_get_width(fill_area);
    int32_t area_h = lv_area_get_height(fill_area);

    uint32_t packed_color;

#if LV_COLOR_DEPTH == 16
    packed_color = dma2d_pack_pixel(opa, color.ch.red << 3, LV_COLOR_GET_G16(color) << 2, color.ch.blue << 3, JLDMA2D_FORMAT_ARGB8888);
#else
    packed_color = dma2d_pack_pixel((uint32_t)color.ch.alpha * (uint32_t)opa / 255, color.ch.red, color.ch.green, color.ch.blue, JLDMA2D_FORMAT_ARGB8888);
#endif

    dma2d_out_layer_params_t out_layer_param = {0};
    dma2d_input_layer_params_t fg_layer_param = {0};
    dma2d_input_layer_params_t bg_layer_param = {0};

    lv_flush_inv_dcache(dest_buf, area_w, area_h, dest_stride, LV_COLOR_DEPTH_EXTEN);
    out_layer_param.data = dest_buf;
    out_layer_param.w = area_w;
    out_layer_param.h = area_h;
    out_layer_param.stride = (dest_stride * LV_COLOR_DEPTH_EXTEN) >> 3;
    out_layer_param.format = LV_DMA2D_COLOR_FORMAT;
    out_layer_param.rbs = 0;
    out_layer_param.endian = LV_JL_DMA2D_BE;

    fg_layer_param.paintmode = 1;
    fg_layer_param.data = NULL;
    fg_layer_param.stride = 0;
    fg_layer_param.format = JLDMA2D_FORMAT_ARGB8888;
    fg_layer_param.alpha_mode = JLDMA2D_NO_MODIF_ALPHA;
    fg_layer_param.rbs = 0;
    fg_layer_param.color = packed_color;

    bg_layer_param.data = (uint32_t)dest_buf;
    bg_layer_param.stride = (dest_stride * LV_COLOR_DEPTH_EXTEN) >> 3;
    bg_layer_param.format = LV_DMA2D_COLOR_FORMAT;
    bg_layer_param.alpha_mode = JLDMA2D_NO_MODIF_ALPHA;
    bg_layer_param.rbs = 0;

#if LV_COLOR_DEPTH_EXTEN == 24
    bg_layer_param.colorkey = 1;
    bg_layer_param.color = 0x52aaa5;
#endif

    dma2d_reset_all_regs();
    dma2d_create_task();
    dma2d_set_fg_layer(&fg_layer_param);
    dma2d_set_bg_layer(&bg_layer_param);
    dma2d_set_out_layer(&out_layer_param);
    dma2d_set_mode(JLDMA2D_M2M_BLEND);
    /* dma2d_dump_all_tasks(); */
    dma2d_run();
    dma2d_wait_done();

}

static void lv_draw_jl_dma2d_blend_map(lv_color_t *dest_buf, const lv_area_t *dest_area, lv_coord_t dest_stride,
                                       const lv_color_t *src_buf, lv_coord_t src_stride, lv_opa_t opa, uint8_t compress_type, uint16_t start_x, uint16_t start_y)
{

    /*Simple copy*/
    int32_t dest_w = lv_area_get_width(dest_area);
    int32_t dest_h = lv_area_get_height(dest_area);
    rle_info_t *rle_info;

    if (opa >= LV_OPA_MAX) {

#if 1
        lv_flush_inv_dcache(src_buf, dest_w, dest_h, src_stride, LV_COLOR_DEPTH_EXTEN);
        lv_flush_inv_dcache(dest_buf, dest_w, dest_h, dest_stride, LV_COLOR_DEPTH_EXTEN);
        /* DcuFlushRegion(src_buf,(dest_w * dest_h * LV_COLOR_DEPTH) >> 3); */
        dma2d_reset_all_regs();
        dma2d_create_task();
        if (compress_type != LV_COMPRESS_RLE) {
#if LV_COLOR_DEPTH_EXTEN == 24
            dma2d_rgb_fpc(dest_buf, (dest_stride * LV_COLOR_DEPTH_EXTEN) >> 3, JLDMA2D_FORMAT_ARGB8565, src_buf, (src_stride * LV_COLOR_DEPTH) >> 3, JLDMA2D_FORMAT_RGB565, dest_w, dest_h);
#else
            dma2d_copy(dest_buf, (dest_stride * LV_COLOR_DEPTH_EXTEN) >> 3, src_buf, (src_stride * LV_COLOR_DEPTH_EXTEN) >> 3, LV_DMA2D_COLOR_FORMAT, dest_w, dest_h);
#endif
        } else {
            dma2d_rle_copy(dest_buf, (dest_stride * LV_COLOR_DEPTH_EXTEN) >> 3, src_buf, LV_DMA2D_COLOR_FORMAT, dest_w, dest_h, start_x, start_y);
        }
        dma2d_run();
        dma2d_wait_done();
        /* DcuInvalidRegion(dest_buf,(dest_w * dest_h * LV_COLOR_DEPTH) >> 3); */
#else

        lv_color_t temp_buf[1024];
        for (uint32_t y = 0; y < dest_h; y++) {
            memcpy(temp_buf, &src_buf[y * src_stride], dest_w * sizeof(lv_color_t));
            memcpy(&dest_buf[y * dest_stride], temp_buf, dest_w * sizeof(lv_color_t));
        }
#endif
    } else {


        dma2d_out_layer_params_t out_layer_param =  {0};
        dma2d_input_layer_params_t fg_layer_param = {0};
        dma2d_input_layer_params_t bg_layer_param = {0};

        out_layer_param.data = dest_buf;
        out_layer_param.w = dest_w;
        out_layer_param.h = dest_h;
        out_layer_param.stride = (dest_stride * LV_COLOR_DEPTH_EXTEN) >> 3;
        out_layer_param.format = LV_DMA2D_COLOR_FORMAT;
        out_layer_param.rbs = 0;
        out_layer_param.dither_en = 1;
        out_layer_param.endian = LV_JL_DMA2D_BE;
        /* lv_flush_inv_dcache(dest_buf,dest_w,dest_h,dest_stride, LV_COLOR_DEPTH); */

        if (compress_type == LV_COMPRESS_RLE) {
            rle_info = (rle_info_t *)src_buf;
            fg_layer_param.data = (uint8_t *)src_buf + rle_info->addr;
            fg_layer_param.rle_en = 1;
            fg_layer_param.x = start_x;
            fg_layer_param.y = start_y;
            fg_layer_param.clut = src_buf + rle_info->lut_addr;
            fg_layer_param.clut_size = rle_info->lut_len;
            fg_layer_param.clut_format = rle_info->lut_format;
            fg_layer_param.image_size = rle_info->len;
            fg_layer_param.stride = lv_rle_get_stride(rle_info, NULL);
            fg_layer_param.format = rle_info->format;
        } else {
            fg_layer_param.data = (uint32_t)src_buf;
            fg_layer_param.stride = (src_stride * LV_COLOR_DEPTH_EXTEN) >> 3;
            fg_layer_param.format = LV_DMA2D_COLOR_FORMAT;
            lv_flush_inv_dcache(src_buf, dest_w, dest_h, src_stride, LV_COLOR_DEPTH_EXTEN);
        }
        fg_layer_param.alpha = opa;
        fg_layer_param.alpha_mode = JLDMA2D_COMBINE_ALPHA;
        fg_layer_param.rbs = 0;


        bg_layer_param.data = (uint32_t)dest_buf;
        bg_layer_param.stride = (dest_stride * LV_COLOR_DEPTH_EXTEN) >> 3;
        bg_layer_param.format = LV_DMA2D_COLOR_FORMAT;
        bg_layer_param.alpha_mode = JLDMA2D_NO_MODIF_ALPHA;
        bg_layer_param.rbs = 0;
        lv_flush_inv_dcache(dest_buf, dest_w, dest_h, dest_stride, LV_COLOR_DEPTH_EXTEN);

        dma2d_reset_all_regs();
        dma2d_create_task();
        dma2d_set_fg_layer(&fg_layer_param);
        dma2d_set_bg_layer(&bg_layer_param);
        dma2d_set_out_layer(&out_layer_param);
        dma2d_set_mode(JLDMA2D_M2M_BLEND);
        dma2d_run();
        dma2d_wait_done();
        /* DcuInvalidRegion(dest_buf,(dest_w * dest_h * LV_COLOR_DEPTH) >> 3); */

    }
}


static void lv_draw_jl_dma2d_blend_fill_with_mask(lv_color_t *dest_buf, lv_coord_t dest_stride, const lv_opa_t *mask, lv_coord_t mask_stride, const lv_area_t *fill_area,
        lv_color_t color, lv_opa_t opa)
{
    /*Simply fill an area*/
    int32_t area_w = lv_area_get_width(fill_area);
    int32_t area_h = lv_area_get_height(fill_area);

    uint32_t packed_color;

#if LV_COLOR_DEPTH == 16
    packed_color = dma2d_pack_pixel(0xff, color.ch.red << 3, LV_COLOR_GET_G16(color) << 2, color.ch.blue << 3, JLDMA2D_FORMAT_ARGB8888);
#else
    packed_color = dma2d_pack_pixel(color.ch.alpha, color.ch.red, color.ch.green, color.ch.blue, JLDMA2D_FORMAT_ARGB8888);
#endif

    dma2d_out_layer_params_t out_layer_param = {0};
    dma2d_input_layer_params_t fg_layer_param = {0};
    dma2d_input_layer_params_t bg_layer_param = {0};

    lv_flush_inv_dcache(dest_buf, area_w, area_h, dest_stride, LV_COLOR_DEPTH_EXTEN);
    out_layer_param.data = dest_buf;
    out_layer_param.w = area_w;
    out_layer_param.h = area_h;
    out_layer_param.stride = (dest_stride * LV_COLOR_DEPTH_EXTEN) >> 3;
    out_layer_param.format = LV_DMA2D_COLOR_FORMAT;
    out_layer_param.rbs = 0;
    out_layer_param.endian = LV_JL_DMA2D_BE;

    lv_flush_inv_dcache(mask, area_w, area_h, mask_stride, 8);
    fg_layer_param.data = (uint32_t)mask;
    fg_layer_param.stride = mask_stride;
    fg_layer_param.format = JLDMA2D_FORMAT_A8;
    fg_layer_param.alpha = opa;
    fg_layer_param.alpha_mode = JLDMA2D_COMBINE_ALPHA;
    fg_layer_param.rbs = 0;
    fg_layer_param.color = packed_color;

    bg_layer_param.data = (uint8_t *)dest_buf;
    bg_layer_param.stride = (dest_stride * LV_COLOR_DEPTH_EXTEN) >> 3;
    bg_layer_param.format = LV_DMA2D_COLOR_FORMAT;
    bg_layer_param.alpha_mode = JLDMA2D_NO_MODIF_ALPHA;
    bg_layer_param.rbs = 0;

#if LV_USE_HW_COLORKEY
    dma2d_reset_all_regs();
    dma2d_create_task();
    dma2d_set_fg_layer(&fg_layer_param);

#if LV_COLOR_DEPTH_EXTEN == 24
    /* 背景 colorkey 像素逐像素视为透明：文字抗锯齿边缘与连写字形叠加都能正确混合 */
    bg_layer_param.colorkey = 1;
    bg_layer_param.color = 0x52aaa5;
#endif
    dma2d_set_bg_layer(&bg_layer_param);
    dma2d_set_out_layer(&out_layer_param);
    dma2d_set_mode(JLDMA2D_M2M_BLEND);
#else
    dma2d_reset_all_regs();
    dma2d_create_task();
    dma2d_set_fg_layer(&fg_layer_param);
    dma2d_set_bg_layer(&bg_layer_param);
    dma2d_set_out_layer(&out_layer_param);

#if LV_COLOR_DEPTH_EXTEN == 24
    if (jlvg_get_bgcolor_is_color_key(dest_buf, LV_COLOR_KEY)) {
        dma2d_set_mode(JLDMA2D_M2M_PFC);
    } else {
        dma2d_set_mode(JLDMA2D_M2M_BLEND);
    }
#else
    dma2d_set_mode(JLDMA2D_M2M_BLEND);
#endif
#endif

    dma2d_run();
    dma2d_wait_done();

}

static void lv_draw_jl_dma2d_blend_map_with_mask(lv_color_t *dest_buf, lv_coord_t dest_stride, lv_color_t *src_buf, lv_coord_t src_stride, const lv_opa_t *mask, lv_coord_t mask_stride, const lv_area_t *blend_area, lv_opa_t opa)
{
    int32_t area_w = lv_area_get_width(blend_area);
    int32_t area_h = lv_area_get_height(blend_area);

    dma2d_out_layer_params_t out_layer_param = {0};
    dma2d_input_layer_params_t fg_layer_param = {0};
    dma2d_input_layer_params_t bg_layer_param = {0};

    dma2d_reset_all_regs();
    dma2d_create_task();

    uint8_t *tmp_buf = lv_mem_alloc(area_w * area_h * 4);

    lv_flush_inv_dcache(dest_buf, area_w, area_h, dest_stride, LV_COLOR_DEPTH);
    out_layer_param.data = tmp_buf;
    out_layer_param.w = area_w;
    out_layer_param.h = area_h;
    out_layer_param.stride = area_w * 4;
    out_layer_param.format = JLDMA2D_FORMAT_ARGB8888;
    out_layer_param.endian = LV_JL_DMA2D_BE;
    out_layer_param.dither_en = 1;

    lv_flush_inv_dcache(src_buf, area_w, area_h, src_stride, LV_COLOR_DEPTH);
    fg_layer_param.data = (uint32_t)src_buf;
    fg_layer_param.stride = (src_stride * LV_COLOR_DEPTH) >> 3;

#if LV_COLOR_DEPTH_EXTEN == 24
    fg_layer_param.format = JLDMA2D_FORMAT_RGB565;
#else
    fg_layer_param.format = LV_DMA2D_COLOR_FORMAT;
#endif

    lv_flush_inv_dcache(mask, area_w, area_h, mask_stride, 8);
    bg_layer_param.data = (uint32_t)mask;
    bg_layer_param.stride = mask_stride;
    bg_layer_param.format = JLDMA2D_FORMAT_A8;
    bg_layer_param.alpha_mode = JLDMA2D_NO_MODIF_ALPHA;

    dma2d_set_fg_layer(&fg_layer_param);
    dma2d_set_bg_layer(&bg_layer_param);
    dma2d_set_out_layer(&out_layer_param);
    dma2d_set_mode(JLDMA2D_M2M_BLEND);
    dma2d_set_blend_mode(1);

    dma2d_create_task();
    lv_flush_inv_dcache(mask, area_w, area_h, mask_stride, 8);
    fg_layer_param.data = (uint32_t)tmp_buf;
    fg_layer_param.stride = area_w * 4;
    fg_layer_param.format = JLDMA2D_FORMAT_ARGB8888;
    fg_layer_param.alpha_mode = JLDMA2D_COMBINE_ALPHA;
    fg_layer_param.alpha = opa;

    bg_layer_param.data = (uint32_t)dest_buf;
    bg_layer_param.stride = (dest_stride * LV_COLOR_DEPTH_EXTEN) >> 3;
    bg_layer_param.format = LV_DMA2D_COLOR_FORMAT;
    bg_layer_param.alpha = 0;
    bg_layer_param.alpha_mode = JLDMA2D_NO_MODIF_ALPHA;
    out_layer_param.data = dest_buf;
    out_layer_param.w = area_w;
    out_layer_param.h = area_h;
    out_layer_param.stride = (dest_stride * LV_COLOR_DEPTH_EXTEN) >> 3;
    out_layer_param.format = LV_DMA2D_COLOR_FORMAT;
    dma2d_set_fg_layer(&fg_layer_param);
    dma2d_set_bg_layer(&bg_layer_param);
    dma2d_set_out_layer(&out_layer_param);

#if LV_COLOR_DEPTH_EXTEN == 24
    if (jlvg_get_bgcolor_is_color_key(dest_buf, LV_COLOR_KEY)) {
        dma2d_set_mode(JLDMA2D_M2M_PFC);
    } else {
        dma2d_set_mode(JLDMA2D_M2M_BLEND);
    }
#else
    dma2d_set_mode(JLDMA2D_M2M_BLEND);
#endif

    dma2d_run();
    dma2d_wait_done();

    if (tmp_buf) {
        lv_mem_free(tmp_buf);
    }

}

static void lv_draw_jl_dma2d_blend_fill_argb8565(lv_color_t *dest_buf, lv_coord_t dest_stride, const lv_area_t *fill_area,
        lv_color_t color)
{
    int32_t area_w = lv_area_get_width(fill_area);
    int32_t area_h = lv_area_get_height(fill_area);

    uint32_t packed_color;

#if LV_COLOR_DEPTH == 16
    uint32_t green = LV_COLOR_GET_G16(color);
#else
    uint32_t green = color.ch.green;
#endif
    packed_color = dma2d_pack_pixel(0xff, color.ch.red << 3, green << 2, color.ch.blue << 3, JLDMA2D_FORMAT_ARGB8888);

    lv_flush_inv_dcache(dest_buf, area_w, area_h, dest_stride, 24);
    dma2d_reset_all_regs();
    dma2d_create_task();
    dma2d_fill(dest_buf, (dest_stride * 24) >> 3, JLDMA2D_FORMAT_ARGB8565, LV_JL_DMA2D_BE, packed_color, area_w, area_h);
    dma2d_run();
    dma2d_wait_done();
}

static void lv_draw_jl_dma2d_blend_map_argb8565(lv_color_t *dest_buf, const lv_area_t *dest_area, lv_coord_t dest_stride,
        const lv_color_t *src_buf, lv_coord_t src_stride, lv_opa_t opa, uint8_t compress_type, uint16_t start_x, uint16_t start_y)
{

    rle_info_t *rle_info;
    /*Simple copy*/
    int32_t dest_w = lv_area_get_width(dest_area);
    int32_t dest_h = lv_area_get_height(dest_area);

    dma2d_out_layer_params_t out_layer_param = {0};
    dma2d_input_layer_params_t fg_layer_param = {0};
    dma2d_input_layer_params_t bg_layer_param = {0};

    out_layer_param.data = dest_buf;
    out_layer_param.w = dest_w;
    out_layer_param.h = dest_h;
    out_layer_param.stride = (dest_stride * 24) >> 3;
    out_layer_param.format = JLDMA2D_FORMAT_ARGB8565;
    out_layer_param.rbs = 0;
    out_layer_param.endian = LV_JL_DMA2D_BE;
    out_layer_param.dither_en = 1;

    if (compress_type == LV_COMPRESS_RLE) {
        rle_info = (rle_info_t *)src_buf;
        fg_layer_param.data = (uint8_t *)src_buf + rle_info->addr;
        fg_layer_param.rle_en = 1;
        fg_layer_param.x = start_x;
        fg_layer_param.y = start_y;
        fg_layer_param.clut = src_buf + rle_info->lut_addr;
        fg_layer_param.clut_size = rle_info->lut_len;
        fg_layer_param.clut_format = rle_info->lut_format;
        fg_layer_param.image_size = rle_info->len;
        fg_layer_param.stride = lv_rle_get_stride(rle_info, NULL);
        fg_layer_param.format = rle_info->format;
    } else {
        fg_layer_param.data = (uint32_t)src_buf;
        fg_layer_param.stride = (src_stride * LV_COLOR_DEPTH_EXTEN) >> 3;
        fg_layer_param.format = LV_DMA2D_COLOR_FORMAT;
        lv_flush_inv_dcache(src_buf, dest_w, dest_h, src_stride, LV_COLOR_DEPTH);
    }
    fg_layer_param.alpha = opa;
    fg_layer_param.alpha_mode = JLDMA2D_COMBINE_ALPHA;
    fg_layer_param.rbs = 0;

    bg_layer_param.data = (uint32_t)dest_buf;
    bg_layer_param.stride = (dest_stride * 24) >> 3;
    bg_layer_param.format = JLDMA2D_FORMAT_ARGB8565;
    bg_layer_param.alpha_mode = JLDMA2D_NO_MODIF_ALPHA;
    bg_layer_param.rbs = 0;
    lv_flush_inv_dcache(dest_buf, dest_w, dest_h, dest_stride, 24);

    dma2d_reset_all_regs();
    dma2d_create_task();
    dma2d_set_fg_layer(&fg_layer_param);
    dma2d_set_bg_layer(&bg_layer_param);
    dma2d_set_out_layer(&out_layer_param);
    dma2d_set_mode(JLDMA2D_M2M_BLEND);
    dma2d_run();
    dma2d_wait_done();
}


static void lv_draw_jl_dma2d_blend_fill_with_opa_argb8565(lv_color_t *dest_buf, lv_coord_t dest_stride, const lv_area_t *fill_area,
        lv_color_t color, lv_opa_t opa)
{
    int32_t area_w = lv_area_get_width(fill_area);
    int32_t area_h = lv_area_get_height(fill_area);

    uint32_t packed_color;

#if LV_COLOR_DEPTH == 16
    packed_color = dma2d_pack_pixel(opa, color.ch.red << 3, LV_COLOR_GET_G16(color) << 2, color.ch.blue << 3, JLDMA2D_FORMAT_ARGB8888);
#else
    packed_color = dma2d_pack_pixel((uint32_t)color.ch.alpha * (uint32_t)opa / 255, color.ch.red, color.ch.green, color.ch.blue, JLDMA2D_FORMAT_ARGB8888);
#endif

    dma2d_out_layer_params_t out_layer_param = {0};
    dma2d_input_layer_params_t fg_layer_param = {0};
    dma2d_input_layer_params_t bg_layer_param = {0};

    lv_flush_inv_dcache(dest_buf, area_w, area_h, dest_stride, 24);

    out_layer_param.data = dest_buf;
    out_layer_param.w = area_w;
    out_layer_param.h = area_h;
    out_layer_param.stride = (dest_stride * 24) >> 3;
    out_layer_param.format = JLDMA2D_FORMAT_ARGB8565;
    out_layer_param.rbs = 0;
    out_layer_param.endian = LV_JL_DMA2D_BE;

    fg_layer_param.paintmode = 1;
    fg_layer_param.data = NULL;
    fg_layer_param.stride = 0;
    fg_layer_param.format = JLDMA2D_FORMAT_ARGB8888;
    fg_layer_param.alpha_mode = JLDMA2D_NO_MODIF_ALPHA;
    fg_layer_param.rbs = 0;
    fg_layer_param.color = packed_color;

    bg_layer_param.data = (uint32_t)dest_buf;
    bg_layer_param.stride = (dest_stride * 24) >> 3;
    bg_layer_param.format = JLDMA2D_FORMAT_ARGB8565;
    bg_layer_param.alpha_mode = JLDMA2D_NO_MODIF_ALPHA;
    bg_layer_param.rbs = 0;

    dma2d_reset_all_regs();
    dma2d_create_task();
    dma2d_set_fg_layer(&fg_layer_param);
    dma2d_set_bg_layer(&bg_layer_param);
    dma2d_set_out_layer(&out_layer_param);
    dma2d_set_mode(JLDMA2D_M2M_BLEND);
    dma2d_run();
    dma2d_wait_done();
}

static void lv_draw_jl_dma2d_blend_fill_with_mask_argb8565(lv_color_t *dest_buf, lv_coord_t dest_stride, const lv_opa_t *mask, lv_coord_t mask_stride, const lv_area_t *fill_area,
        lv_color_t color, lv_opa_t opa)
{
    int32_t area_w = lv_area_get_width(fill_area);
    int32_t area_h = lv_area_get_height(fill_area);

    uint32_t packed_color;

#if LV_COLOR_DEPTH == 16
    uint32_t green = LV_COLOR_GET_G16(color);
#else
    uint32_t green = color.ch.green;
#endif
    packed_color = dma2d_pack_pixel(0xff, color.ch.red << 3, green << 2, color.ch.blue << 3, JLDMA2D_FORMAT_ARGB8888);

    dma2d_out_layer_params_t out_layer_param = {0};
    dma2d_input_layer_params_t fg_layer_param = {0};
    dma2d_input_layer_params_t bg_layer_param = {0};

    lv_flush_inv_dcache(dest_buf, area_w, area_h, dest_stride, 24);
    out_layer_param.data = dest_buf;
    out_layer_param.w = area_w;
    out_layer_param.h = area_h;
    out_layer_param.stride = (dest_stride * 24) >> 3;
    out_layer_param.format = JLDMA2D_FORMAT_ARGB8565;
    out_layer_param.rbs = 0;
    out_layer_param.endian = LV_JL_DMA2D_BE;

    lv_flush_inv_dcache(mask, area_w, area_h, mask_stride, 8);
    fg_layer_param.data = (uint32_t)mask;
    fg_layer_param.stride = mask_stride;
    fg_layer_param.format = JLDMA2D_FORMAT_A8;
    fg_layer_param.alpha = opa;
    fg_layer_param.alpha_mode = JLDMA2D_COMBINE_ALPHA;
    fg_layer_param.rbs = 0;
    fg_layer_param.color = packed_color;

    bg_layer_param.data = (uint32_t)dest_buf;
    bg_layer_param.stride = (dest_stride * 24) >> 3;
    bg_layer_param.format = JLDMA2D_FORMAT_ARGB8565;
    bg_layer_param.alpha_mode = JLDMA2D_NO_MODIF_ALPHA;
    bg_layer_param.rbs = 0;


    dma2d_reset_all_regs();
    dma2d_create_task();
    dma2d_set_fg_layer(&fg_layer_param);
    dma2d_set_bg_layer(&bg_layer_param);
    dma2d_set_out_layer(&out_layer_param);
    dma2d_set_mode(JLDMA2D_M2M_BLEND);
    dma2d_run();
    dma2d_wait_done();
}

void lv_gpu_jl_dma2d_wait_cb(lv_draw_ctx_t *draw_ctx)
{
    lv_disp_t *disp = _lv_refr_get_disp_refreshing();

#if 0
    if (disp->driver && disp->driver->wait_cb) {
        while (DMA2D->CR & DMA2D_CR_START_Msk) {
            disp->driver->wait_cb(disp->driver);
        }
    } else {
        while (DMA2D->CR & DMA2D_CR_START_Msk);
    }
#endif

    lv_draw_sw_wait_for_finish(draw_ctx);
}

#endif


# make 编译并下载
# make VERBOSE=1 显示编译详细过程
# make clean 清除编译临时文件
#
# 注意： Linux 下编译方式：
#     1. 从 http://pkgman.jieliapp.com/doc/all 处找到下载链接
#     2. 下载后，解压到 /opt/jieli 目录下，保证
#       /opt/jieli/common/bin/clang 存在（注意目录层次）
#     3. 确认 ulimit -n 的结果足够大（建议大于8096），否则链接可能会因为打开文件太多而失败
#       可以通过 ulimit -n 8096 来设置一个较大的值
#

# 工具路径设置
ifeq ($(OS), Windows_NT)
# Windows 下工具链位置
TOOL_DIR := C:/JL/pi32/bin
CC    := clang.exe
CXX   := clang.exe
LD    := pi32v2-lto-wrapper.exe
AR    := llvm-ar.exe
MKDIR := mkdir_win -p
RM    := rm -rf

SYS_LIB_DIR := 
SYS_INC_DIR := 
EXT_CFLAGS  := # Windows 下不需要 -D__SHELL__
export PATH:=$(TOOL_DIR);$(PATH)

## 后处理脚本
FIXBAT          := ../../../../tools/utils/fixbat.exe # 用于处理 utf8->gbk 编码问题
POST_SCRIPT     := ../../../../cpu/wl83/tools/download.bat
RUN_POST_SCRIPT := ..\..\..\..\cpu\wl83\tools\download.bat
else
# Linux 下工具链位置
TOOL_DIR := /opt/jieli/pi32v2/bin
CC    := clang
CXX   := clang
LD    := lto-wrapper
AR    := lto-ar
MKDIR := mkdir -p
RM    := rm -rf
export OBJDUMP := $(TOOL_DIR)/objdump
export OBJCOPY := $(TOOL_DIR)/objcopy
export OBJSIZEDUMP := $(TOOL_DIR)/objsizedump

SYS_LIB_DIR := $(TOOL_DIR)/../lib/r3
SYS_INC_DIR := $(TOOL_DIR)/../include
EXT_CFLAGS  := -D__SHELL__ # Linux 下需要这个保证正确处理 download.c
export PATH:=$(TOOL_DIR):$(PATH)

## 后处理脚本
FIXBAT          := touch # Linux下不需要处理 bat 编码问题
POST_SCRIPT     := ../../../../cpu/wl83/tools/download.sh
RUN_POST_SCRIPT := bash $(POST_SCRIPT)
endif

CC  := $(TOOL_DIR)/$(CC)
CXX := $(TOOL_DIR)/$(CXX)
LD  := $(TOOL_DIR)/$(LD)
AR  := $(TOOL_DIR)/$(AR)
# 输出文件设置
OUT_ELF   := ../../../../cpu/wl83/tools/sdk.elf
OBJ_FILE  := $(OUT_ELF).objs.txt
# 编译路径设置
BUILD_DIR := objs
# 工程路径前缀
ROOT_PREFIX := ../../../..

# 编译参数设置
CFLAGS := \
	-target pi32v2 \
	-integrated-as \
	-mcpu=r3 \
	-mfprev1 \
	-Wuninitialized \
	-Wno-invalid-noreturn \
	-fno-common \
	-integrated-as \
	-Oz \
	-g \
	-flto \
	-fallow-pointer-null \
	-fprefer-gnu-section \
	-femulated-tls \
	-Wno-shift-negative-value \
	-Wframe-larger-than=2560 \
	-mllvm -pi32v2-large-program=true \
	-fms-extensions \
	-w \
	-fno-unwind-tables \
	-ffunction-sections \
	-fdata-sections \
	-fmessage-length=0 \


# C++额外的编译参数
CXXFLAGS := \
	-std=gnu++14 \
	-fno-exceptions \
	-fno-rtti \


# 宏定义
DEFINES := \
	-DSUPPORT_MS_EXTENSIONS \
	-DCONFIG_RELEASE_ENABLE \
	-DCONFIG_CPU_WL83 \
	-DMAX_AUDIO_ADC_CHANNEL_NUM=2 \
	-DCONFIG_UCOS_ENABLE \
	-DCONFIG_THREAD_ENABLE \
	-D__GCC_PI32V2__ \
	-DCONFIG_NEW_ECC_ENABLE \
	-DCONFIG_WIFI_DVR_PROJECT_ENABLE \
	-DEVENT_HANDLER_NUM_CONFIG=2 \
	-DEVENT_POOL_SIZE_CONFIG=256 \
	-DAPP_ASYNC_POOL_NUM_CONFIG=8 \
	-DVFS_FILE_POOL_NUM_CONFIG=8 \
	-DVM_MAX_PAGE_ALIGN_SIZE_CONFIG=64*1024 \
	-DVM_MAX_SECTOR_ALIGN_SIZE_CONFIG=64*1024 \
	-DVM_ITEM_MAX_NUM=256 \
	-DCONFIG_BTCTRLER_TASK_DEL_ENABLE \
	-DCONFIG_LMP_CONN_SUSPEND_ENABLE \
	-DCONFIG_LMP_REFRESH_ENCRYPTION_KEY_ENABLE \
	-DLVGL_COMMIT_HASH=\"release/v8.4_2409c8dfc6b8d9ae3f7970e1c333e743e0fd2911\" \
	-DFREETYPE_COMMIT_HASH=\"ef54557bda3b17da1b7e42e8e207efd1842bcc05\" \
	-DFT2_BUILD_LIBRARY \
	-D_POSIX_SOURCE \
	-DFIONREAD \
	-DFIONBIO \
	-D_XOPEN_SOURCE=700 \
	-D_GNU_SOURCE \
	-D__ELF__ \


DEFINES += $(EXT_CFLAGS) # 额外的一些定义

# 头文件搜索路径
INCLUDES := \
	-I../../../../include_lib/c++ \
	-I../../../../include_lib/c++/include \
	-I../../../../include_lib/newlib/include \
	-I../../../../include_lib/c++/simple_pthread \
	-I../../../../include_lib \
	-I../../../../include_lib/driver \
	-I../../../../include_lib/driver/device \
	-I../../../../include_lib/driver/cpu/wl83 \
	-I../../../../include_lib/system \
	-I../../../../include_lib/system/generic \
	-I../../../../include_lib/btctrler \
	-I../../../../include_lib/btctrler/port/wl83 \
	-I../../../../include_lib/update \
	-I../../../../include_lib/btstack/third_party/common \
	-I../../../../include_lib/btstack/third_party/rcsp \
	-I../../../../include_lib/utils \
	-I../../../../include_lib/utils/syscfg \
	-I../../../../include_lib/utils/event \
	-I../../../../include_lib/btstack \
	-I../../../../apps/common/ble/include \
	-I../../../../apps/wifi_camera/include \
	-I../../../../apps/wifi_camera/misc \
	-I../../../../apps/wifi_camera/video_ops/include \
	-I../../../../include_lib/driver/device/video \
	-I../../../../apps \
	-I../../../../apps/common/third_party_profile \
	-I../../../../apps/common/include \
	-I../../../../apps/common/ui/include \
	-I../../../../apps/common/system \
	-I../../../../apps/common/video \
	-I../../../../apps/common/system/perf_counter \
	-I../../../../apps/common/config/include \
	-I../../../../apps/common/usb \
	-I../../../../include_lib/utils/ui \
	-I../../../../include_lib/utils/bplus-tree/include \
	-I../../../../apps/common/usb/device \
	-I../../../../apps/common/usb/host \
	-I../../../../apps/common/usb/include/host \
	-I../../../../apps/common/usb/include/device \
	-I../../../../apps/common/usb/include \
	-I../../../../apps/wifi_camera \
	-I../../../../apps/wifi_camera/board/wl83 \
	-I../../../../apps/common/net \
	-I../../../../include_lib/net \
	-I../../../../include_lib/net/mbedtls_3_4_0 \
	-I../../../../include_lib/net/server \
	-I../../../../include_lib/server \
	-I../../../../include_lib/server/media \
	-I../../../../include_lib/net/wifi_module \
	-I../../../../include_lib/net/hostapdandwpa_supplicant \
	-I../../../../include_lib/net/lwip_2_2_0/lwip/src/include \
	-I../../../../include_lib/net/lwip_2_2_0/lwip/src/include/lwip \
	-I../../../../include_lib/net/lwip_2_2_0/lwip/port \
	-I../../../../include_lib/net/lwip_2_2_0/lwip/app \
	-I../../../../include_lib/net/lwip_2_2_0/lwip/app/ctp \
	-I../../../../include_lib/net/lwip_2_2_0/lwip/app/cdp \
	-I../../../../include_lib/net/lte_module \
	-I../../../../include_lib/driver/device/video/pipeline \
	-I../../../../include_lib/driver/device/video/pipeline/buffer \
	-I../../../../include_lib/driver/device/video/pipeline/core \
	-I../../../../include_lib/system/ui \
	-I../../../../include_lib/driver/cpu/wl83/asm \
	-I../../../../apps/common/lvgl_v8 \
	-I../../../../apps/common/lvgl_v8/src \
	-I../../../../apps/common/lvgl_v8/src/extra \
	-I../../../../apps/common/freetype/include \
	-I../../../../apps/common/lvgl_v8/examples/porting \
	-I../../../../apps/common/lvgl_v8/examples/libs/freetype \
	-I../../../../apps/common \
	-I../../../../apps/common/lvgl_v8/demos \
	-I../../../../apps/common/lvgl_v8/demos/benchmark \
	-I../../../../apps/common/lvgl_v8/demos/keypad_encoder \
	-I../../../../apps/common/lvgl_v8/demos/music \
	-I../../../../apps/common/lvgl_v8/demos/stress \
	-I../../../../apps/common/lvgl_v8/demos/widgets \
	-I../../../../apps/common/lvgl_v8/demos/amap/gps/map \
	-I../../../../apps/common/lvgl_v8/demos/amap/gps \
	-I../../../../apps/common/lvgl_v8/demos/amap \
	-I../../../../apps/common/lvgl_v8/src/draw/jl/jl_dma2d \
	-I../../../../apps/wifi_camera/lvgl_v8_ui_app/style_JL/generated \
	-I../../../../apps/wifi_camera/lvgl_v8_ui_app/style_JL/generated/gui_events \
	-I../../../../apps/wifi_camera/lvgl_v8_ui_app/style_JL/generated/gui_fonts \
	-I../../../../apps/wifi_camera/lvgl_v8_ui_app/style_JL/generated/gui_images \
	-I../../../../apps/wifi_camera/lvgl_v8_ui_app/style_JL/generated/gui_msg \
	-I../../../../apps/wifi_camera/lvgl_v8_ui_app/style_JL/generated/gui_res \
	-I../../../../apps/wifi_camera/lvgl_v8_ui_app/style_JL/generated/gui_src \
	-I../../../../apps/wifi_camera/lvgl_v8_ui_app/style_JL/generated/gui_group \
	-I../../../../apps/wifi_camera/lvgl_v8_ui_app/style_JL/generated/gui_timelines \
	-I../../../../apps/wifi_camera/lvgl_v8_ui_app/style_JL/generated/i18n \
	-I../../../../apps/wifi_camera/lvgl_v8_ui_app/style_JL/custom \
	-I../../../../apps/common/example/third_party/freetype/include \
	-I../../../../include_lib/driver/device/usb \
	-I../../../../include_lib/driver/device/usb/device \
	-I../../../../include_lib/driver/device/usb/host \
	-I../../../../apps/common/rf_fcc_tool/include \
	-I../../../../apps/common/product_test_tool/includes \
	-I../../../../apps/common/gsensor \
	-I../../../../apps/common/ext_uart_bt \
	-I../../../../apps/common/ext_spi_bt \
	-I../../../../include_lib/net/curl \
	-I../../../../include_lib/net/lwip_2_2_0/lwip/src/include/compat/posix \
	-I../../../../include_lib/net/lwip_2_2_0/lwip/src/include/compat/stdc \
	-I../../../../apps/common/video/net_avi_player \
	-I../../../../apps/common/net/testbox/include \
	-I../../../../apps/common/third_party_profile/mijia \
	-I../../../../apps/common/third_party_profile/mijia/mijia_ble_api \
	-I../../../../apps/common/third_party_profile/mijia/mijia_ble_libs \
	-I../../../../apps/common/third_party_profile/alipay \
	-I../../../../apps/common/third_party_profile/ancs_client_demo \
	-I../../../../apps/common/third_party_profile/jieli/rcsp \
	-I../../../../include_lib/btstack/third_party \
	-I../../../../apps/common/lcd/include \
	-I../../../../apps/wifi_camera/mode/bt/le_audio/include \
	-I../../../../apps/wifi_camera/mode/bt/le_audio/include/big \
	-I../../../../apps/wifi_camera/mode/bt/le_audio/include/cig \
	-I../../../../apps/wifi_camera/mode/bt/le_audio/include/auracast \
	-I../../../../apps/common/dev_manager \
	-I../../../../apps/common/file_operate \
	-I../../../../apps/common/debug/online_db \
	-I../../../../audio/interface \
	-I../../../../audio/interface/includes \
	-I../../../../audio \
	-I../../../../audio/effect \
	-I../../../../audio/framework/include \
	-I../../../../audio/framework/plugs/source \
	-I../../../../audio/CVP \
	-I../../../../audio/test_tools \
	-I../../../../audio/cpu/wl83 \
	-I../../../../audio/common \
	-I../../../../audio/common/config \
	-I../../../../include_lib/media \
	-I../../../../include_lib/media/cvp \
	-I../../../../include_lib/media/cpu/wl83 \
	-I../../../../include_lib/media/cpu/wl83/asm \
	-I../../../../include_lib/media/framework/include \
	-I../../../../audio/smart_voice \
	-I../../../../audio/effect/spatial_effect \
	-I../../../../include_lib/media/aispeech/enc \
	-I../../../../audio/cpu/wl83/audio_dai \
	-I../../../../audio/cpu/wl83/audio_common \


# 需要编译的 .c 文件
c_SRC_FILES := \
	../../../../apps/common/asr/jlkws/jlkws.c \
	../../../../apps/common/audio_music/audio_enc_aec_data.c \
	../../../../apps/common/c++/cpp_run_init.c \
	../../../../apps/common/camera/GC1084_DVP/GC1084_DVP.c \
	../../../../apps/common/camera/GC1084_DVP/GC1084_DVP_ae.c \
	../../../../apps/common/camera/GC1084_DVP/GC1084_DVP_awb.c \
	../../../../apps/common/camera/GC1084_DVP/GC1084_DVP_iq.c \
	../../../../apps/common/camera/N5_DVP/N5_DVP.c \
	../../../../apps/common/camera/SP140A_dvp/sp140a_dvp.c \
	../../../../apps/common/camera/SP140A_dvp/sp140a_dvp_ae.c \
	../../../../apps/common/camera/SP140A_dvp/sp140a_dvp_awb.c \
	../../../../apps/common/camera/SP140A_dvp/sp140a_dvp_iq.c \
	../../../../apps/common/camera/TVP5150A_DVP.c \
	../../../../apps/common/camera/bf20a6/bf20a6.c \
	../../../../apps/common/camera/gc0308/gc0308.c \
	../../../../apps/common/camera/gc1084_mipi/gc1084_mipi.c \
	../../../../apps/common/camera/gc1084_mipi/gc1084_mipi_ae.c \
	../../../../apps/common/camera/gc1084_mipi/gc1084_mipi_awb.c \
	../../../../apps/common/camera/gc1084_mipi/gc1084_mipi_iq.c \
	../../../../apps/common/camera/gc2145_mipi/gc2145_mipi.c \
	../../../../apps/common/camera/h63p_dvp/h63p_dvp.c \
	../../../../apps/common/camera/h63p_dvp/h63p_dvp_ae.c \
	../../../../apps/common/camera/h63p_dvp/h63p_dvp_awb.c \
	../../../../apps/common/camera/h63p_dvp/h63p_dvp_iq.c \
	../../../../apps/common/camera/h63p_mipi/h63p_mipi.c \
	../../../../apps/common/camera/h63p_mipi/h63p_mipi_ae.c \
	../../../../apps/common/camera/h63p_mipi/h63p_mipi_awb.c \
	../../../../apps/common/camera/h63p_mipi/h63p_mipi_iq.c \
	../../../../apps/common/camera/isp_scenes.c \
	../../../../apps/common/camera/isp_tools.c \
	../../../../apps/common/camera/nt99141.c \
	../../../../apps/common/camera/ov2659/ov2659.c \
	../../../../apps/common/camera/sc1245_dvp/sc1245.c \
	../../../../apps/common/camera/sc1245_dvp/sc1245_ae.c \
	../../../../apps/common/camera/sc1245_dvp/sc1245_awb.c \
	../../../../apps/common/camera/sc1245_dvp/sc1245_iq.c \
	../../../../apps/common/camera/sc1336_mipi/sc1336_mipi.c \
	../../../../apps/common/camera/sc1336_mipi/sc1336_mipi_ae.c \
	../../../../apps/common/camera/sc1336_mipi/sc1336_mipi_awb.c \
	../../../../apps/common/camera/sc1336_mipi/sc1336_mipi_iq.c \
	../../../../apps/common/camera/sc1345_mipi/SC1345_mipi.c \
	../../../../apps/common/camera/sc1345_mipi/SC1345_mipi_ae.c \
	../../../../apps/common/camera/sc1345_mipi/SC1345_mipi_awb.c \
	../../../../apps/common/camera/sc1345_mipi/SC1345_mipi_iq.c \
	../../../../apps/common/camera/sc1346_mipi/SC1346_mipi.c \
	../../../../apps/common/camera/sc1346_mipi/SC1346_mipi_ae.c \
	../../../../apps/common/camera/sc1346_mipi/SC1346_mipi_awb.c \
	../../../../apps/common/camera/sc1346_mipi/SC1346_mipi_iq.c \
	../../../../apps/common/camera/xs9950_mipi/xs9950_mipi.c \
	../../../../apps/common/config/bt_profile_config.c \
	../../../../apps/common/config/log_config/app_config.c \
	../../../../apps/common/config/log_config/lib_btctrler_config.c \
	../../../../apps/common/config/log_config/lib_btstack_config.c \
	../../../../apps/common/config/log_config/lib_driver_config.c \
	../../../../apps/common/config/log_config/lib_power_config.c \
	../../../../apps/common/config/log_config/lib_system_config.c \
	../../../../apps/common/config/log_config/lib_ui_config.c \
	../../../../apps/common/config/log_config/lib_update_config.c \
	../../../../apps/common/config/user_cfg.c \
	../../../../apps/common/debug/debug.c \
	../../../../apps/common/debug/debug_user.c \
	../../../../apps/common/debug/dlog_config.c \
	../../../../apps/common/debug/dlog_output_config.c \
	../../../../apps/common/dev_manager/dev_manager.c \
	../../../../apps/common/dma2d_gpu/dma2d_common_api.c \
	../../../../apps/common/dma2d_gpu/filter/dma2d_adapter.c \
	../../../../apps/common/dma2d_gpu/filter/gpu_adapter.c \
	../../../../apps/common/example/peripheral/lcd/lcd_driver_test.c \
	../../../../apps/common/example/storage/flash/flash_ex_sdfile/main.c \
	../../../../apps/common/example/third_party/freetype/src/autofit/autofit.c \
	../../../../apps/common/example/third_party/freetype/src/base/ftbase.c \
	../../../../apps/common/example/third_party/freetype/src/base/ftbitmap.c \
	../../../../apps/common/example/third_party/freetype/src/base/ftdebug.c \
	../../../../apps/common/example/third_party/freetype/src/base/ftglyph.c \
	../../../../apps/common/example/third_party/freetype/src/base/ftinit.c \
	../../../../apps/common/example/third_party/freetype/src/base/ftmm.c \
	../../../../apps/common/example/third_party/freetype/src/base/ftsystem.c \
	../../../../apps/common/example/third_party/freetype/src/bdf/bdf.c \
	../../../../apps/common/example/third_party/freetype/src/bzip2/ftbzip2.c \
	../../../../apps/common/example/third_party/freetype/src/cache/ftcache.c \
	../../../../apps/common/example/third_party/freetype/src/cff/cff.c \
	../../../../apps/common/example/third_party/freetype/src/cid/type1cid.c \
	../../../../apps/common/example/third_party/freetype/src/dlg/dlgwrap.c \
	../../../../apps/common/example/third_party/freetype/src/gxvalid/gxvalid.c \
	../../../../apps/common/example/third_party/freetype/src/gzip/ftgzip.c \
	../../../../apps/common/example/third_party/freetype/src/lzw/ftlzw.c \
	../../../../apps/common/example/third_party/freetype/src/otvalid/otvalid.c \
	../../../../apps/common/example/third_party/freetype/src/pcf/pcf.c \
	../../../../apps/common/example/third_party/freetype/src/pfr/pfr.c \
	../../../../apps/common/example/third_party/freetype/src/psaux/psaux.c \
	../../../../apps/common/example/third_party/freetype/src/pshinter/pshinter.c \
	../../../../apps/common/example/third_party/freetype/src/psnames/psnames.c \
	../../../../apps/common/example/third_party/freetype/src/raster/raster.c \
	../../../../apps/common/example/third_party/freetype/src/sdf/sdf.c \
	../../../../apps/common/example/third_party/freetype/src/sfnt/sfnt.c \
	../../../../apps/common/example/third_party/freetype/src/smooth/smooth.c \
	../../../../apps/common/example/third_party/freetype/src/svg/svg.c \
	../../../../apps/common/example/third_party/freetype/src/truetype/truetype.c \
	../../../../apps/common/example/third_party/freetype/src/type1/type1.c \
	../../../../apps/common/example/third_party/freetype/src/type42/type42.c \
	../../../../apps/common/example/third_party/freetype/src/winfonts/winfnt.c \
	../../../../apps/common/example/third_party/sqlite/os_jl.c \
	../../../../apps/common/example/third_party/sqlite/sqlite3.c \
	../../../../apps/common/example/third_party/sqlite/sqlite_test_main.c \
	../../../../apps/common/example/video/image/jpeg/jpeg_disp_frame.c \
	../../../../apps/common/ext_spi_bt/bt_album_manager.c \
	../../../../apps/common/ext_spi_bt/bt_device_manager.c \
	../../../../apps/common/ext_spi_bt/bt_music_manager.c \
	../../../../apps/common/ext_spi_bt/bt_ota_manager.c \
	../../../../apps/common/ext_spi_bt/bt_phone_manager.c \
	../../../../apps/common/ext_spi_bt/bt_test_example.c \
	../../../../apps/common/ext_spi_bt/serial_packager.c \
	../../../../apps/common/ext_spi_bt/serial_unpackager.c \
	../../../../apps/common/ext_spi_bt/spi_bt_driver.c \
	../../../../apps/common/ext_spi_bt/spi_manager.c \
	../../../../apps/common/ext_uart_bt/bt_album_manager.c \
	../../../../apps/common/ext_uart_bt/bt_device_manager.c \
	../../../../apps/common/ext_uart_bt/bt_music_manager.c \
	../../../../apps/common/ext_uart_bt/bt_ota_manager.c \
	../../../../apps/common/ext_uart_bt/bt_phone_manager.c \
	../../../../apps/common/ext_uart_bt/bt_test_example.c \
	../../../../apps/common/ext_uart_bt/serial_packager.c \
	../../../../apps/common/ext_uart_bt/serial_unpackager.c \
	../../../../apps/common/ext_uart_bt/uart_driver.c \
	../../../../apps/common/ext_uart_bt/uart_manager.c \
	../../../../apps/common/fat_nor/extflash.c \
	../../../../apps/common/file_operate/file_bs_deal.c \
	../../../../apps/common/file_operate/file_manager.c \
	../../../../apps/common/gsensor/da380.c \
	../../../../apps/common/gsensor/gSensor_manage.c \
	../../../../apps/common/gsensor/gma301.c \
	../../../../apps/common/gsensor/msa300.c \
	../../../../apps/common/gsensor/stk8312.c \
	../../../../apps/common/iic/iic.c \
	../../../../apps/common/iic/iic_api.c \
	../../../../apps/common/iic/software_iic.c \
	../../../../apps/common/key/adkey.c \
	../../../../apps/common/key/adkey_rtcvdd.c \
	../../../../apps/common/key/ctmu_touch_key.c \
	../../../../apps/common/key/iokey.c \
	../../../../apps/common/key/irkey.c \
	../../../../apps/common/key/key_driver.c \
	../../../../apps/common/key/matrix_keyboard.c \
	../../../../apps/common/key/rdec_key.c \
	../../../../apps/common/key/slidekey.c \
	../../../../apps/common/key/tent600_key.c \
	../../../../apps/common/key/touch_key.c \
	../../../../apps/common/key/uart_key.c \
	../../../../apps/common/lcd/lcd_driver/fb_component/fb_combine.c \
	../../../../apps/common/lcd/lcd_driver/fb_component/fb_lcd.c \
	../../../../apps/common/lcd/lcd_driver/fb_component/fb_out_dev.c \
	../../../../apps/common/lcd/lcd_driver/fb_component/fb_rotate.c \
	../../../../apps/common/lcd/lcd_driver/lcd_driver.c \
	../../../../apps/common/lcd/lcd_driver/lcd_scenes.c \
	../../../../apps/common/lcd/lcd_driver/lcd_tools.c \
	../../../../apps/common/lcd/lcd_driver/mcu_lcd/lcd_mcu_ili9481_320x480.c \
	../../../../apps/common/lcd/lcd_driver/mcu_lcd/lcd_mcu_ili9488_320x480.c \
	../../../../apps/common/lcd/lcd_driver/mcu_lcd/lcd_mcu_st7789v_240x320.c \
	../../../../apps/common/lcd/lcd_driver/mipi_lcd/lcd_mipi_fl7707_480x1280.c \
	../../../../apps/common/lcd/lcd_driver/mipi_lcd/lcd_mipi_ili9881c_800x1280.c \
	../../../../apps/common/lcd/lcd_driver/mipi_lcd/lcd_mipi_jd9165ba_1024x600.c \
	../../../../apps/common/lcd/lcd_driver/mipi_lcd/lcd_mipi_st7701s_480x640_rotate_90.c \
	../../../../apps/common/lcd/lcd_driver/mipi_lcd/lcd_mipi_st7701s_480x800.c \
	../../../../apps/common/lcd/lcd_driver/mipi_lcd/lcd_mipi_st7701s_480x800_rotate_90.c \
	../../../../apps/common/lcd/lcd_driver/mipi_lcd/lcd_mipi_st7703_720x720.c \
	../../../../apps/common/lcd/lcd_driver/mipi_lcd/lcd_mipi_st77922_480x480.c \
	../../../../apps/common/lcd/lcd_driver/rgb_lcd/lcd_rgb_fl58202_800x480.c \
	../../../../apps/common/lcd/lcd_driver/rgb_lcd/lcd_rgb_hx8264_800x480.c \
	../../../../apps/common/lcd/lcd_driver/rgb_lcd/lcd_rgb_ssd2828_720x1280.c \
	../../../../apps/common/lcd/lcd_driver/rgb_lcd/lcd_rgb_st7701s_480x800.c \
	../../../../apps/common/lcd/lcd_driver/rgb_lcd/lcd_rgb_xxxx_1280x800.c \
	../../../../apps/common/lcd/lcd_driver/rgb_lcd/lcd_rgb_xxxx_480x272.c \
	../../../../apps/common/lcd/lcd_driver/spi_lcd/lcd_spi_gc9307_172x320.c \
	../../../../apps/common/lcd/lcd_driver/spi_lcd/lcd_spi_gc9307_240x296.c \
	../../../../apps/common/lcd/lcd_driver/spi_lcd/lcd_spi_nv3006a_142x428.c \
	../../../../apps/common/lcd/touch_driver/cst3240.c \
	../../../../apps/common/lcd/touch_driver/cst816d.c \
	../../../../apps/common/lcd/touch_driver/ft6236.c \
	../../../../apps/common/lcd/touch_driver/gt1151.c \
	../../../../apps/common/lcd/touch_driver/gt911.c \
	../../../../apps/common/lcd/touch_driver/gt9271.c \
	../../../../apps/common/lcd/touch_driver/tp_driver.c \
	../../../../apps/common/lvgl_v8/demos/amap/amap_canvas.c \
	../../../../apps/common/lvgl_v8/demos/amap/amap_non_canvas.c \
	../../../../apps/common/lvgl_v8/demos/benchmark/assets/img_benchmark_cogwheel_alpha16.c \
	../../../../apps/common/lvgl_v8/demos/benchmark/assets/img_benchmark_cogwheel_argb.c \
	../../../../apps/common/lvgl_v8/demos/benchmark/assets/img_benchmark_cogwheel_chroma_keyed.c \
	../../../../apps/common/lvgl_v8/demos/benchmark/assets/img_benchmark_cogwheel_indexed16.c \
	../../../../apps/common/lvgl_v8/demos/benchmark/assets/img_benchmark_cogwheel_rgb.c \
	../../../../apps/common/lvgl_v8/demos/benchmark/assets/img_benchmark_cogwheel_rgb565a8.c \
	../../../../apps/common/lvgl_v8/demos/benchmark/assets/lv_font_bechmark_montserrat_12_compr_az.c.c \
	../../../../apps/common/lvgl_v8/demos/benchmark/assets/lv_font_bechmark_montserrat_16_compr_az.c.c \
	../../../../apps/common/lvgl_v8/demos/benchmark/assets/lv_font_bechmark_montserrat_28_compr_az.c.c \
	../../../../apps/common/lvgl_v8/demos/benchmark/lv_demo_benchmark.c \
	../../../../apps/common/lvgl_v8/demos/keypad_encoder/lv_demo_keypad_encoder.c \
	../../../../apps/common/lvgl_v8/demos/music/assets/img_lv_demo_music_btn_corner_large.c \
	../../../../apps/common/lvgl_v8/demos/music/assets/img_lv_demo_music_btn_list_pause.c \
	../../../../apps/common/lvgl_v8/demos/music/assets/img_lv_demo_music_btn_list_pause_large.c \
	../../../../apps/common/lvgl_v8/demos/music/assets/img_lv_demo_music_btn_list_play.c \
	../../../../apps/common/lvgl_v8/demos/music/assets/img_lv_demo_music_btn_list_play_large.c \
	../../../../apps/common/lvgl_v8/demos/music/assets/img_lv_demo_music_btn_loop.c \
	../../../../apps/common/lvgl_v8/demos/music/assets/img_lv_demo_music_btn_loop_large.c \
	../../../../apps/common/lvgl_v8/demos/music/assets/img_lv_demo_music_btn_next.c \
	../../../../apps/common/lvgl_v8/demos/music/assets/img_lv_demo_music_btn_next_large.c \
	../../../../apps/common/lvgl_v8/demos/music/assets/img_lv_demo_music_btn_pause.c \
	../../../../apps/common/lvgl_v8/demos/music/assets/img_lv_demo_music_btn_pause_large.c \
	../../../../apps/common/lvgl_v8/demos/music/assets/img_lv_demo_music_btn_play.c \
	../../../../apps/common/lvgl_v8/demos/music/assets/img_lv_demo_music_btn_play_large.c \
	../../../../apps/common/lvgl_v8/demos/music/assets/img_lv_demo_music_btn_prev.c \
	../../../../apps/common/lvgl_v8/demos/music/assets/img_lv_demo_music_btn_prev_large.c \
	../../../../apps/common/lvgl_v8/demos/music/assets/img_lv_demo_music_btn_rnd.c \
	../../../../apps/common/lvgl_v8/demos/music/assets/img_lv_demo_music_btn_rnd_large.c \
	../../../../apps/common/lvgl_v8/demos/music/assets/img_lv_demo_music_corner_left.c \
	../../../../apps/common/lvgl_v8/demos/music/assets/img_lv_demo_music_corner_left_large.c \
	../../../../apps/common/lvgl_v8/demos/music/assets/img_lv_demo_music_corner_right.c \
	../../../../apps/common/lvgl_v8/demos/music/assets/img_lv_demo_music_corner_right_large.c \
	../../../../apps/common/lvgl_v8/demos/music/assets/img_lv_demo_music_cover_1.c \
	../../../../apps/common/lvgl_v8/demos/music/assets/img_lv_demo_music_cover_1_large.c \
	../../../../apps/common/lvgl_v8/demos/music/assets/img_lv_demo_music_cover_2.c \
	../../../../apps/common/lvgl_v8/demos/music/assets/img_lv_demo_music_cover_2_large.c \
	../../../../apps/common/lvgl_v8/demos/music/assets/img_lv_demo_music_cover_3.c \
	../../../../apps/common/lvgl_v8/demos/music/assets/img_lv_demo_music_cover_3_large.c \
	../../../../apps/common/lvgl_v8/demos/music/assets/img_lv_demo_music_icon_1.c \
	../../../../apps/common/lvgl_v8/demos/music/assets/img_lv_demo_music_icon_1_large.c \
	../../../../apps/common/lvgl_v8/demos/music/assets/img_lv_demo_music_icon_2.c \
	../../../../apps/common/lvgl_v8/demos/music/assets/img_lv_demo_music_icon_2_large.c \
	../../../../apps/common/lvgl_v8/demos/music/assets/img_lv_demo_music_icon_3.c \
	../../../../apps/common/lvgl_v8/demos/music/assets/img_lv_demo_music_icon_3_large.c \
	../../../../apps/common/lvgl_v8/demos/music/assets/img_lv_demo_music_icon_4.c \
	../../../../apps/common/lvgl_v8/demos/music/assets/img_lv_demo_music_icon_4_large.c \
	../../../../apps/common/lvgl_v8/demos/music/assets/img_lv_demo_music_list_border.c \
	../../../../apps/common/lvgl_v8/demos/music/assets/img_lv_demo_music_list_border_large.c \
	../../../../apps/common/lvgl_v8/demos/music/assets/img_lv_demo_music_logo.c \
	../../../../apps/common/lvgl_v8/demos/music/assets/img_lv_demo_music_slider_knob.c \
	../../../../apps/common/lvgl_v8/demos/music/assets/img_lv_demo_music_slider_knob_large.c \
	../../../../apps/common/lvgl_v8/demos/music/assets/img_lv_demo_music_wave_bottom.c \
	../../../../apps/common/lvgl_v8/demos/music/assets/img_lv_demo_music_wave_bottom_large.c \
	../../../../apps/common/lvgl_v8/demos/music/assets/img_lv_demo_music_wave_top.c \
	../../../../apps/common/lvgl_v8/demos/music/assets/img_lv_demo_music_wave_top_large.c \
	../../../../apps/common/lvgl_v8/demos/music/lv_demo_music.c \
	../../../../apps/common/lvgl_v8/demos/music/lv_demo_music_list.c \
	../../../../apps/common/lvgl_v8/demos/music/lv_demo_music_main.c \
	../../../../apps/common/lvgl_v8/demos/stress/lv_demo_stress.c \
	../../../../apps/common/lvgl_v8/demos/widgets/assets/img_clothes.c \
	../../../../apps/common/lvgl_v8/demos/widgets/assets/img_demo_widgets_avatar.c \
	../../../../apps/common/lvgl_v8/demos/widgets/assets/img_lvgl_logo.c \
	../../../../apps/common/lvgl_v8/demos/widgets/lv_demo_widgets.c \
	../../../../apps/common/lvgl_v8/examples/anim/lv_example_anim_1.c \
	../../../../apps/common/lvgl_v8/examples/anim/lv_example_anim_2.c \
	../../../../apps/common/lvgl_v8/examples/anim/lv_example_anim_3.c \
	../../../../apps/common/lvgl_v8/examples/anim/lv_example_anim_timeline_1.c \
	../../../../apps/common/lvgl_v8/examples/assets/animimg001.c \
	../../../../apps/common/lvgl_v8/examples/assets/animimg002.c \
	../../../../apps/common/lvgl_v8/examples/assets/animimg003.c \
	../../../../apps/common/lvgl_v8/examples/assets/emoji/img_emoji_F617.c \
	../../../../apps/common/lvgl_v8/examples/assets/img_caret_down.c \
	../../../../apps/common/lvgl_v8/examples/assets/img_cogwheel_alpha16.c \
	../../../../apps/common/lvgl_v8/examples/assets/img_cogwheel_argb.c \
	../../../../apps/common/lvgl_v8/examples/assets/img_cogwheel_chroma_keyed.c \
	../../../../apps/common/lvgl_v8/examples/assets/img_cogwheel_indexed16.c \
	../../../../apps/common/lvgl_v8/examples/assets/img_cogwheel_rgb.c \
	../../../../apps/common/lvgl_v8/examples/assets/img_hand.c \
	../../../../apps/common/lvgl_v8/examples/assets/img_skew_strip.c \
	../../../../apps/common/lvgl_v8/examples/assets/img_star.c \
	../../../../apps/common/lvgl_v8/examples/assets/imgbtn_left.c \
	../../../../apps/common/lvgl_v8/examples/assets/imgbtn_mid.c \
	../../../../apps/common/lvgl_v8/examples/assets/imgbtn_right.c \
	../../../../apps/common/lvgl_v8/examples/event/lv_example_event_1.c \
	../../../../apps/common/lvgl_v8/examples/event/lv_example_event_2.c \
	../../../../apps/common/lvgl_v8/examples/event/lv_example_event_3.c \
	../../../../apps/common/lvgl_v8/examples/event/lv_example_event_4.c \
	../../../../apps/common/lvgl_v8/examples/get_started/lv_example_get_started_1.c \
	../../../../apps/common/lvgl_v8/examples/get_started/lv_example_get_started_2.c \
	../../../../apps/common/lvgl_v8/examples/get_started/lv_example_get_started_3.c \
	../../../../apps/common/lvgl_v8/examples/layouts/flex/lv_example_flex_1.c \
	../../../../apps/common/lvgl_v8/examples/layouts/flex/lv_example_flex_2.c \
	../../../../apps/common/lvgl_v8/examples/layouts/flex/lv_example_flex_3.c \
	../../../../apps/common/lvgl_v8/examples/layouts/flex/lv_example_flex_4.c \
	../../../../apps/common/lvgl_v8/examples/layouts/flex/lv_example_flex_5.c \
	../../../../apps/common/lvgl_v8/examples/layouts/flex/lv_example_flex_6.c \
	../../../../apps/common/lvgl_v8/examples/layouts/grid/lv_example_grid_1.c \
	../../../../apps/common/lvgl_v8/examples/layouts/grid/lv_example_grid_2.c \
	../../../../apps/common/lvgl_v8/examples/layouts/grid/lv_example_grid_3.c \
	../../../../apps/common/lvgl_v8/examples/layouts/grid/lv_example_grid_4.c \
	../../../../apps/common/lvgl_v8/examples/layouts/grid/lv_example_grid_5.c \
	../../../../apps/common/lvgl_v8/examples/layouts/grid/lv_example_grid_6.c \
	../../../../apps/common/lvgl_v8/examples/libs/bmp/lv_example_bmp_1.c \
	../../../../apps/common/lvgl_v8/examples/libs/ffmpeg/lv_example_ffmpeg_1.c \
	../../../../apps/common/lvgl_v8/examples/libs/ffmpeg/lv_example_ffmpeg_2.c \
	../../../../apps/common/lvgl_v8/examples/libs/freetype/lv_example_freetype_1.c \
	../../../../apps/common/lvgl_v8/examples/libs/gif/img_bulb_gif.c \
	../../../../apps/common/lvgl_v8/examples/libs/gif/lv_example_gif_1.c \
	../../../../apps/common/lvgl_v8/examples/libs/png/img_wink_png.c \
	../../../../apps/common/lvgl_v8/examples/libs/png/lv_example_png_1.c \
	../../../../apps/common/lvgl_v8/examples/libs/qrcode/lv_example_qrcode_1.c \
	../../../../apps/common/lvgl_v8/examples/libs/rlottie/lv_example_rlottie_1.c \
	../../../../apps/common/lvgl_v8/examples/libs/rlottie/lv_example_rlottie_2.c \
	../../../../apps/common/lvgl_v8/examples/libs/rlottie/lv_example_rlottie_approve.c \
	../../../../apps/common/lvgl_v8/examples/libs/sjpg/lv_example_sjpg_1.c \
	../../../../apps/common/lvgl_v8/examples/libs/tiny_ttf/lv_example_tiny_ttf_1.c \
	../../../../apps/common/lvgl_v8/examples/libs/tiny_ttf/lv_example_tiny_ttf_2.c \
	../../../../apps/common/lvgl_v8/examples/libs/tiny_ttf/lv_example_tiny_ttf_3.c \
	../../../../apps/common/lvgl_v8/examples/libs/tiny_ttf/lv_example_tiny_ttf_4.c \
	../../../../apps/common/lvgl_v8/examples/libs/tiny_ttf/ubuntu_font.c \
	../../../../apps/common/lvgl_v8/examples/lv_example.c \
	../../../../apps/common/lvgl_v8/examples/others/fragment/lv_example_fragment_1.c \
	../../../../apps/common/lvgl_v8/examples/others/fragment/lv_example_fragment_2.c \
	../../../../apps/common/lvgl_v8/examples/others/gridnav/lv_example_gridnav_1.c \
	../../../../apps/common/lvgl_v8/examples/others/gridnav/lv_example_gridnav_2.c \
	../../../../apps/common/lvgl_v8/examples/others/gridnav/lv_example_gridnav_3.c \
	../../../../apps/common/lvgl_v8/examples/others/gridnav/lv_example_gridnav_4.c \
	../../../../apps/common/lvgl_v8/examples/others/ime/lv_example_ime_pinyin_1.c \
	../../../../apps/common/lvgl_v8/examples/others/ime/lv_example_ime_pinyin_2.c \
	../../../../apps/common/lvgl_v8/examples/others/imgfont/lv_example_imgfont_1.c \
	../../../../apps/common/lvgl_v8/examples/others/monkey/lv_example_monkey_1.c \
	../../../../apps/common/lvgl_v8/examples/others/monkey/lv_example_monkey_2.c \
	../../../../apps/common/lvgl_v8/examples/others/monkey/lv_example_monkey_3.c \
	../../../../apps/common/lvgl_v8/examples/others/msg/lv_example_msg_1.c \
	../../../../apps/common/lvgl_v8/examples/others/msg/lv_example_msg_2.c \
	../../../../apps/common/lvgl_v8/examples/others/msg/lv_example_msg_3.c \
	../../../../apps/common/lvgl_v8/examples/others/snapshot/lv_example_snapshot_1.c \
	../../../../apps/common/lvgl_v8/examples/porting/lv_jl_profiler.c \
	../../../../apps/common/lvgl_v8/examples/porting/lv_port_disp.c \
	../../../../apps/common/lvgl_v8/examples/porting/lv_port_fs.c \
	../../../../apps/common/lvgl_v8/examples/porting/lv_port_indev.c \
	../../../../apps/common/lvgl_v8/examples/scroll/lv_example_scroll_1.c \
	../../../../apps/common/lvgl_v8/examples/scroll/lv_example_scroll_2.c \
	../../../../apps/common/lvgl_v8/examples/scroll/lv_example_scroll_3.c \
	../../../../apps/common/lvgl_v8/examples/scroll/lv_example_scroll_4.c \
	../../../../apps/common/lvgl_v8/examples/scroll/lv_example_scroll_5.c \
	../../../../apps/common/lvgl_v8/examples/scroll/lv_example_scroll_6.c \
	../../../../apps/common/lvgl_v8/examples/styles/lv_example_style_1.c \
	../../../../apps/common/lvgl_v8/examples/styles/lv_example_style_10.c \
	../../../../apps/common/lvgl_v8/examples/styles/lv_example_style_11.c \
	../../../../apps/common/lvgl_v8/examples/styles/lv_example_style_12.c \
	../../../../apps/common/lvgl_v8/examples/styles/lv_example_style_13.c \
	../../../../apps/common/lvgl_v8/examples/styles/lv_example_style_14.c \
	../../../../apps/common/lvgl_v8/examples/styles/lv_example_style_15.c \
	../../../../apps/common/lvgl_v8/examples/styles/lv_example_style_2.c \
	../../../../apps/common/lvgl_v8/examples/styles/lv_example_style_3.c \
	../../../../apps/common/lvgl_v8/examples/styles/lv_example_style_4.c \
	../../../../apps/common/lvgl_v8/examples/styles/lv_example_style_5.c \
	../../../../apps/common/lvgl_v8/examples/styles/lv_example_style_6.c \
	../../../../apps/common/lvgl_v8/examples/styles/lv_example_style_7.c \
	../../../../apps/common/lvgl_v8/examples/styles/lv_example_style_8.c \
	../../../../apps/common/lvgl_v8/examples/styles/lv_example_style_9.c \
	../../../../apps/common/lvgl_v8/examples/widgets/album/lv_example_album_1.c \
	../../../../apps/common/lvgl_v8/examples/widgets/animimg/lv_example_animimg_1.c \
	../../../../apps/common/lvgl_v8/examples/widgets/arc/lv_example_arc_1.c \
	../../../../apps/common/lvgl_v8/examples/widgets/arc/lv_example_arc_2.c \
	../../../../apps/common/lvgl_v8/examples/widgets/bar/lv_example_bar_1.c \
	../../../../apps/common/lvgl_v8/examples/widgets/bar/lv_example_bar_2.c \
	../../../../apps/common/lvgl_v8/examples/widgets/bar/lv_example_bar_3.c \
	../../../../apps/common/lvgl_v8/examples/widgets/bar/lv_example_bar_4.c \
	../../../../apps/common/lvgl_v8/examples/widgets/bar/lv_example_bar_5.c \
	../../../../apps/common/lvgl_v8/examples/widgets/bar/lv_example_bar_6.c \
	../../../../apps/common/lvgl_v8/examples/widgets/btn/lv_example_btn_1.c \
	../../../../apps/common/lvgl_v8/examples/widgets/btn/lv_example_btn_2.c \
	../../../../apps/common/lvgl_v8/examples/widgets/btn/lv_example_btn_3.c \
	../../../../apps/common/lvgl_v8/examples/widgets/btnmatrix/lv_example_btnmatrix_1.c \
	../../../../apps/common/lvgl_v8/examples/widgets/btnmatrix/lv_example_btnmatrix_2.c \
	../../../../apps/common/lvgl_v8/examples/widgets/btnmatrix/lv_example_btnmatrix_3.c \
	../../../../apps/common/lvgl_v8/examples/widgets/calendar/lv_example_calendar_1.c \
	../../../../apps/common/lvgl_v8/examples/widgets/canvas/lv_example_canvas_1.c \
	../../../../apps/common/lvgl_v8/examples/widgets/canvas/lv_example_canvas_2.c \
	../../../../apps/common/lvgl_v8/examples/widgets/chart/lv_example_chart_1.c \
	../../../../apps/common/lvgl_v8/examples/widgets/chart/lv_example_chart_2.c \
	../../../../apps/common/lvgl_v8/examples/widgets/chart/lv_example_chart_3.c \
	../../../../apps/common/lvgl_v8/examples/widgets/chart/lv_example_chart_4.c \
	../../../../apps/common/lvgl_v8/examples/widgets/chart/lv_example_chart_5.c \
	../../../../apps/common/lvgl_v8/examples/widgets/chart/lv_example_chart_6.c \
	../../../../apps/common/lvgl_v8/examples/widgets/chart/lv_example_chart_7.c \
	../../../../apps/common/lvgl_v8/examples/widgets/chart/lv_example_chart_8.c \
	../../../../apps/common/lvgl_v8/examples/widgets/chart/lv_example_chart_9.c \
	../../../../apps/common/lvgl_v8/examples/widgets/checkbox/lv_example_checkbox_1.c \
	../../../../apps/common/lvgl_v8/examples/widgets/checkbox/lv_example_checkbox_2.c \
	../../../../apps/common/lvgl_v8/examples/widgets/colorwheel/lv_example_colorwheel_1.c \
	../../../../apps/common/lvgl_v8/examples/widgets/dropdown/lv_example_dropdown_1.c \
	../../../../apps/common/lvgl_v8/examples/widgets/dropdown/lv_example_dropdown_2.c \
	../../../../apps/common/lvgl_v8/examples/widgets/dropdown/lv_example_dropdown_3.c \
	../../../../apps/common/lvgl_v8/examples/widgets/img/lv_example_img_1.c \
	../../../../apps/common/lvgl_v8/examples/widgets/img/lv_example_img_2.c \
	../../../../apps/common/lvgl_v8/examples/widgets/img/lv_example_img_3.c \
	../../../../apps/common/lvgl_v8/examples/widgets/img/lv_example_img_4.c \
	../../../../apps/common/lvgl_v8/examples/widgets/imgbtn/lv_example_imgbtn_1.c \
	../../../../apps/common/lvgl_v8/examples/widgets/keyboard/lv_example_keyboard_1.c \
	../../../../apps/common/lvgl_v8/examples/widgets/label/lv_example_label_1.c \
	../../../../apps/common/lvgl_v8/examples/widgets/label/lv_example_label_2.c \
	../../../../apps/common/lvgl_v8/examples/widgets/label/lv_example_label_3.c \
	../../../../apps/common/lvgl_v8/examples/widgets/label/lv_example_label_4.c \
	../../../../apps/common/lvgl_v8/examples/widgets/label/lv_example_label_5.c \
	../../../../apps/common/lvgl_v8/examples/widgets/led/lv_example_led_1.c \
	../../../../apps/common/lvgl_v8/examples/widgets/line/lv_example_line_1.c \
	../../../../apps/common/lvgl_v8/examples/widgets/list/lv_example_list_1.c \
	../../../../apps/common/lvgl_v8/examples/widgets/list/lv_example_list_2.c \
	../../../../apps/common/lvgl_v8/examples/widgets/lyrics/lv_example_lyrics_1.c \
	../../../../apps/common/lvgl_v8/examples/widgets/lyrics/lv_example_lyrics_2.c \
	../../../../apps/common/lvgl_v8/examples/widgets/lyrics/lv_example_lyrics_3.c \
	../../../../apps/common/lvgl_v8/examples/widgets/lyrics/lv_example_lyrics_4.c \
	../../../../apps/common/lvgl_v8/examples/widgets/lyrics/lv_example_lyrics_5.c \
	../../../../apps/common/lvgl_v8/examples/widgets/menu/lv_example_menu_1.c \
	../../../../apps/common/lvgl_v8/examples/widgets/menu/lv_example_menu_2.c \
	../../../../apps/common/lvgl_v8/examples/widgets/menu/lv_example_menu_3.c \
	../../../../apps/common/lvgl_v8/examples/widgets/menu/lv_example_menu_4.c \
	../../../../apps/common/lvgl_v8/examples/widgets/menu/lv_example_menu_5.c \
	../../../../apps/common/lvgl_v8/examples/widgets/meter/lv_example_meter_1.c \
	../../../../apps/common/lvgl_v8/examples/widgets/meter/lv_example_meter_2.c \
	../../../../apps/common/lvgl_v8/examples/widgets/meter/lv_example_meter_3.c \
	../../../../apps/common/lvgl_v8/examples/widgets/meter/lv_example_meter_4.c \
	../../../../apps/common/lvgl_v8/examples/widgets/msgbox/lv_example_msgbox_1.c \
	../../../../apps/common/lvgl_v8/examples/widgets/obj/lv_example_obj_1.c \
	../../../../apps/common/lvgl_v8/examples/widgets/obj/lv_example_obj_2.c \
	../../../../apps/common/lvgl_v8/examples/widgets/obj2p5d/lv_example_font2p5d_1.c \
	../../../../apps/common/lvgl_v8/examples/widgets/obj2p5d/lv_example_img2p5d_1.c \
	../../../../apps/common/lvgl_v8/examples/widgets/roller/lv_example_roller_1.c \
	../../../../apps/common/lvgl_v8/examples/widgets/roller/lv_example_roller_2.c \
	../../../../apps/common/lvgl_v8/examples/widgets/roller/lv_example_roller_3.c \
	../../../../apps/common/lvgl_v8/examples/widgets/slider/lv_example_slider_1.c \
	../../../../apps/common/lvgl_v8/examples/widgets/slider/lv_example_slider_2.c \
	../../../../apps/common/lvgl_v8/examples/widgets/slider/lv_example_slider_3.c \
	../../../../apps/common/lvgl_v8/examples/widgets/span/lv_example_span_1.c \
	../../../../apps/common/lvgl_v8/examples/widgets/spinbox/lv_example_spinbox_1.c \
	../../../../apps/common/lvgl_v8/examples/widgets/spinner/lv_example_spinner_1.c \
	../../../../apps/common/lvgl_v8/examples/widgets/switch/lv_example_switch_1.c \
	../../../../apps/common/lvgl_v8/examples/widgets/table/lv_example_table_1.c \
	../../../../apps/common/lvgl_v8/examples/widgets/table/lv_example_table_2.c \
	../../../../apps/common/lvgl_v8/examples/widgets/tabview/lv_example_tabview_1.c \
	../../../../apps/common/lvgl_v8/examples/widgets/tabview/lv_example_tabview_2.c \
	../../../../apps/common/lvgl_v8/examples/widgets/textarea/lv_example_textarea_1.c \
	../../../../apps/common/lvgl_v8/examples/widgets/textarea/lv_example_textarea_2.c \
	../../../../apps/common/lvgl_v8/examples/widgets/textarea/lv_example_textarea_3.c \
	../../../../apps/common/lvgl_v8/examples/widgets/tileview/lv_example_tileview_1.c \
	../../../../apps/common/lvgl_v8/examples/widgets/win/lv_example_win_1.c \
	../../../../apps/common/lvgl_v8/lvgl_main.c \
	../../../../apps/common/lvgl_v8/src/core/lv_disp.c \
	../../../../apps/common/lvgl_v8/src/core/lv_event.c \
	../../../../apps/common/lvgl_v8/src/core/lv_group.c \
	../../../../apps/common/lvgl_v8/src/core/lv_indev.c \
	../../../../apps/common/lvgl_v8/src/core/lv_indev_scroll.c \
	../../../../apps/common/lvgl_v8/src/core/lv_obj.c \
	../../../../apps/common/lvgl_v8/src/core/lv_obj_class.c \
	../../../../apps/common/lvgl_v8/src/core/lv_obj_draw.c \
	../../../../apps/common/lvgl_v8/src/core/lv_obj_pos.c \
	../../../../apps/common/lvgl_v8/src/core/lv_obj_scroll.c \
	../../../../apps/common/lvgl_v8/src/core/lv_obj_style.c \
	../../../../apps/common/lvgl_v8/src/core/lv_obj_style_gen.c \
	../../../../apps/common/lvgl_v8/src/core/lv_obj_tree.c \
	../../../../apps/common/lvgl_v8/src/core/lv_refr.c \
	../../../../apps/common/lvgl_v8/src/core/lv_theme.c \
	../../../../apps/common/lvgl_v8/src/draw/jl/jl_dma2d/lv_gpu_jl_dma2d.c \
	../../../../apps/common/lvgl_v8/src/draw/jl/jl_gpu2p5d/lv_gpu_jl_gpu2p5d.c \
	../../../../apps/common/lvgl_v8/src/draw/jl/jl_gpu2p5d/lv_gpu_jl_gpu2p5d_arc.c \
	../../../../apps/common/lvgl_v8/src/draw/jl/jl_gpu2p5d/lv_gpu_jl_gpu2p5d_comm.c \
	../../../../apps/common/lvgl_v8/src/draw/jl/jl_gpu2p5d/lv_gpu_jl_gpu2p5d_img.c \
	../../../../apps/common/lvgl_v8/src/draw/jl/jl_gpu2p5d/lv_gpu_jl_gpu2p5d_letter.c \
	../../../../apps/common/lvgl_v8/src/draw/jl/jl_gpu2p5d/lv_gpu_jl_gpu2p5d_line.c \
	../../../../apps/common/lvgl_v8/src/draw/jl/jl_gpu2p5d/lv_gpu_jl_gpu2p5d_polygon.c \
	../../../../apps/common/lvgl_v8/src/draw/jl/jl_gpu2p5d/lv_gpu_jl_gpu2p5d_rect.c \
	../../../../apps/common/lvgl_v8/src/draw/jl/lv_gpu_jl.c \
	../../../../apps/common/lvgl_v8/src/draw/lv_draw.c \
	../../../../apps/common/lvgl_v8/src/draw/lv_draw_arc.c \
	../../../../apps/common/lvgl_v8/src/draw/lv_draw_img.c \
	../../../../apps/common/lvgl_v8/src/draw/lv_draw_label.c \
	../../../../apps/common/lvgl_v8/src/draw/lv_draw_layer.c \
	../../../../apps/common/lvgl_v8/src/draw/lv_draw_line.c \
	../../../../apps/common/lvgl_v8/src/draw/lv_draw_mask.c \
	../../../../apps/common/lvgl_v8/src/draw/lv_draw_rect.c \
	../../../../apps/common/lvgl_v8/src/draw/lv_draw_transform.c \
	../../../../apps/common/lvgl_v8/src/draw/lv_draw_triangle.c \
	../../../../apps/common/lvgl_v8/src/draw/lv_img_buf.c \
	../../../../apps/common/lvgl_v8/src/draw/lv_img_cache.c \
	../../../../apps/common/lvgl_v8/src/draw/lv_img_decoder.c \
	../../../../apps/common/lvgl_v8/src/draw/sw/lv_draw_sw.c \
	../../../../apps/common/lvgl_v8/src/draw/sw/lv_draw_sw_arc.c \
	../../../../apps/common/lvgl_v8/src/draw/sw/lv_draw_sw_blend.c \
	../../../../apps/common/lvgl_v8/src/draw/sw/lv_draw_sw_dither.c \
	../../../../apps/common/lvgl_v8/src/draw/sw/lv_draw_sw_gradient.c \
	../../../../apps/common/lvgl_v8/src/draw/sw/lv_draw_sw_img.c \
	../../../../apps/common/lvgl_v8/src/draw/sw/lv_draw_sw_layer.c \
	../../../../apps/common/lvgl_v8/src/draw/sw/lv_draw_sw_letter.c \
	../../../../apps/common/lvgl_v8/src/draw/sw/lv_draw_sw_line.c \
	../../../../apps/common/lvgl_v8/src/draw/sw/lv_draw_sw_polygon.c \
	../../../../apps/common/lvgl_v8/src/draw/sw/lv_draw_sw_rect.c \
	../../../../apps/common/lvgl_v8/src/draw/sw/lv_draw_sw_transform.c \
	../../../../apps/common/lvgl_v8/src/extra/jl_extra/lv_jl_extra.c \
	../../../../apps/common/lvgl_v8/src/extra/layouts/flex/lv_flex.c \
	../../../../apps/common/lvgl_v8/src/extra/layouts/grid/lv_grid.c \
	../../../../apps/common/lvgl_v8/src/extra/libs/bmp/lv_bmp.c \
	../../../../apps/common/lvgl_v8/src/extra/libs/ffmpeg/lv_ffmpeg.c \
	../../../../apps/common/lvgl_v8/src/extra/libs/freetype/lv_freetype.c \
	../../../../apps/common/lvgl_v8/src/extra/libs/fsdrv/lv_fs_fatfs.c \
	../../../../apps/common/lvgl_v8/src/extra/libs/fsdrv/lv_fs_littlefs.c \
	../../../../apps/common/lvgl_v8/src/extra/libs/fsdrv/lv_fs_posix.c \
	../../../../apps/common/lvgl_v8/src/extra/libs/fsdrv/lv_fs_stdio.c \
	../../../../apps/common/lvgl_v8/src/extra/libs/fsdrv/lv_fs_win32.c \
	../../../../apps/common/lvgl_v8/src/extra/libs/gif/gifdec.c \
	../../../../apps/common/lvgl_v8/src/extra/libs/gif/lv_gif.c \
	../../../../apps/common/lvgl_v8/src/extra/libs/png/lodepng.c \
	../../../../apps/common/lvgl_v8/src/extra/libs/png/lv_png.c \
	../../../../apps/common/lvgl_v8/src/extra/libs/qrcode/lv_qrcode.c \
	../../../../apps/common/lvgl_v8/src/extra/libs/qrcode/qrcodegen.c \
	../../../../apps/common/lvgl_v8/src/extra/libs/rlottie/lv_rlottie.c \
	../../../../apps/common/lvgl_v8/src/extra/libs/rlottie/lv_rlottie_example_fingerprint.c \
	../../../../apps/common/lvgl_v8/src/extra/libs/sjpg/lv_sjpg.c \
	../../../../apps/common/lvgl_v8/src/extra/libs/sjpg/tjpgd.c \
	../../../../apps/common/lvgl_v8/src/extra/libs/tiny_ttf/lv_tiny_ttf.c \
	../../../../apps/common/lvgl_v8/src/extra/lv_extra.c \
	../../../../apps/common/lvgl_v8/src/extra/others/fragment/lv_fragment.c \
	../../../../apps/common/lvgl_v8/src/extra/others/fragment/lv_fragment_manager.c \
	../../../../apps/common/lvgl_v8/src/extra/others/gridnav/lv_gridnav.c \
	../../../../apps/common/lvgl_v8/src/extra/others/imgfont/lv_imgfont.c \
	../../../../apps/common/lvgl_v8/src/extra/others/monkey/lv_monkey.c \
	../../../../apps/common/lvgl_v8/src/extra/others/msg/lv_msg.c \
	../../../../apps/common/lvgl_v8/src/extra/others/observer/lv_observer.c \
	../../../../apps/common/lvgl_v8/src/extra/others/snapshot/lv_snapshot.c \
	../../../../apps/common/lvgl_v8/src/extra/themes/basic/lv_theme_basic.c \
	../../../../apps/common/lvgl_v8/src/extra/themes/default/lv_theme_default.c \
	../../../../apps/common/lvgl_v8/src/extra/themes/mono/lv_theme_mono.c \
	../../../../apps/common/lvgl_v8/src/extra/widgets/animimg/lv_animimg.c \
	../../../../apps/common/lvgl_v8/src/extra/widgets/calendar/lv_calendar.c \
	../../../../apps/common/lvgl_v8/src/extra/widgets/calendar/lv_calendar_header_arrow.c \
	../../../../apps/common/lvgl_v8/src/extra/widgets/calendar/lv_calendar_header_dropdown.c \
	../../../../apps/common/lvgl_v8/src/extra/widgets/chart/lv_chart.c \
	../../../../apps/common/lvgl_v8/src/extra/widgets/colorwheel/lv_colorwheel.c \
	../../../../apps/common/lvgl_v8/src/extra/widgets/imgbtn/lv_imgbtn.c \
	../../../../apps/common/lvgl_v8/src/extra/widgets/led/lv_led.c \
	../../../../apps/common/lvgl_v8/src/extra/widgets/list/lv_list.c \
	../../../../apps/common/lvgl_v8/src/extra/widgets/menu/lv_menu.c \
	../../../../apps/common/lvgl_v8/src/extra/widgets/meter/lv_meter.c \
	../../../../apps/common/lvgl_v8/src/extra/widgets/msgbox/lv_msgbox.c \
	../../../../apps/common/lvgl_v8/src/extra/widgets/span/lv_span.c \
	../../../../apps/common/lvgl_v8/src/extra/widgets/spinbox/lv_spinbox.c \
	../../../../apps/common/lvgl_v8/src/extra/widgets/spinner/lv_spinner.c \
	../../../../apps/common/lvgl_v8/src/extra/widgets/tabview/lv_tabview.c \
	../../../../apps/common/lvgl_v8/src/extra/widgets/tileview/lv_tileview.c \
	../../../../apps/common/lvgl_v8/src/extra/widgets/win/lv_win.c \
	../../../../apps/common/lvgl_v8/src/font/lv_font.c \
	../../../../apps/common/lvgl_v8/src/font/lv_font_dejavu_16_persian_hebrew.c \
	../../../../apps/common/lvgl_v8/src/font/lv_font_fmt_txt.c \
	../../../../apps/common/lvgl_v8/src/font/lv_font_loader.c \
	../../../../apps/common/lvgl_v8/src/font/lv_font_montserrat_10.c \
	../../../../apps/common/lvgl_v8/src/font/lv_font_montserrat_12.c \
	../../../../apps/common/lvgl_v8/src/font/lv_font_montserrat_12_subpx.c \
	../../../../apps/common/lvgl_v8/src/font/lv_font_montserrat_14.c \
	../../../../apps/common/lvgl_v8/src/font/lv_font_montserrat_16.c \
	../../../../apps/common/lvgl_v8/src/font/lv_font_montserrat_18.c \
	../../../../apps/common/lvgl_v8/src/font/lv_font_montserrat_20.c \
	../../../../apps/common/lvgl_v8/src/font/lv_font_montserrat_22.c \
	../../../../apps/common/lvgl_v8/src/font/lv_font_montserrat_24.c \
	../../../../apps/common/lvgl_v8/src/font/lv_font_montserrat_26.c \
	../../../../apps/common/lvgl_v8/src/font/lv_font_montserrat_28.c \
	../../../../apps/common/lvgl_v8/src/font/lv_font_montserrat_28_compressed.c \
	../../../../apps/common/lvgl_v8/src/font/lv_font_montserrat_30.c \
	../../../../apps/common/lvgl_v8/src/font/lv_font_montserrat_32.c \
	../../../../apps/common/lvgl_v8/src/font/lv_font_montserrat_34.c \
	../../../../apps/common/lvgl_v8/src/font/lv_font_montserrat_36.c \
	../../../../apps/common/lvgl_v8/src/font/lv_font_montserrat_38.c \
	../../../../apps/common/lvgl_v8/src/font/lv_font_montserrat_40.c \
	../../../../apps/common/lvgl_v8/src/font/lv_font_montserrat_42.c \
	../../../../apps/common/lvgl_v8/src/font/lv_font_montserrat_44.c \
	../../../../apps/common/lvgl_v8/src/font/lv_font_montserrat_46.c \
	../../../../apps/common/lvgl_v8/src/font/lv_font_montserrat_48.c \
	../../../../apps/common/lvgl_v8/src/font/lv_font_montserrat_8.c \
	../../../../apps/common/lvgl_v8/src/font/lv_font_simsun_16_cjk.c \
	../../../../apps/common/lvgl_v8/src/font/lv_font_unscii_16.c \
	../../../../apps/common/lvgl_v8/src/font/lv_font_unscii_8.c \
	../../../../apps/common/lvgl_v8/src/hal/lv_hal_disp.c \
	../../../../apps/common/lvgl_v8/src/hal/lv_hal_indev.c \
	../../../../apps/common/lvgl_v8/src/hal/lv_hal_tick.c \
	../../../../apps/common/lvgl_v8/src/misc/lv_anim.c \
	../../../../apps/common/lvgl_v8/src/misc/lv_anim_timeline.c \
	../../../../apps/common/lvgl_v8/src/misc/lv_area.c \
	../../../../apps/common/lvgl_v8/src/misc/lv_async.c \
	../../../../apps/common/lvgl_v8/src/misc/lv_bidi.c \
	../../../../apps/common/lvgl_v8/src/misc/lv_color.c \
	../../../../apps/common/lvgl_v8/src/misc/lv_fs.c \
	../../../../apps/common/lvgl_v8/src/misc/lv_gc.c \
	../../../../apps/common/lvgl_v8/src/misc/lv_ll.c \
	../../../../apps/common/lvgl_v8/src/misc/lv_log.c \
	../../../../apps/common/lvgl_v8/src/misc/lv_lru.c \
	../../../../apps/common/lvgl_v8/src/misc/lv_math.c \
	../../../../apps/common/lvgl_v8/src/misc/lv_mem.c \
	../../../../apps/common/lvgl_v8/src/misc/lv_printf.c \
	../../../../apps/common/lvgl_v8/src/misc/lv_profiler_builtin.c \
	../../../../apps/common/lvgl_v8/src/misc/lv_style.c \
	../../../../apps/common/lvgl_v8/src/misc/lv_style_gen.c \
	../../../../apps/common/lvgl_v8/src/misc/lv_templ.c \
	../../../../apps/common/lvgl_v8/src/misc/lv_timer.c \
	../../../../apps/common/lvgl_v8/src/misc/lv_tlsf.c \
	../../../../apps/common/lvgl_v8/src/misc/lv_txt.c \
	../../../../apps/common/lvgl_v8/src/misc/lv_txt_ap.c \
	../../../../apps/common/lvgl_v8/src/misc/lv_utils.c \
	../../../../apps/common/lvgl_v8/src/widgets/lv_arc.c \
	../../../../apps/common/lvgl_v8/src/widgets/lv_bar.c \
	../../../../apps/common/lvgl_v8/src/widgets/lv_btn.c \
	../../../../apps/common/lvgl_v8/src/widgets/lv_btnmatrix.c \
	../../../../apps/common/lvgl_v8/src/widgets/lv_canvas.c \
	../../../../apps/common/lvgl_v8/src/widgets/lv_checkbox.c \
	../../../../apps/common/lvgl_v8/src/widgets/lv_dropdown.c \
	../../../../apps/common/lvgl_v8/src/widgets/lv_img.c \
	../../../../apps/common/lvgl_v8/src/widgets/lv_keyboard.c \
	../../../../apps/common/lvgl_v8/src/widgets/lv_label.c \
	../../../../apps/common/lvgl_v8/src/widgets/lv_line.c \
	../../../../apps/common/lvgl_v8/src/widgets/lv_objx_templ.c \
	../../../../apps/common/lvgl_v8/src/widgets/lv_roller.c \
	../../../../apps/common/lvgl_v8/src/widgets/lv_slider.c \
	../../../../apps/common/lvgl_v8/src/widgets/lv_switch.c \
	../../../../apps/common/lvgl_v8/src/widgets/lv_table.c \
	../../../../apps/common/lvgl_v8/src/widgets/lv_textarea.c \
	../../../../apps/common/net/assign_macaddr.c \
	../../../../apps/common/net/config_network.c \
	../../../../apps/common/net/iperf_test.c \
	../../../../apps/common/net/platform_cfg.c \
	../../../../apps/common/net/testbox/client.c \
	../../../../apps/common/net/testbox/fsm.c \
	../../../../apps/common/net/testbox/wbcp.c \
	../../../../apps/common/net/voiceprint_cfg.c \
	../../../../apps/common/net/wifi_conf.c \
	../../../../apps/common/product_test_tool/communication.c \
	../../../../apps/common/product_test_tool/devices_control.c \
	../../../../apps/common/product_test_tool/product_main.c \
	../../../../apps/common/product_test_tool/product_net.c \
	../../../../apps/common/product_test_tool/product_port.c \
	../../../../apps/common/rf_fcc_tool/bt.c \
	../../../../apps/common/rf_fcc_tool/rf_fcc_main.c \
	../../../../apps/common/rf_fcc_tool/wifi_adaptivity.c \
	../../../../apps/common/screen_mirror/screen_mirror_adapter.c \
	../../../../apps/common/screen_mirror/screen_mirror_api.c \
	../../../../apps/common/system/device_mount.c \
	../../../../apps/common/system/flash_write_protect.c \
	../../../../apps/common/system/init.c \
	../../../../apps/common/system/init_expand.c \
	../../../../apps/common/system/mem_mmu.c \
	../../../../apps/common/system/perf_counter/example/main.c \
	../../../../apps/common/system/perf_counter/perf_counter.c \
	../../../../apps/common/system/power_manage.c \
	../../../../apps/common/system/version.c \
	../../../../apps/common/third_party_profile/alipay/le_alipay.c \
	../../../../apps/common/third_party_profile/ancs_client_demo/le_ancs_client.c \
	../../../../apps/common/third_party_profile/custom_protocol_demo/custom_protocol.c \
	../../../../apps/common/third_party_profile/jieli/hid_user.c \
	../../../../apps/common/third_party_profile/le_hogp/le_hogp.c \
	../../../../apps/common/third_party_profile/mijia/mijia_api.c \
	../../../../apps/common/third_party_profile/mijia/mijia_ble_api/mible_api.c \
	../../../../apps/common/third_party_profile/mijia/mijia_ble_api/mible_mcu.c \
	../../../../apps/common/third_party_profile/mijia/mijia_ble_libs/mi_config.c \
	../../../../apps/common/third_party_profile/mijia/mijia_ble_libs/third_party/SEGGER_RTT/SEGGER_RTT.c \
	../../../../apps/common/third_party_profile/mijia/mijia_ble_libs/third_party/SEGGER_RTT/SEGGER_RTT_printf.c \
	../../../../apps/common/third_party_profile/mijia/mijia_ble_libs/third_party/mbedtls/ccm.c \
	../../../../apps/common/third_party_profile/mijia/mijia_ble_libs/third_party/mbedtls/sha256_hkdf.c \
	../../../../apps/common/third_party_profile/mijia/mijia_ble_libs/third_party/micro-ecc/uECC.c \
	../../../../apps/common/third_party_profile/mijia/mijia_ble_libs/third_party/pt/pt_misc.c \
	../../../../apps/common/third_party_profile/mijia/mijia_main.c \
	../../../../apps/common/third_party_profile/multi_box_adv/multi_box_adv.c \
	../../../../apps/common/third_party_profile/multi_box_adv/multi_box_role.c \
	../../../../apps/common/third_party_profile/multi_box_adv/multi_box_scan.c \
	../../../../apps/common/third_party_profile/multi_protocol_common.c \
	../../../../apps/common/third_party_profile/multi_protocol_event.c \
	../../../../apps/common/third_party_profile/multi_protocol_main.c \
	../../../../apps/common/third_party_profile/net_cfg/le_net_cfg.c \
	../../../../apps/common/update/fs_update.c \
	../../../../apps/common/update/http_update.c \
	../../../../apps/common/update/net_update.c \
	../../../../apps/common/update/update.c \
	../../../../apps/common/usb/device/cdc.c \
	../../../../apps/common/usb/device/custom_hid.c \
	../../../../apps/common/usb/device/descriptor.c \
	../../../../apps/common/usb/device/hid.c \
	../../../../apps/common/usb/device/hid_keyboard.c \
	../../../../apps/common/usb/device/hid_pos.c \
	../../../../apps/common/usb/device/midi.c \
	../../../../apps/common/usb/device/msd.c \
	../../../../apps/common/usb/device/msd_cdrom.c \
	../../../../apps/common/usb/device/msd_upgrade.c \
	../../../../apps/common/usb/device/printer.c \
	../../../../apps/common/usb/device/ram_disk_circle.c \
	../../../../apps/common/usb/device/ram_disk_update.c \
	../../../../apps/common/usb/device/rndis_connectionless.c \
	../../../../apps/common/usb/device/task_pc.c \
	../../../../apps/common/usb/device/uac.c \
	../../../../apps/common/usb/device/uac1.c \
	../../../../apps/common/usb/device/uac2.c \
	../../../../apps/common/usb/device/uac_stream.c \
	../../../../apps/common/usb/device/usb_device.c \
	../../../../apps/common/usb/device/user_setup.c \
	../../../../apps/common/usb/device/uvc.c \
	../../../../apps/common/usb/host/adb.c \
	../../../../apps/common/usb/host/aoa.c \
	../../../../apps/common/usb/host/audio.c \
	../../../../apps/common/usb/host/hid.c \
	../../../../apps/common/usb/host/hub.c \
	../../../../apps/common/usb/host/rndis_host_connectionless.c \
	../../../../apps/common/usb/host/task_host_husb.c \
	../../../../apps/common/usb/host/usb_bulk_transfer.c \
	../../../../apps/common/usb/host/usb_ctrl_transfer.c \
	../../../../apps/common/usb/host/usb_host.c \
	../../../../apps/common/usb/host/usb_host_cdc.c \
	../../../../apps/common/usb/host/usb_storage.c \
	../../../../apps/common/usb/host/usb_video.c \
	../../../../apps/common/usb/host/usbnet.c \
	../../../../apps/common/usb/host/uvc_host.c \
	../../../../apps/common/usb/usb_config.c \
	../../../../apps/common/usb/usb_epbuf_manager.c \
	../../../../apps/common/usb/usb_host_config.c \
	../../../../apps/common/video/decoder_one_jpeg.c \
	../../../../apps/common/video/encoder_one_jpeg.c \
	../../../../apps/common/video/jpg2avi.c \
	../../../../apps/common/video/logo_show.c \
	../../../../apps/common/video/net_avi_player/avi_parse.c \
	../../../../apps/common/video/net_avi_player/avi_player.c \
	../../../../apps/common/video/net_avi_player/curl_get_data.c \
	../../../../apps/common/video/net_avi_player/net_avi_player.c \
	../../../../apps/common/video/net_avi_player/pipeline_jpeg_dec.c \
	../../../../apps/common/video/net_avi_player/play_demo.c \
	../../../../apps/common/video/quick_show_uvc.c \
	../../../../apps/common/video/sharpen_adapter.c \
	../../../../apps/common/video/simple_avi_unpkg.c \
	../../../../apps/common/video/user_video_rec.c \
	../../../../apps/common/video/video_dec_demo.c \
	../../../../apps/common/video/video_rec_demo.c \
	../../../../apps/wifi_camera/app_database.c \
	../../../../apps/wifi_camera/app_main.c \
	../../../../apps/wifi_camera/app_msg.c \
	../../../../apps/wifi_camera/audio/mic_effect.c \
	../../../../apps/wifi_camera/audio/scene_switch.c \
	../../../../apps/wifi_camera/audio/tone_table.c \
	../../../../apps/wifi_camera/audio/vol_sync.c \
	../../../../apps/wifi_camera/board/wl83/board_develop.c \
	../../../../apps/wifi_camera/board/wl83/sdk_config.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/custom/custom.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/custom/sim_database.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/custom/ui_action_animation.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/custom/ui_action_dec.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/custom/ui_action_linedrift.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/custom/ui_action_list.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/custom/ui_action_main.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/custom/ui_action_page_lyrics.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/custom/ui_action_page_map.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/custom/ui_action_page_music.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/custom/ui_action_parking.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/custom/ui_action_replay.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/custom/ui_action_sys_setting.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/custom/ui_action_usb.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/custom/ui_action_video_photo.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/custom/ui_action_video_rec.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/generated/gui_events/callback_handler.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/generated/gui_events/events_init.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/generated/gui_events/i18n_handler.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/generated/gui_events/screen_handler.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/generated/gui_fonts/gui_fonts.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/generated/gui_group/gui_group.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/generated/gui_guider.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/generated/gui_msg/gui_model_global_subpage_msg.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/generated/gui_msg/gui_model_main_msg.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/generated/gui_msg/gui_model_msg.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/generated/gui_msg/gui_model_sys_setting_msg.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/generated/gui_msg/gui_model_update_msg.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/generated/gui_msg/gui_model_video_dec_msg.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/generated/gui_msg/gui_model_video_photo_msg.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/generated/gui_msg/gui_model_video_rec_msg.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/generated/gui_msg/gui_msg.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/generated/gui_msg/gui_speed_num_msg.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/generated/gui_msg/gui_sys_model_msg.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/generated/gui_msg/gui_tyw_system_msg.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/generated/gui_res/res_common.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/generated/gui_scr/setup_scr_car_parking.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/generated/gui_scr/setup_scr_line_drift.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/generated/gui_scr/setup_scr_page_map.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/generated/gui_scr/setup_scr_page_meter.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/generated/gui_scr/setup_scr_page_music.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/generated/gui_scr/setup_scr_power_on.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/generated/gui_scr/setup_scr_sys_popwin.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/generated/gui_scr/setup_scr_sys_prompt.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/generated/gui_scr/setup_scr_sys_setting.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/generated/gui_scr/setup_scr_usb_slave.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/generated/gui_scr/setup_scr_video_dec.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/generated/gui_scr/setup_scr_video_dec_options.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/generated/gui_scr/setup_scr_video_dir.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/generated/gui_scr/setup_scr_video_file.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/generated/gui_scr/setup_scr_video_photo.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/generated/gui_scr/setup_scr_video_play.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/generated/gui_scr/setup_scr_video_rec.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/generated/gui_scr/ui_scr_manager.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/generated/gui_scr/ui_style.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/generated/gui_timelines/gui_timeline_timeline.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/generated/gui_timelines/gui_timelines.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/style_LY/generated/i18n/lv_i18n.c \
	../../../../apps/wifi_camera/lvgl_v8_ui_app/ui_action.c \
	../../../../apps/wifi_camera/misc/backlight.c \
	../../../../apps/wifi_camera/misc/bitstore.c \
	../../../../apps/wifi_camera/misc/key_voice.c \
	../../../../apps/wifi_camera/misc/parking_detect.c \
	../../../../apps/wifi_camera/misc/video_system.c \
	../../../../apps/wifi_camera/misc/video_system_cfg.c \
	../../../../apps/wifi_camera/mode/bt/bt.c \
	../../../../apps/wifi_camera/mode/bt/bt_slience_detect.c \
	../../../../apps/wifi_camera/mode/bt/dual_conn.c \
	../../../../apps/wifi_camera/mode/bt/le_audio/auracast/app_le_auracast.c \
	../../../../apps/wifi_camera/mode/bt/le_audio/big/app_le_broadcast.c \
	../../../../apps/wifi_camera/mode/bt/le_audio/big/le_broadcast.c \
	../../../../apps/wifi_camera/mode/bt/le_audio/big/le_broadcast_config.c \
	../../../../apps/wifi_camera/mode/bt/le_audio/le_audio_buffer.c \
	../../../../apps/wifi_camera/mode/bt/le_audio/le_audio_common.c \
	../../../../apps/wifi_camera/mode/bt/le_audio/le_audio_custom_data_api.c \
	../../../../apps/wifi_camera/mode/bt/sniff.c \
	../../../../apps/wifi_camera/mode/bt/tone.c \
	../../../../apps/wifi_camera/mode/bt/tws_dual_conn.c \
	../../../../apps/wifi_camera/mode/linein_music/linein_dev.c \
	../../../../apps/wifi_camera/mode/linein_music/linein_music.c \
	../../../../apps/wifi_camera/mode/local_music/local_music.c \
	../../../../apps/wifi_camera/mode/local_tws/local_tws.c \
	../../../../apps/wifi_camera/mode/net_music/net_app_cfg.c \
	../../../../apps/wifi_camera/mode/net_music/net_music.c \
	../../../../apps/wifi_camera/mode/net_music/wifi_app_task.c \
	../../../../apps/wifi_camera/mode/pc/pc.c \
	../../../../apps/wifi_camera/mode/recorder/recorder.c \
	../../../../apps/wifi_camera/mode/video_dec/video_dec.c \
	../../../../apps/wifi_camera/mode/video_dec/video_dec_cfg.c \
	../../../../apps/wifi_camera/mode/video_photo/video_photo.c \
	../../../../apps/wifi_camera/mode/video_rec/video_rec.c \
	../../../../apps/wifi_camera/mode/video_rec/video_rec_cfg.c \
	../../../../apps/wifi_camera/wifi/ctp_cmd.c \
	../../../../apps/wifi_camera/wifi/ext_wifi_app_task.c \
	../../../../apps/wifi_camera/wifi/ftpd_vfs_reg.c \
	../../../../apps/wifi_camera/wifi/media_file_list.c \
	../../../../apps/wifi_camera/wifi/net_video_rec.c \
	../../../../apps/wifi_camera/wifi/network_mssdp.c \
	../../../../apps/wifi_camera/wifi/network_update_resume.c \
	../../../../apps/wifi_camera/wifi/packet.c \
	../../../../apps/wifi_camera/wifi/stream/video_rt_tcp.c \
	../../../../apps/wifi_camera/wifi/stream/video_rt_udp.c \
	../../../../apps/wifi_camera/wifi/stream/video_rt_usr.c \
	../../../../apps/wifi_camera/wifi/strm_video_rec.c \
	../../../../apps/wifi_camera/wifi/video_playback.c \
	../../../../apps/wifi_camera/wifi/video_preview.c \
	../../../../apps/wifi_camera/wifi/wifi_app_task.c \
	../../../../audio/CVP/audio_cvp.c \
	../../../../audio/CVP/audio_cvp_3mic.c \
	../../../../audio/CVP/audio_cvp_ais_3mic.c \
	../../../../audio/CVP/audio_cvp_config.c \
	../../../../audio/CVP/audio_cvp_develop.c \
	../../../../audio/CVP/audio_cvp_dms.c \
	../../../../audio/CVP/audio_cvp_online.c \
	../../../../audio/CVP/audio_cvp_ref_task.c \
	../../../../audio/CVP/audio_cvp_sync.c \
	../../../../audio/common/amplitude_statistic.c \
	../../../../audio/common/audio_build_needed.c \
	../../../../audio/common/audio_dvol.c \
	../../../../audio/common/audio_event_handler.c \
	../../../../audio/common/audio_export_demo.c \
	../../../../audio/common/audio_general.c \
	../../../../audio/common/audio_node_config.c \
	../../../../audio/common/audio_noise_gate.c \
	../../../../audio/common/audio_ns.c \
	../../../../audio/common/audio_plc.c \
	../../../../audio/common/audio_utils.c \
	../../../../audio/common/audio_volume_mixer.c \
	../../../../audio/common/bt_audio_energy_detection.c \
	../../../../audio/common/common_func.c \
	../../../../audio/common/config/new_cfg_tool.c \
	../../../../audio/common/frame_length_adaptive.c \
	../../../../audio/common/jlstream_event_handler.c \
	../../../../audio/common/lyrics/lyrics_api.c \
	../../../../audio/common/online_debug/aud_data_export.c \
	../../../../audio/common/online_debug/aud_mic_dut.c \
	../../../../audio/common/online_debug/aud_spatial_effect_dut.c \
	../../../../audio/common/online_debug/audio_capture.c \
	../../../../audio/common/online_debug/audio_online_debug.c \
	../../../../audio/common/uartPcmSender.c \
	../../../../audio/cpu/wl83/audio_accelerator/hw_fft.c \
	../../../../audio/cpu/wl83/audio_config.c \
	../../../../audio/cpu/wl83/audio_dai/audio_iis.c \
	../../../../audio/cpu/wl83/audio_dai/audio_link.c \
	../../../../audio/cpu/wl83/audio_dai/audio_pdm.c \
	../../../../audio/cpu/wl83/audio_setup.c \
	../../../../audio/cpu/wl83/media_memory_manager.c \
	../../../../audio/effect/audio_dc_offset_remove.c \
	../../../../audio/effect/audio_pitch_speed_api.c \
	../../../../audio/effect/audio_voice_changer_api.c \
	../../../../audio/effect/bass_treble.c \
	../../../../audio/effect/demo/autotune_demo.c \
	../../../../audio/effect/demo/bass_treble_demo.c \
	../../../../audio/effect/demo/chorus_demo.c \
	../../../../audio/effect/demo/crossover_demo.c \
	../../../../audio/effect/demo/drc_demo.c \
	../../../../audio/effect/demo/dynamic_eq_demo.c \
	../../../../audio/effect/demo/echo_demo.c \
	../../../../audio/effect/demo/energy_detect_demo.c \
	../../../../audio/effect/demo/eq_demo.c \
	../../../../audio/effect/demo/frequency_shift_howling_demo.c \
	../../../../audio/effect/demo/gain_demo.c \
	../../../../audio/effect/demo/harmonic_exciter_demo.c \
	../../../../audio/effect/demo/noisegate_demo.c \
	../../../../audio/effect/demo/notch_howling_demo.c \
	../../../../audio/effect/demo/pitch_speed_demo.c \
	../../../../audio/effect/demo/reverb_advance_demo.c \
	../../../../audio/effect/demo/reverb_demo.c \
	../../../../audio/effect/demo/spectrum_demo.c \
	../../../../audio/effect/demo/stereo_widener_demo.c \
	../../../../audio/effect/demo/surround_demo.c \
	../../../../audio/effect/demo/virtual_bass_demo.c \
	../../../../audio/effect/demo/voice_changer_demo.c \
	../../../../audio/effect/effects_adj.c \
	../../../../audio/effect/effects_default_param.c \
	../../../../audio/effect/effects_dev.c \
	../../../../audio/effect/eq_config.c \
	../../../../audio/effect/esco_ul_voice_changer.c \
	../../../../audio/effect/node_param_update.c \
	../../../../audio/effect/scene_update.c \
	../../../../audio/effect/spatial_effect/spatial_effect.c \
	../../../../audio/effect/spatial_effect/spatial_effect_imu.c \
	../../../../audio/effect/spatial_effect/spatial_effect_tws.c \
	../../../../audio/effect/spatial_effect/spatial_effects_process.c \
	../../../../audio/effect/spatial_effect/spatial_imu_trim.c \
	../../../../audio/effect/spk_eq.c \
	../../../../audio/framework/nodes/a2dp_tx_node.c \
	../../../../audio/framework/nodes/agc_node.c \
	../../../../audio/framework/nodes/channle_swap_node.c \
	../../../../audio/framework/nodes/cvp_3mic_node.c \
	../../../../audio/framework/nodes/cvp_develop_node.c \
	../../../../audio/framework/nodes/cvp_dms_node.c \
	../../../../audio/framework/nodes/cvp_sms_node.c \
	../../../../audio/framework/nodes/data_export_node.c \
	../../../../audio/framework/nodes/dns_node.c \
	../../../../audio/framework/nodes/effect_dev0_node.c \
	../../../../audio/framework/nodes/effect_dev1_node.c \
	../../../../audio/framework/nodes/effect_dev2_node.c \
	../../../../audio/framework/nodes/effect_dev3_node.c \
	../../../../audio/framework/nodes/effect_dev4_node.c \
	../../../../audio/framework/nodes/esco_tx_node.c \
	../../../../audio/framework/nodes/iis_node.c \
	../../../../audio/framework/nodes/le_audio_source.c \
	../../../../audio/framework/nodes/lhdc_x_node.c \
	../../../../audio/framework/nodes/local_tws_source_node.c \
	../../../../audio/framework/nodes/multi_ch_iis_node.c \
	../../../../audio/framework/nodes/ns_node.c \
	../../../../audio/framework/nodes/pc_mic_node.c \
	../../../../audio/framework/nodes/plc_node.c \
	../../../../audio/framework/nodes/sink_dev0_node.c \
	../../../../audio/framework/nodes/sink_dev1_node.c \
	../../../../audio/framework/nodes/sink_dev2_node.c \
	../../../../audio/framework/nodes/sink_dev3_node.c \
	../../../../audio/framework/nodes/sink_dev4_node.c \
	../../../../audio/framework/nodes/spatial_effects_node.c \
	../../../../audio/framework/nodes/spdif_node.c \
	../../../../audio/framework/nodes/surround_demo_node.c \
	../../../../audio/framework/nodes/uart_node.c \
	../../../../audio/framework/nodes/usb_host_mic_node.c \
	../../../../audio/framework/nodes/vad_node.c \
	../../../../audio/framework/nodes/vir_data_rx_node.c \
	../../../../audio/framework/nodes/vir_data_tx_node.c \
	../../../../audio/framework/nodes/volume_node.c \
	../../../../audio/framework/plugs/source/a2dp_file.c \
	../../../../audio/framework/plugs/source/a2dp_streamctrl.c \
	../../../../audio/framework/plugs/source/adc_file.c \
	../../../../audio/framework/plugs/source/esco_file.c \
	../../../../audio/framework/plugs/source/fm_file.c \
	../../../../audio/framework/plugs/source/iis_file.c \
	../../../../audio/framework/plugs/source/le_audio_file.c \
	../../../../audio/framework/plugs/source/linein_file.c \
	../../../../audio/framework/plugs/source/local_tws_file.c \
	../../../../audio/framework/plugs/source/multi_ch_adc_file.c \
	../../../../audio/framework/plugs/source/multi_ch_iis_file.c \
	../../../../audio/framework/plugs/source/pc_spk_file.c \
	../../../../audio/framework/plugs/source/pdm_mic_file.c \
	../../../../audio/framework/plugs/source/source_dev0_file.c \
	../../../../audio/framework/plugs/source/source_dev1_file.c \
	../../../../audio/framework/plugs/source/source_dev2_file.c \
	../../../../audio/framework/plugs/source/source_dev3_file.c \
	../../../../audio/framework/plugs/source/source_dev4_file.c \
	../../../../audio/framework/plugs/source/spdif_file.c \
	../../../../audio/framework/plugs/source/usb_host_spk_file.c \
	../../../../audio/interface/player/a2dp_player.c \
	../../../../audio/interface/player/adda_loop_player.c \
	../../../../audio/interface/player/dev_flow_player.c \
	../../../../audio/interface/player/esco_player.c \
	../../../../audio/interface/player/file_player.c \
	../../../../audio/interface/player/fm_player.c \
	../../../../audio/interface/player/iis_player.c \
	../../../../audio/interface/player/key_tone_player.c \
	../../../../audio/interface/player/le_audio_player.c \
	../../../../audio/interface/player/linein_player.c \
	../../../../audio/interface/player/local_tws_player.c \
	../../../../audio/interface/player/net_file_player.c \
	../../../../audio/interface/player/pc_spk_player.c \
	../../../../audio/interface/player/reference_time.c \
	../../../../audio/interface/player/ring_player.c \
	../../../../audio/interface/player/tone_player.c \
	../../../../audio/interface/player/tts_player.c \
	../../../../audio/interface/player/tws_tone_player.c \
	../../../../audio/interface/player/usb_host_spk_player.c \
	../../../../audio/interface/player/video_dec_player.c \
	../../../../audio/interface/player/vir_dev_player.c \
	../../../../audio/interface/player/vir_source_player.c \
	../../../../audio/interface/recorder/ai_voice_recorder.c \
	../../../../audio/interface/recorder/dev_flow_recorder.c \
	../../../../audio/interface/recorder/esco_recorder.c \
	../../../../audio/interface/recorder/file_recorder.c \
	../../../../audio/interface/recorder/le_audio_recorder.c \
	../../../../audio/interface/recorder/pc_mic_recorder.c \
	../../../../audio/interface/recorder/usb_host_mic_recorder.c \
	../../../../audio/interface/recorder/video_rec_recorder.c \
	../../../../audio/interface/recorder/vir_dev_recorder.c \
	../../../../audio/interface/recorder/voiceprint_cfg_recorder.c \
	../../../../audio/jl_kws/jl_kws_algo.c \
	../../../../audio/jl_kws/jl_kws_audio.c \
	../../../../audio/jl_kws/jl_kws_event.c \
	../../../../audio/jl_kws/jl_kws_main.c \
	../../../../audio/le_audio/big.c \
	../../../../audio/le_audio/cig.c \
	../../../../audio/le_audio/le_audio_stream.c \
	../../../../audio/le_audio/wireless_trans_manager.c \
	../../../../audio/log_config/lib_media_config.c \
	../../../../audio/music/audio_dec_midi_ctrl.c \
	../../../../audio/music/audio_dec_midi_file.c \
	../../../../audio/music/breakpoint.c \
	../../../../audio/music/music_decrypt.c \
	../../../../audio/music/music_id3.c \
	../../../../audio/music/music_player.c \
	../../../../audio/smart_voice/jl_kws_platform.c \
	../../../../audio/smart_voice/kws_event.c \
	../../../../audio/smart_voice/nn_vad.c \
	../../../../audio/smart_voice/smart_voice_config.c \
	../../../../audio/smart_voice/smart_voice_core.c \
	../../../../audio/smart_voice/user_asr.c \
	../../../../audio/smart_voice/voice_mic_data.c \
	../../../../audio/smart_voice/wanson_asr.c \
	../../../../cpu/wl83/adc_api.c \
	../../../../cpu/wl83/debug.c \
	../../../../cpu/wl83/debug_exception.c \
	../../../../cpu/wl83/setup.c \


# 需要编译的 .S 文件
S_SRC_FILES := \
	../../../../include_lib/btctrler/version.z.S \
	../../../../include_lib/btstack/version.z.S \
	../../../../include_lib/driver/version.z.S \
	../../../../include_lib/net/version.z.S \
	../../../../include_lib/server/version.z.S \
	../../../../include_lib/system/version.z.S \
	../../../../include_lib/update/version.z.S \
	../../../../include_lib/utils/version.z.S \
	../../../../lib/net/version.z.S \
	../../../../lib/server/version.z.S \
	../../../../lib/utils/version.z.S \


# 需要编译的 .s 文件
s_SRC_FILES :=


# 需要编译的 .cpp 文件
cpp_SRC_FILES := \
	../../../../apps/common/c++/cxx_runtime.cpp \


# 需要编译的 .cc 文件
cc_SRC_FILES :=


# 需要编译的 .cxx 文件
cxx_SRC_FILES :=


# 链接参数
LFLAGS := \
	--plugin-opt=-pi32v2-always-use-itblock=false \
	--plugin-opt=-enable-ipra=true \
	--plugin-opt=-pi32v2-merge-max-offset=4096 \
	--plugin-opt=-pi32v2-enable-simd=true \
	--plugin-opt=mcpu=r3 \
	--plugin-opt=-global-merge-on-const \
	--plugin-opt=-inline-threshold=5 \
	--plugin-opt=-inline-max-allocated-size=32 \
	--plugin-opt=-inline-normal-into-special-section=true \
	--plugin-opt=-dont-used-symbol-list=malloc,free,sprintf,printf,puts,putchar \
	--plugin-opt=save-temps \
	--plugin-opt=-pi32v2-enable-rep-memop \
	--plugin-opt=-emulated-tls \
	--sort-common \
	--plugin-opt=-used-symbol-file=../../../../cpu/wl83/sdk_used_list.used \
	--plugin-opt=-enable-movable-region=true \
	--plugin-opt=-movable-region-section-prefix=.movable.slot. \
	--plugin-opt=-movable-region-stub-section-prefix=.movable.stub. \
	--plugin-opt=-movable-region-prefix=.movable.region. \
	--plugin-opt=-movable-region-stub-prefix=__movable_stub_ \
	--plugin-opt=-movable-region-stub-swi-number=-2 \
	--plugin-opt=-movable-region-map-file-list=../../../../apps/common/movable/funcname.txt \
	--plugin-opt=-movable-region-section-map-file-list=../../../../apps/common/movable/section.txt \
	--plugin-opt=-movable-region-exclude-func-file-list=../../../../apps/common/movable/exclude.txt \
	--plugin-opt=-pi32v2-large-program=true \
	--plugin-opt=-pi32v2-csync-before-rti=true \
	../../../../include_lib/c++/libstdc++/ucos/libemutls.a \
	../../../../include_lib/c++/libstdc++/ucos/libc++.a \
	../../../../include_lib/c++/libstdc++/ucos/libc++abi.a \
	../../../../cpu/wl83/liba/le_audio_profile.a \
	../../../../cpu/wl83/liba/lib_sig_mesh.a \
	../../../../cpu/wl83/liba/lib_mijia.a \
	--start-group \
	../../../../cpu/wl83/liba/btctrler.a \
	../../../../cpu/wl83/liba/btstack.a \
	--end-group \
	../../../../cpu/wl83/liba/lib_ccm_aes.a \
	../../../../cpu/wl83/liba/crypto_toolbox_Osize.a \
	../../../../cpu/wl83/liba/wl_wifi.a \
	../../../../cpu/wl83/liba/wpasupplicant.a \
	../../../../cpu/wl83/liba/libcrypto_mbedtls.a \
	../../../../cpu/wl83/liba/wl_rf_common.a \
	../../../../cpu/wl83/liba/hsm.a \
	../../../../cpu/wl83/liba/lte_module.a \
	../../../../cpu/wl83/liba/profile.a \
	../../../../cpu/wl83/liba/libduer-device.a \
	../../../../cpu/wl83/liba/network_download.a \
	../../../../cpu/wl83/liba/lib_mqtt.a \
	../../../../cpu/wl83/liba/libdlna.a \
	../../../../cpu/wl83/liba/websocket.a \
	../../../../cpu/wl83/liba/stream_media_server.a \
	../../../../cpu/wl83/liba/http_cli.a \
	../../../../cpu/wl83/liba/https_cli.a \
	../../../../cpu/wl83/liba/http_server.a \
	../../../../cpu/wl83/liba/iperf.a \
	../../../../cpu/wl83/liba/ai_server.a \
	../../../../cpu/wl83/liba/libcurl.a \
	../../../../cpu/wl83/liba/lwip_2_2_0.a \
	../../../../cpu/wl83/liba/libmbedtls_3_4_0.a \
	../../../../cpu/wl83/liba/json.a \
	../../../../cpu/wl83/liba/cJSON.a \
	../../../../cpu/wl83/liba/libmpeg.a \
	../../../../cpu/wl83/liba/zliblite.a \
	../../../../cpu/wl83/liba/lib_sound_wave.a \
	../../../../cpu/wl83/liba/libqrcode.a \
	../../../../cpu/wl83/liba/net_server.a \
	../../../../cpu/wl83/liba/video_engine.a \
	../../../../cpu/wl83/liba/video_rec_server.a \
	../../../../cpu/wl83/liba/video_dec_server.a \
	../../../../cpu/wl83/liba/video.a \
	../../../../cpu/wl83/liba/lib_amap_demo.a \
	../../../../cpu/wl83/liba/media.a \
	../../../../cpu/wl83/liba/media/lib_mp3_dec.a \
	../../../../cpu/wl83/liba/media/lib_m4a_dec.a \
	../../../../cpu/wl83/liba/media/lib_mty_dec.a \
	../../../../cpu/wl83/liba/media/lib_f2a_dec.a \
	../../../../cpu/wl83/liba/media/lib_wav_dec.a \
	../../../../cpu/wl83/liba/media/lib_wtg_dec.a \
	../../../../cpu/wl83/liba/media/lib_wtgv2_dec.a \
	../../../../cpu/wl83/liba/media/lib_flac_dec.a \
	../../../../cpu/wl83/liba/media/lib_ape_dec.a \
	../../../../cpu/wl83/liba/media/lib_amr_dec.a \
	../../../../cpu/wl83/liba/media/lib_amr_wb_dec.a \
	../../../../cpu/wl83/liba/media/lib_dts_dec.a \
	../../../../cpu/wl83/liba/media/lib_wma_dec.a \
	../../../../cpu/wl83/liba/media/lib_sbc_dec.a \
	../../../../cpu/wl83/liba/media/lib_midi_dec.a \
	../../../../cpu/wl83/liba/media/lib_bt_aac_dec.a \
	../../../../cpu/wl83/liba/media/lib_ogg_opus_dec.a \
	../../../../cpu/wl83/liba/media/lib_ogg_vorbis_dec.a \
	../../../../cpu/wl83/liba/media/lib_ldac_dec.a \
	../../../../cpu/wl83/liba/media/lib_lhdcv4_dec.a \
	../../../../cpu/wl83/liba/media/lib_lhdcv5_dec.a \
	../../../../cpu/wl83/liba/media/lib_jla_codec.a \
	../../../../cpu/wl83/liba/media/lib_jla_v2_codec.a \
	../../../../cpu/wl83/liba/media/lib_jla_ll_codec.a \
	../../../../cpu/wl83/liba/media/lib_jla_lw_codec.a \
	../../../../cpu/wl83/liba/media/lib_lc3_codec.a \
	../../../../cpu/wl83/liba/media/lib_jl_codec_common.a \
	../../../../cpu/wl83/liba/media/lib_mp3_dec_stream.a \
	../../../../cpu/wl83/liba/media/lib_msbc_codec.a \
	../../../../cpu/wl83/liba/media/lib_speex_codec.a \
	../../../../cpu/wl83/liba/media/lib_mp2_codec.a \
	../../../../cpu/wl83/liba/media/lib_adpcm_codec.a \
	../../../../cpu/wl83/liba/media/lib_adpcm_ima_codec.a \
	../../../../cpu/wl83/liba/media/lib_aac_enc.a \
	../../../../cpu/wl83/liba/media/lib_amr_enc.a \
	../../../../cpu/wl83/liba/media/lib_amr_wb_enc.a \
	../../../../cpu/wl83/liba/media/lib_mp3_enc.a \
	../../../../cpu/wl83/liba/media/lib_opus_enc.a \
	../../../../cpu/wl83/liba/media/lib_sbc_eng.a \
	../../../../cpu/wl83/liba/media/lib_bt_hash_enc.a \
	../../../../cpu/wl83/liba/media/lib_advaudio_plc.a \
	../../../../cpu/wl83/liba/media/lib_esco_audio_plc.a \
	../../../../cpu/wl83/liba/media/libJLPhoneCall.a \
	../../../../cpu/wl83/liba/media/libDns.a \
	../../../../cpu/wl83/liba/media/libJLsp.a \
	../../../../cpu/wl83/liba/media/libJLspKws.a \
	../../../../cpu/wl83/liba/media/libJLspKwsFarKeyword.a \
	../../../../cpu/wl83/liba/media/libKwsCommon.a \
	../../../../cpu/wl83/liba/media/libAptFilt.a \
	../../../../cpu/wl83/liba/media/libEchoSuppress.a \
	../../../../cpu/wl83/liba/media/libNoiseSuppress.a \
	../../../../cpu/wl83/liba/media/libAdaptiveEchoSuppress.a \
	../../../../cpu/wl83/liba/media/libSplittingFilter.a \
	../../../../cpu/wl83/liba/media/libDelayEstimate.a \
	../../../../cpu/wl83/liba/media/libDualMicSystem.a \
	../../../../cpu/wl83/liba/media/libSMS_TDE.a \
	../../../../cpu/wl83/liba/media/libSAS.a \
	../../../../cpu/wl83/liba/media/libAec.a \
	../../../../cpu/wl83/liba/media/libFFT.a \
	../../../../cpu/wl83/liba/media/libDynamicEQ.a \
	../../../../cpu/wl83/liba/media/libDynamicEQPro.a \
	../../../../cpu/wl83/liba/media/libLoudness.a \
	../../../../cpu/wl83/liba/media/libLlns.a \
	../../../../cpu/wl83/liba/media/libHowling.a \
	../../../../cpu/wl83/liba/media/libCrossoverCoff.a \
	../../../../cpu/wl83/liba/media/libDrc.a \
	../../../../cpu/wl83/liba/media/libDrcDetect.a \
	../../../../cpu/wl83/liba/media/libDrcAdvance.a \
	../../../../cpu/wl83/liba/media/libVoiceChanger.a \
	../../../../cpu/wl83/liba/media/libFrequencyShift.a \
	../../../../cpu/wl83/liba/media/libFrequencyCompressor.a \
	../../../../cpu/wl83/liba/media/libReverb.a \
	../../../../cpu/wl83/liba/media/libReverbCal.a \
	../../../../cpu/wl83/liba/media/libPitchSpeed.a \
	../../../../cpu/wl83/liba/media/libHarmonicExciter.a \
	../../../../cpu/wl83/liba/media/libResampleFastCal.a \
	../../../../cpu/wl83/liba/media/libChorus.a \
	../../../../cpu/wl83/liba/media/libPlateReverbAdv.a \
	../../../../cpu/wl83/liba/media/libPcmDelay.a \
	../../../../cpu/wl83/liba/media/libPingPongDelay.a \
	../../../../cpu/wl83/liba/media/libThreeDEffect.a \
	../../../../cpu/wl83/liba/media/libAutoWah.a \
	../../../../cpu/wl83/liba/media/libSpectrum.a \
	../../../../cpu/wl83/liba/media/libSpectrumShow.a \
	../../../../cpu/wl83/liba/media/libStereoWidener.a \
	../../../../cpu/wl83/liba/media/libStereoSpatialWider.a \
	../../../../cpu/wl83/liba/media/libStereoMtapsEcho.a \
	../../../../cpu/wl83/liba/media/libSurroundEffect.a \
	../../../../cpu/wl83/liba/media/libVirtualBass.a \
	../../../../cpu/wl83/liba/media/libVirtualBassClassic.a \
	../../../../cpu/wl83/liba/media/libHowlingGate.a \
	../../../../cpu/wl83/liba/media/libDistortion.a \
	../../../../cpu/wl83/liba/media/libDistortionEXP.a \
	../../../../cpu/wl83/liba/media/libNoisegate.a \
	../../../../cpu/wl83/liba/media/libNoisegatePro.a \
	../../../../cpu/wl83/liba/media/libIIRFilter.a \
	../../../../cpu/wl83/liba/media/libLimiter.a \
	../../../../cpu/wl83/liba/media/libBfilterFun.a \
	../../../../cpu/wl83/liba/media/lib_phaser.a \
	../../../../cpu/wl83/liba/media/lib_flanger.a \
	../../../../cpu/wl83/liba/media/lib_chorus_adv.a \
	../../../../cpu/wl83/liba/media/lib_pingpong_echo.a \
	../../../../cpu/wl83/liba/media/lib_wanson_asr.a \
	../../../../cpu/wl83/liba/libvad.a \
	../../../../cpu/wl83/liba/event.a \
	../../../../cpu/wl83/liba/update.a \
	../../../../cpu/wl83/liba/cpu.a \
	../../../../cpu/wl83/liba/system.a \
	../../../../cpu/wl83/liba/fs.a \
	../../../../cpu/wl83/liba/cfg_tool.a \
	../../../../cpu/wl83/liba/common_lib.a \
	../../../../cpu/wl83/liba/btstack.a  \
	../../../../cpu/wl83/liba/btctrler.a  \
	../../../../cpu/wl83/liba/crypto_toolbox_Osize.a  \
	../../../../cpu/wl83/liba/lib_ccm_aes.a  \
	../../../../cpu/wl83/liba/lib_sig_mesh.a  \
	../../../../cpu/wl83/liba/rcsp_stack.a \
	../../../../cpu/wl83/liba/lvgl_v8_extend.a \
	../../../../include_lib/newlib/pi32v2-lib/libm.a \
	../../../../include_lib/newlib/pi32v2-lib/libc.a \
	../../../../include_lib/newlib/pi32v2-lib/libcompiler_rt.a \
	-T../../../../cpu/wl83/sdk.ld \
	-M=../../../../cpu/wl83/tools/sdk.map \
	--plugin-opt=mcpu=r3 \
	--plugin-opt=-mattr=+fprev1 \



c_OBJS    := $(c_SRC_FILES:%.c=%.c.o)
S_OBJS    := $(S_SRC_FILES:%.S=%.S.o)
s_OBJS    := $(s_SRC_FILES:%.s=%.s.o)
cpp_OBJS  := $(cpp_SRC_FILES:%.cpp=%.cpp.o)
cxx_OBJS  := $(cxx_SRC_FILES:%.cxx=%.cxx.o)
cc_OBJS   := $(cc_SRC_FILES:%.cc=%.cc.o)

OBJS      := $(c_OBJS) $(S_OBJS) $(s_OBJS) $(cpp_OBJS) $(cxx_OBJS) $(cc_OBJS)
DEP_FILES := $(OBJS:%.o=%.d)


OBJS      := $(addprefix $(BUILD_DIR)/, $(OBJS:$(ROOT_PREFIX)/%=%))
DEP_FILES := $(addprefix $(BUILD_DIR)/, $(DEP_FILES:$(ROOT_PREFIX)/%=%))


VERBOSE ?= 0
ifeq ($(VERBOSE), 1)
QUITE :=
else
QUITE := @
endif

# 一些旧的 make 不支持 file 函数，需要 make 的时候指定 LINK_AT=0 make
LINK_AT ?= 1

# 表示下面的不是一个文件的名字，无论是否存在 all, clean, pre_build 这样的文件
# 还是要执行命令
# see: https://www.gnu.org/software/make/manual/html_node/Phony-Targets.html
.PHONY: all clean pre_build

# 不要使用 make 预设置的规则
# see: https://www.gnu.org/software/make/manual/html_node/Suffix-Rules.html
.SUFFIXES:

all: pre_build $(OUT_ELF)
	$(info +POST-BUILD)
	$(QUITE) $(RUN_POST_SCRIPT) sdk

pre_build:
	$(info +PRE-BUILD)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -D__LD__ -E -P ../../../../cpu/wl83/sdk_used_list.c -o ../../../../cpu/wl83/sdk_used_list.used
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -D__LD__ -E -P ../../../../apps/common/movable/section.c -o ../../../../apps/common/movable/section.txt
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -D__LD__ -E -P ../../../../cpu/wl83/sdk_ld.c -o ../../../../cpu/wl83/sdk.ld
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -D__LD__ -E -P ../../../../cpu/wl83/tools/download.c -o $(POST_SCRIPT)
	$(QUITE) $(FIXBAT) $(POST_SCRIPT)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -D__LD__ -E -P ../../../../cpu/wl83/tools/isd_config_rule.c -o ../../../../cpu/wl83/tools/isd_config.ini
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -D__LD__ -E -P ../../../../cpu/wl83/tools/isd_config_rule_loader.c -o ../../../../cpu/wl83/tools/isd_config_loader.ini

clean:
	$(QUITE) $(RM) $(OUT_ELF)
	$(QUITE) $(RM) $(BUILD_DIR)



ifeq ($(LINK_AT), 1)
$(OUT_ELF): $(OBJS)
	$(info +LINK $@)
	$(shell $(MKDIR) $(@D))
	$(file >$(OBJ_FILE), $(OBJS))
	$(QUITE) $(LD) -o $(OUT_ELF) @$(OBJ_FILE) $(LFLAGS) $(LIBPATHS) $(LIBS)
else
$(OUT_ELF): $(OBJS)
	$(info +LINK $@)
	$(shell $(MKDIR) $(@D))
	$(QUITE) $(LD) -o $(OUT_ELF) $(OBJS) $(LFLAGS) $(LIBPATHS) $(LIBS)
endif


$(BUILD_DIR)/%.c.o : $(ROOT_PREFIX)/%.c
	$(info +CC $<)
	$(QUITE) $(MKDIR) $(@D)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -MMD -MF $(@:.o=.d) -c $< -o $@

$(BUILD_DIR)/%.S.o : $(ROOT_PREFIX)/%.S
	$(info +AS $<)
	$(QUITE) $(MKDIR) $(@D)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -MMD -MF $(@:.o=.d) -c $< -o $@

$(BUILD_DIR)/%.s.o : $(ROOT_PREFIX)/%.s
	$(info +AS $<)
	$(QUITE) $(MKDIR) $(@D)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -MMD -MF $(@:.o=.d) -c $< -o $@

$(BUILD_DIR)/%.cpp.o : $(ROOT_PREFIX)/%.cpp
	$(info +CXX $<)
	$(QUITE) $(MKDIR) $(@D)
	$(QUITE) $(CXX) $(CXXFLAGS) $(CFLAGS) $(DEFINES) $(INCLUDES) -MMD -MF $(@:.o=.d) -c $< -o $@

$(BUILD_DIR)/%.cxx.o : $(ROOT_PREFIX)/%.cxx
	$(info +CXX $<)
	$(QUITE) $(MKDIR) $(@D)
	$(QUITE) $(CXX) $(CXXFLAGS) $(CFLAGS) $(DEFINES) $(INCLUDES) -MMD -MF $(@:.o=.d) -c $< -o $@

$(BUILD_DIR)/%.cc.o : $(ROOT_PREFIX)/%.cc
	$(info +CXX $<)
	$(QUITE) $(MKDIR) $(@D)
	$(QUITE) $(CXX) $(CXXFLAGS) $(CFLAGS) $(DEFINES) $(INCLUDES) -MMD -MF $(@:.o=.d) -c $< -o $@

-include $(DEP_FILES)

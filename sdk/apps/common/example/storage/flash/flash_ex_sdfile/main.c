#include "app_config.h"
#include "system/includes.h"
#include "fs/fs.h"
#include "asm/sfc_norflash_api.h"

//1、工具需要支持的的isd_donwload
//2、loader需要支持的wl83loader.bin
//3、isd_config_rule.c配置:
/* [SYS_CFG_PARAM] */
/* EX_FLASH = PA04_1A_NULL_0; */
/* EX_FLASH_IO = 4_PA09_PA10_PA05_PA06_PA08; */

/* [EX_FLASH_CFG] */
/* EXFS_FILE=fw.cfg; */
/* EXFS_ADR=AUTO; */
/* EXFS_LEN=0x1000; */
/* EXFS_OPT=1; */
/*  */
/* EXFS1_FILE=local.db; */
/* EXFS1_ADR=AUTO; */
/* EXFS1_LEN=0x2000; */
/* EXFS1_OPT=1; */

//4、打开sfc1: sfc_spi_open();
/* void board_init(void) */
/* { */
/*  */
/*     y_printf("\n >>>[test]:func = %s,line= %d\n",__FUNCTION__, __LINE__); */
/*     int sfc_spi_init(const struct sfc_spi_platform_data *sfc_spi_data); */
/*     int sfc_spi_open(void); */
/*     #<{(| sfc_spi_init(&sfc_spi_data); |)}># */
/*     sfc_spi_open(); */
/*     ...... */

//5、测试代码
static void flash_ex_sdfile_test(void)
{
    char buf[512];
    FILE *fp = fopen("mnt/sdfile/EX_FLASH/EXFS", "r");
    if (!fp) {
        y_printf(">>>[test]:error open\n");
    } else {
        y_printf(">>>[test]:open OK\n");
        struct vfs_attr attrs;
        fget_attrs(fp, &attrs);
        y_printf(">>>[test]:addr = 0x%x, len = 0x%x\n", attrs.sclust, attrs.fsize);
        fread(buf, 32, 1, fp);
        put_buf(buf, 32);
        fclose(fp);
    }

    fp = fopen("mnt/sdfile/EX_FLASH/EXFS1", "r");
    if (!fp) {
        y_printf(">>>[test]:error open\n");
    } else {
        y_printf(">>>[test]:open OK\n");
        struct vfs_attr attrs;
        fget_attrs(fp, &attrs);
        y_printf(">>>[test]:addr = 0x%x, len = 0x%x\n", attrs.sclust, attrs.fsize);
        fread(buf, 512, 1, fp);
        put_buf(buf, 512);
    }
    while (1) {
        os_time_dly(10);
    }
}
static int c_main(void)
{
    os_task_create(flash_ex_sdfile_test, NULL, 12, 1000, 0, "flash_ex_sdfile_test");
    return 0;
}
/* late_initcall(c_main); */

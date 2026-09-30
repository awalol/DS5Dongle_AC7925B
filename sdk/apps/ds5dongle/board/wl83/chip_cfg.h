#ifndef CONFIG_CHIP_CFG_H
#define CONFIG_CHIP_CFG_H

/* AC7925BE: 内置 DDR1 4M*16bit = 64Mbit = 8MB
 * 外置 Flash GD25Q64ESIGR (V2.0 板) = 64Mbit = 8MB
 * 参考选型表: E 后缀 = 4Mx16bit DDR1 */
#define __FLASH_SIZE__    (8 * 1024 * 1024)
#define __SDRAM_SIZE__    (8 * 1024 * 1024)

#endif

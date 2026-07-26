#ifdef CONFIG_BOARD_DEMO

//*********************************************************************************//
//                                功能模块配置                                     //
//*********************************************************************************//
#define CONFIG_SFC_ENABLE
#define CONFIG_NO_SDRAM_ENABLE


//*********************************************************************************//
//                                   时钟配置                                      //
//*********************************************************************************//
#define TCFG_OSC_FREQUENCY                  24000000
#define TCFG_SYS_CLK                        192000000         //240M,192M,160M,120M,96M,80M,64M,60M,48M,40M,32M,24M，其他值时启用SYS_PLL
#define TCFG_LSB_CLK                        48000000          //96M,80M,60M,48M,40M,24M,20M,12M
#define TCFG_HSB_CLK_DIV                    2                 //HSB_CLK = SYS_CLK / HSB_CLK_DIV
#define TCFG_SFCTZ_CLK                      48000000          //SFC时钟，和加载代码的速度有关
#define TCFG_SDRAM_CLK                      200000000         //SDRAM/DDR时钟


//*********************************************************************************//
//                                  flash配置                                      //
//*********************************************************************************//
/*
#data_width[1 2 3 4] 3的时候uboot自动识别2或者4线
#clkdiv [1-255] SPITZ_CLK=SYS_CLK/HSB_CLK_DIV/clkdiv
#mode:
#	  0 RD_OUTPUT,		 1 cmd 		 1 addr
#	  1 RD_I/O,   		 1 cmd 		 x addr
#	  2 RD_I/O_CONTINUE] no_send_cmd x add
#port:固定0
*/
#define TCFG_SPITZ_WIDTH_CLKDIV_MODE_PORT   4_3_0_0

/*
#FLASH_QE_POS和FLASH_SPEC_QE_ID为'或'关系, 有一项条件符合，开机会按特殊QE位flash配置
#TCFG_FLASH_QE_POS     —— QE位置, 0-常规(SR2-BIT1), 1-特殊(SR1-BIT6)
#TCFG_FLASH_SPEC_QE_ID —— QE在SR1_BIT6的FlashID列表(最长128个字符, 'X'为通配符)
*/
#define TCFG_FLASH_QE_POS                   0
#define TCFG_FLASH_SPEC_QE_ID               C220XX_9D70XX_1C7019

//*********************************************************************************//
//                                  sdram配置                                      //
//*********************************************************************************//
#define TCFG_FREE_DCACHE_WAY                0
#define TCFG_SDRAM_MODE                     1              //0-sdram    1-ddr1


//*********************************************************************************//
//                                  UART配置                                       //
//*********************************************************************************//
#define TCFG_DEBUG_PORT                     IO_PORTD_01
#define TCFG_UBOOT_DEBUG_PORT               PD01

#define TCFG_UART0_ENABLE                   1              //UART0默认用来打印口
#define TCFG_UART0_TX_IO                    TCFG_DEBUG_PORT
#define TCFG_UART0_RX_IO                    IO_PORTE_11    //-1
#define TCFG_UART0_BAUDRATE                 1000000
#define TCFG_UART0_PARITY                   UART_PARITY_DISABLE


//*********************************************************************************//
//                                  AD按键配置                                     //
//*********************************************************************************//
#define TCFG_ADKEY_ENABLE                   1              //AD按键
#define TCFG_ADKEY_INPUT_IO                 IO_PORTD_00
#define TCFG_ADKEY_INPUT_CHANNEL            ADC_IO_CH_PD00

#define ADKEY_UPLOAD_R                      22             //上拉电阻值
#define TCFG_ADC_LEVEL_09                   0x3FF
#define TCFG_ADC_LEVEL_08                   0x3FF
#define TCFG_ADC_LEVEL_07                   0x3FF
#define TCFG_ADC_LEVEL_06                   0x3FF
#define TCFG_ADC_LEVEL_05                   0x3FF
#define TCFG_ADC_LEVEL_04                   (ADC_VDDIO * 100 / (100 + ADKEY_UPLOAD_R))
#define TCFG_ADC_LEVEL_03                   (ADC_VDDIO * 33  / (33  + ADKEY_UPLOAD_R))
#define TCFG_ADC_LEVEL_02                   (ADC_VDDIO * 15  / (15  + ADKEY_UPLOAD_R))
#define TCFG_ADC_LEVEL_01                   (ADC_VDDIO * 51  / (51  + ADKEY_UPLOAD_R * 10))
#define TCFG_ADC_LEVEL_00                   (0)

#define TCFG_ADKEY_VALUE_0                  KEY_POWER
#define TCFG_ADKEY_VALUE_1                  KEY_MODE
#define TCFG_ADKEY_VALUE_2                  KEY_UP
#define TCFG_ADKEY_VALUE_3                  KEY_DOWN
#define TCFG_ADKEY_VALUE_4                  KEY_OK
#define TCFG_ADKEY_VALUE_5                  KEY_CANCLE
#define TCFG_ADKEY_VALUE_6                  KEY_ENC
#define TCFG_ADKEY_VALUE_7                  KEY_PHOTO
#define TCFG_ADKEY_VALUE_8                  KEY_F1
#define TCFG_ADKEY_VALUE_9                  NO_KEY


//*********************************************************************************//
//                                  IO按键配置                                     //
//*********************************************************************************//
#define TCFG_IOKEY_ENABLE                   0
#define TCFG_IOKEY_INPUT0_ENABLE            1
#define TCFG_IOKEY_INPUT0_CONNECT_WAY       ONE_PORT_TO_LOW
#define TCFG_IOKEY_INPUT0_IO                IO_PORTD_00
#define TCFG_IOKEY_INPUT0_VALUE             KEY_POWER
#define TCFG_IOKEY_INPUT1_ENABLE            0
#define TCFG_IOKEY_INPUT1_CONNECT_WAY       ONE_PORT_TO_LOW
#define TCFG_IOKEY_INPUT1_IO                IO_PORTC_01
#define TCFG_IOKEY_INPUT1_VALUE             KEY_VOLUME_INC
#define TCFG_IOKEY_INPUT2_ENABLE            0
#define TCFG_IOKEY_INPUT2_CONNECT_WAY       ONE_PORT_TO_LOW
#define TCFG_IOKEY_INPUT2_IO                IO_PORTC_02
#define TCFG_IOKEY_INPUT2_VALUE             KEY_VOLUME_DEC


//*********************************************************************************//
//                              电源低功耗配置                                     //
//*********************************************************************************//
#define TCFG_POWER_MODE                     PWR_DCDC15
#define TCFG_POWER_AVDD18_ENABLE            1
#define TCFG_POWER_AVDD28_ENABLE            1

#define TCFG_LOWPOWER_VDDIOM_LEVEL          VDDIOM_VOL_330V//强VDDIO电压档位
#define TCFG_LOWPOWER_VDDIOW_LEVEL          VDDIOW_VOL_200V //弱VDDIO电压档位
#define TCFG_LOWPOWER_VDC14_LEVEL           DCVDD_VOL_140V
#define TCFG_LOWPOWER_FUNCTION              LOWPOWER_CLOSE
#define TCFG_LOWPOWER_OSC_TYPE              OSC_TYPE_LRC

#define TCFG_LOWPOWER_WAKEUP_PORT0_ENABLE   1
#define TCFG_LOWPOWER_WAKEUP_PORT0_IO       IO_PORTD_00
#define TCFG_LOWPOWER_WAKEUP_PORT0_EDGE     FALLING_EDGE
#define TCFG_LOWPOWER_WAKEUP_PORT0_FILTER   PORT_FLT_DISABLE

#endif

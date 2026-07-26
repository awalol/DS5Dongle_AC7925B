#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "board_config.h"

//*********************************************************************************//
//                                   DEBUG相关配置                                 //
//*********************************************************************************//
#define CONFIG_MPU_DEBUG_ENABLE
// #define CONFIG_RTOS_STACK_CHECK_ENABLE       //是否启用定时检查任务栈
// #define CONFIG_MEM_LEAK_CHECK_ENABLE         //是否启用内存泄漏检查(需要包含mem_leak_test.h头文件)
#define CONFIG_DEBUG_ENABLE                     //打印开关
#define CONFIG_CXX_SUPPORT                      //使能C++支持


//*********************************************************************************//
//                                   Demo例程使能                                  //
//*********************************************************************************//
// #define USE_ADC_TEST_DEMO
// #define USE_CAN_TEST_DEMO
// #define USE_DMA_TEST_DEMO
// #define USE_FLASH_API_TEST_DEMO
// #define USE_FLASH_OPT_TEST_DEMO
// #define USE_FLASH_REGISTER_TEST_DEMO
// #define USE_FLASH_SYSCFG_TEST_DEMO
// #define USE_FLASH_USER_TEST_DEMO
// #define USE_GPCNT_TEST_DEMO
// #define USE_GPCRC_TEST_DEMO
// #define USE_GPIO_TEST_DEMO
// #define USE_IIC_TEST_DEMO
// #define USE_PWM_TEST_DEMO
// #define USE_RTC_TEST_DEMO
// #define USE_SPI_MASTER_TEST_DEMO
// #define USE_SPI_SLAVE_TEST_DEMO
// #define USE_TIMER_TEST1_DEMO
// #define USE_TIMER_CAPTURE_TEST
// #define USE_UART_DEVICE_TEST_DEMO
// #define USE_WDT_TEST_DEMO
// #define USE_LEDC_TEST_DEMO
// #define USE_SPI_SIMULATE_EXP_UART_TEST_DEMO

// #define USE_BENCHMARK_TEST


//*********************************************************************************//
//                                   CAN配置                                       //
//*********************************************************************************//
#ifdef USE_CAN_TEST_DEMO
#define TCFG_CAN_ENABLE                     1
#define TCFG_CANOPEN_ENABLE                 0//TCFG_CAN_ENABLE
#define TCFG_CAN_MODE                       1 //enhanced can
#define TCFG_CAN_RX_IO                      IO_PORTC_01
#define TCFG_CAN_TX_IO                      IO_PORTC_02
#define TCFG_CAN_STM_ENABLE                 0
#define TCFG_CAN_LOM_ENABLE                 0
#define TCFG_CAN_BAUD_BRP                   3
#define TCFG_CAN_BAUD_SJW                   3
#define TCFG_CAN_BAUD_SAM                   1
#define TCFG_CAN_BAUD_PTS_PBS1              2
#define TCFG_CAN_BAUD_PBS2                  1
#if TCFG_CAN_MODE == 1 //enhanced can
#define TCFG_CAN_FILTER_MODE                1
#define TCFG_CAN_FILTER_ACC_ID0             0x1FFFFFFF
#define TCFG_CAN_FILTER_ACC_ID1             0x10FFFFFF
#define TCFG_CAN_FILTER_ACC_RTR0            1
#define TCFG_CAN_FILTER_ACC_RTR1            0
#define TCFG_CAN_FILTER_MASK_ID0            0xFFFFFFFF
#define TCFG_CAN_FILTER_MASK_ID1            0xFFFFFFFF
#define TCFG_CAN_FILTER_MASK_RTR0           1
#define TCFG_CAN_FILTER_MASK_RTR1           1
#else
#define TCFG_CAN_FILTER_MODE                2
#define TCFG_CAN_FILTER_ACC_ID0             0x654
#define TCFG_CAN_FILTER_MASK_ID0            0xFF
#endif // TCFG_CAN_MODE
#endif // USE_CAN_TEST_DEMO


//*********************************************************************************//
//                                  GPCNT配置                                      //
//*********************************************************************************//
#ifdef USE_GPCNT_TEST_DEMO
#define TCFG_GPCNT_ENABLE                   1
#define TCFG_GPCNT_INPUT_IO                 IO_PORTA_04
#define TCFG_GPCNT_GSS_CLK_SOURCE           GPCNT_GSS_CSS_PLL_96M
#define TCFG_GPCNT_CSS_CLK_SOURCE           GPCNT_GSS_CSS_INPUT_CAP_MUX
#define TCFG_GPCNT_GSS_CYCLE                15
#endif // USE_GPCNT_TEST_DEMO


//*********************************************************************************//
//                                   IIC配置                                       //
//*********************************************************************************//
#ifdef USE_IIC_TEST_DEMO
/* IIC0为增强型IIC */
#define TCFG_IIC_ENABLE                     1
#define TCFG_HW_IIC0_ROLE                   IIC_ROLE_MASTER
#define TCFG_HW_IIC0_HD_LEVEL               GPIO_DRIVE_STRENGTH_2p5mA
#define TCFG_HW_IIC0_SCL_IO                 IO_PORTA_04
#define TCFG_HW_IIC0_SDA_IO                 IO_PORTA_05
#define TCFG_HW_IIC0_IRQ_ENABLE             0
#define TCFG_HW_IIC0_BAUDRATE               100000
#define TCFG_HW_IIC0_SLAVE_ADDR             0x86
#define TCFG_HW_IIC0_STRETCH_DISABLE        0

/* IIC1为普通型IIC */
#define TCFG_HW_IIC1_HD_LEVEL               GPIO_DRIVE_STRENGTH_2p5mA
#define TCFG_HW_IIC1_SCL_IO                 IO_PORTA_06
#define TCFG_HW_IIC1_SDA_IO                 IO_PORTA_07
#define TCFG_HW_IIC1_BAUDRATE               100000

/* 软件IIC */
#define TCFG_SW_IIC2_HD_LEVEL               GPIO_DRIVE_STRENGTH_2p5mA
#define TCFG_SW_IIC2_SCL_IO                 IO_PORTA_06
#define TCFG_SW_IIC2_SDA_IO                 IO_PORTA_07
#define TCFG_SW_IIC2_DELAY                  80
#define TCFG_SW_IIC2_BYTE_GAP_TIME          300
#endif // USE_IIC_TEST_DEMO


//*********************************************************************************//
//                                  PWM配置                                      //
//*********************************************************************************//
#ifdef USE_PWM_TEST_DEMO
#define TCFG_PWM1_ENABLE                    1
#define TCFG_PWM1_TIMER_PWM_REMAP_IO        { IO_PORTA_04/*PWM2*/, -1/*PWM3*/, -1/*PWM4*/, -1/*PWM5*/, }
#define TCFG_PWM1_MCPWM_REMAP_IO            {\
                                                -1/*MCPWM0H*/, -1/*MCPWM1H*/, -1/*MCPWM2H*/, -1/*MCPWM3H*/,\
                                                -1/*MCPWM0L*/, -1/*MCPWM1L*/, -1/*MCPWM2L*/, -1/*MCPWM3L*/,\
                                            }
#define TCFG_PWM1_CH_MAPPING                PWM_TIMER2_OPCH
#define TCFG_PWM1_FREQUENCY                 2000000
#define TCFG_PWM1_DUTY                      50
#define TCFG_PWM1_DECIMAL_POINT             0   //占空比小数点精度位: 0<freq<=4K,point_bit=2;4K<freq<=40K,point_bit=1; freq>40K,point_bit=0;
#endif // USE_PWM_TEST_DEM


//*********************************************************************************//
//                                  RTC配置                                        //
//*********************************************************************************//
#ifdef USE_RTC_TEST_DEMO
#define TCFG_RTC_ENABLE                     1
#define TCFG_RTC_CLK_SEL                    CLK_SEL_32K
#define TCFG_RTC_X32XS_LEVEL                X32XS_LEVEL_1
#endif // USE_RTC_TEST_DEMO


//*********************************************************************************//
//                                  SPI配置                                        //
//*********************************************************************************//
#if defined(USE_SPI_MASTER_TEST_DEMO) || defined(USE_SPI_SIMULATE_EXP_UART_TEST_DEMO)
#define TCFG_SPI1_ENABLE                    1
#define TCFG_SPI1_CS_IO                     -1
#define TCFG_SPI1_CLK_IO                    IO_PORTA_04
#define TCFG_SPI1_DO_IO                     IO_PORTA_05
#define TCFG_SPI1_DI_IO                     IO_PORTA_06
#define TCFG_SPI1_D2_IO                     -1 // IO_PORTA_07
#define TCFG_SPI1_D3_IO                     -1 // IO_PORTA_08
#define TCFG_SPI1_BAUDRATE                  10000000
#define TCFG_SPI1_HD_LEVEL                  0
#define TCFG_SPI1_MODE                      SPI_STD_MODE
#define TCFG_SPI1_ATTR                      SPI_SCLK_L_UPL_SMPH
#define TCFG_SPI1_ROLE                      SPI_MODE_MASTER
#endif // USE_SPI_MASTER_TEST_DEMO

#ifdef USE_SPI_SLAVE_TEST_DEMO
#define TCFG_SPI2_ENABLE                    1
#define TCFG_SPI2_CS_IO                     -1
#define TCFG_SPI2_CLK_IO                    IO_PORTA_04
#define TCFG_SPI2_DO_IO                     IO_PORTA_05
#define TCFG_SPI2_DI_IO                     IO_PORTA_06
#define TCFG_SPI2_D2_IO                     -1 // IO_PORTA_07
#define TCFG_SPI2_D3_IO                     -1 // IO_PORTA_08
#define TCFG_SPI2_BAUDRATE                  10000000
#define TCFG_SPI2_HD_LEVEL                  0
#define TCFG_SPI2_MODE                      SPI_STD_MODE
#define TCFG_SPI2_ATTR                      SPI_SCLK_L_UPL_SMPH
#define TCFG_SPI2_ROLE                      SPI_MODE_SLAVE
#endif // USE_SPI_SLAVE_TEST_DEMO


//*********************************************************************************//
//                                  UART配置                                        //
//*********************************************************************************//
#ifdef USE_UART_DEVICE_TEST_DEMO
#define TCFG_UART2_ENABLE                   1
#define TCFG_UART2_TX_IO                    IO_PORTA_06
#define TCFG_UART2_RX_IO                    IO_PORTA_07
#define TCFG_UART2_BAUDRATE                 1000000
#define TCFG_UART2_FLOW_CTL_ENABLE          1
#define TCFG_UART2_RTS_HW_ENABLE            0
#define TCFG_UART2_RTS_RX_THRESH            80
#define TCFG_UART2_MAX_CONTINUE_RECV_CNT    1024
#define TCFG_UART2_RTS_IO                   IO_PORTA_05
#define TCFG_UART2_CTS_IO                   IO_PORTA_04
#define TCFG_UART2_RTS_IDLE_LEVEL           0
#define TCFG_UART2_CTS_IDLE_LEVEL           0
#define TCFG_UART2_PARITY                   UART_PARITY_DISABLE
#endif // USE_UART_DEVICE_TEST_DEMO


//*********************************************************************************//
//                                  LEDC配置                                        //
//*********************************************************************************//
#ifdef USE_LEDC_TEST_DEMO
#define TCFG_LEDC1_ENABLE                   1
#define TCFG_LEDC1_PORT                     IO_PORTA_04
#define TCFG_LEDC1_IDLE_LEVEL               LEDC_IDLE_LEVEL_L
#define TCFG_LEDC1_OUT_INV                  LEDC_OUT_INV_NREV
#define TCFG_LEDC1_BIT_INV                  LEDC_BIT_INV_8BIT
#define TCFG_LEDC1_TIME_BASE                LEDC_TIME_BASE_42NS
#define TCFG_LEDC1_T1H_CNT                  24
#define TCFG_LEDC1_T1L_CNT                  7
#define TCFG_LEDC1_T0H_CNT                  7
#define TCFG_LEDC1_T0L_CNT                  24
#define TCFG_LEDC1_RST_TIME_CNT             20000
#endif // USE_LEDC_TEST_DEMO


#ifdef CONFIG_RELEASE_ENABLE
#define LIB_DEBUG    0
#else
#define LIB_DEBUG    1
#endif
#define CONFIG_DEBUG_LIB(x)         (x & LIB_DEBUG)


#endif ///< APP_CONFIG_H


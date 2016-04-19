/*
 * *******************************************************************************
 * *   FileName    :    rha_i2c.h
 * *   Description :    i2c 驱动头文件,引脚接线SCL:ENET_RXD0 SDA:CSI0_DAT10

 * *   Author      :    2016-2-29 10:33   by   liulu
 * ********************************************************************************
 * */
#ifndef __RHA_I2C_H_
#define __RHA_I2C_H_
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/mutex.h>
struct rha_gpio{
    unsigned int mux_phy_addr;
    unsigned int pad_phy_addr;
    unsigned int gpio_phy_addr;
    unsigned int gpio_phy_size;
    int gpio_dr_offset;
    int gpio_gdr_offset;
    int gpio_psr_offset;
    int gpio_icr1_offset;
    int gpio_icr2_offset;
    int gpio_imr_offset;
    int gpio_isr_offset;
    int gpio_edge_sel_offset;
    int gpio_nr;
    volatile void *mux;
    volatile void *pad;
    volatile void *gpio_base;
};
struct rha_i2c{
    struct rha_gpio sda;
    struct rha_gpio scl;
    struct miscdevice misc;
    struct mutex mutex;
};
#endif

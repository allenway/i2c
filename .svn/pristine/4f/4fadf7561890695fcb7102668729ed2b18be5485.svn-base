/*
 * *******************************************************************************
 * *   FileName    :    rha_i2c_ioctl.h
 * *   Description :    i2c ioctl

 * *   Author      :    2016-2-29 10:33   by   liulu
 * ********************************************************************************
 * */
#ifndef __RHA_I2C_IOCTL_H_
#define __RHA_I2C_IOCTL_H_
#include<linux/ioctl.h>
struct rha_i2c_data{
    unsigned char addr;
    unsigned char reg;
    unsigned short val;
    int isWord;
};
#define RHA_I2C_TYPE 'R'
#define RHA_I2C_SIZE sizeof(struct rha_i2c_data)
#define RHA_I2C_CMD_R   1
#define RHA_I2C_CMD_W   2
#define RHA_I2C_CTL_R _IOWR(RHA_I2C_TYPE,RHA_I2C_CMD_R,RHA_I2C_SIZE)
#define RHA_I2C_CTL_W _IOWR(RHA_I2C_TYPE,RHA_I2C_CMD_W,RHA_I2C_SIZE)
#endif

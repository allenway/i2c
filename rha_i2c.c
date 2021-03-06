/*
 * *******************************************************************************
 * *   FileName    :    rha_i2c.c
 * *   Description :
 * *   Author      :    2016-2-29 18:34   by   liulu
 * ********************************************************************************
 * */
#include <linux/module.h>     /* 引入与模块相关的宏 */
#include <linux/init.h>        /* 引入module_init() module_exit()函数 */
#include <linux/moduleparam.h> /* 引入module_param() */
#include <linux/workqueue.h>
#include <asm/io.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <linux/fs.h> 
#include <asm/uaccess.h>
#include "rha_i2c.h"
#include "rha_i2c_ioctl.h"
//#define __DEBUG
static struct file_operations i2c_fops; 
static struct rha_i2c rha_i2c_dev[] = {
    {
        .scl = {
            .mux_phy_addr = 0x20E00A0,
            .pad_phy_addr = 0x20E03B4,
            .gpio_phy_addr = 0x20A4000,
            .gpio_phy_size = 0x20,
            .gpio_dr_offset = 0x0,
            .gpio_gdr_offset = 0x4,
            .gpio_psr_offset = 0x8,
            .gpio_icr1_offset = 0xc,
            .gpio_icr2_offset = 0x10,
            .gpio_imr_offset = 0x14,
            .gpio_isr_offset = 0x18,
            .gpio_edge_sel_offset = 0x1c,
            .gpio_nr = 20,
            .mux = NULL,
            .pad = NULL,
            .gpio_base = NULL,
        },
        .sda = {
            .mux_phy_addr = 0x20E01E4,
            .pad_phy_addr = 0x20E04F8,
            .gpio_phy_addr = 0x209C000,
            .gpio_phy_size = 0x20,
            .gpio_dr_offset = 0x0,
            .gpio_gdr_offset = 0x4,
            .gpio_psr_offset = 0x8,
            .gpio_icr1_offset = 0xc,
            .gpio_icr2_offset = 0x10,
            .gpio_imr_offset = 0x14,
            .gpio_isr_offset = 0x18,
            .gpio_edge_sel_offset = 0x1c,
            .gpio_nr = 27,
            .mux = NULL,
            .pad = NULL,
            .gpio_base = NULL,
        },
        .misc = {
            .minor = MISC_DYNAMIC_MINOR,
            .name = "rha_i2c_0",
            .fops = &i2c_fops,
        }
    },
#if 0
    {
        .scl = {
            .mux = NULL,
            .pad = NULL,
            .gpio_base = NULL,
        },
        .sda = {
            .mux = NULL,
            .pad = NULL,
            .gpio_base = NULL,
        },
        .misc = {
            .minor = MISC_DYNAMIC_MINOR,
            .name = "rha_i2c_1",
            .fops = &i2c_fops,
        }
    },
#endif
};
#define RHA_I2C_DEV_NUM  (sizeof(rha_i2c_dev)/(sizeof(struct rha_i2c)))

static int  i2c_read_val(struct rha_i2c *i2c,struct rha_i2c_data *i2c_data);
static int  i2c_write_val(struct rha_i2c *i2c,struct rha_i2c_data *i2c_data);
//i2c misc
static long i2c_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)  
{  
    struct rha_i2c *i2c = filp->private_data;
    struct rha_i2c_data i2c_data;
    int ret = 0;
#ifdef __DEBUG
    printk ("i2c_ioctl:%s\n",i2c->misc.name);
#endif
    switch(_IOC_NR(cmd)){
        case RHA_I2C_CMD_R:
            if(copy_from_user((void *)&i2c_data,(void *)arg,RHA_I2C_SIZE))
            {
                ret = -1;
                break;
            }
            ret = i2c_read_val(i2c,&i2c_data);
            if(!ret)
            {
                 if(copy_to_user((void *)arg,&i2c_data,RHA_I2C_SIZE))
                     ret = -1;
            }
            break;
        case RHA_I2C_CMD_W:
            if(copy_from_user((void *)&i2c_data,(void *)arg,RHA_I2C_SIZE))
            {
                ret = -1;
                break;
            }
            ret = i2c_write_val(i2c,&i2c_data);
            break;
        default:
            ret = -1;
    }
    return ret;  
}  

static int i2c_open(struct inode *inode, struct file *filp)  
{  
    int minor = iminor(inode);
    int i;
    for(i=0;i<RHA_I2C_DEV_NUM;i++)
    {
        if(minor==rha_i2c_dev[i].misc.minor)
        {
            filp->private_data = &rha_i2c_dev[i];
            break;
        }
    }
    if(i<RHA_I2C_DEV_NUM)
    {
#ifdef __DEBUG
        printk("i2c_open %s ok\n",rha_i2c_dev[i].misc.name);
#endif
        return 0;
    }
    else
    {
#ifdef __DEBUG
        printk("i2c_open fail\n");
#endif
        return -1;
    }
}  
static int i2c_release(struct inode *inode, struct file *filp)  
{  
#ifdef __DEBUG
    printk ("i2c_release\n");  
#endif
    return 0;  
}  

static struct file_operations i2c_fops =  
{  
    .owner   = THIS_MODULE,  
    .unlocked_ioctl = i2c_ioctl,
    .open    = i2c_open,  
    .release = i2c_release  
};  
//mode 1 output ,0 input
static inline void gpio_set_mode(struct rha_gpio *gpio,int mode)
{
    unsigned int val;
    val = ioread32(gpio->gpio_base+gpio->gpio_gdr_offset);
    if(mode)  //output
        val |= (0x1<<gpio->gpio_nr); //set output
    else
        val &= ~(0x1<<gpio->gpio_nr); //set input
    iowrite32(val,gpio->gpio_base+gpio->gpio_gdr_offset);

};
static inline void gpio_set_val(struct rha_gpio *gpio,int n)
{
    unsigned int val;
    val = ioread32(gpio->gpio_base+gpio->gpio_dr_offset);
    if(n)  //output 1
        val |= (0x1<<gpio->gpio_nr); //output 1
    else
        val &= ~(0x1<<gpio->gpio_nr); //output 0
    iowrite32(val,gpio->gpio_base+gpio->gpio_dr_offset);
};
static inline int gpio_get_val(struct rha_gpio *gpio)
{
    unsigned int val;
    val = ioread32(gpio->gpio_base+gpio->gpio_dr_offset);
    return (val & (0x1<<gpio->gpio_nr))?1:0;
};
//unit us
#define T_SCL    6 
#define T_SDA    5
static void i2c_start(struct rha_i2c *i2c)
{
    gpio_set_mode(&i2c->scl,1);
    gpio_set_mode(&i2c->sda,1);
    gpio_set_val(&i2c->scl,1);
    gpio_set_val(&i2c->sda,1);
    udelay(T_SCL);
    gpio_set_val(&i2c->sda,0);
    udelay(T_SDA);
};
static void i2c_stop(struct rha_i2c *i2c)
{
    gpio_set_mode(&i2c->scl,1);
    gpio_set_mode(&i2c->sda,1);
    gpio_set_val(&i2c->scl,1);
    gpio_set_val(&i2c->sda,0);
    udelay(T_SCL);
    gpio_set_val(&i2c->sda,1);
    udelay(T_SDA);
};
static void i2c_read(struct rha_i2c *i2c,unsigned char *n)
{
    int i;
    *n = 0;
    //read 8bit data
    gpio_set_mode(&i2c->scl,1);
    gpio_set_mode(&i2c->sda,0);
    for(i=7;i>=0;i--)
    {
        gpio_set_val(&i2c->scl,0);
        udelay(T_SCL);
        
        gpio_set_val(&i2c->scl,1);
        udelay(T_SCL);
        
        *n |= gpio_get_val(&i2c->sda)<<i;
    }
    //send ack
    gpio_set_val(&i2c->scl,0);
    udelay(T_SCL);

    gpio_set_mode(&i2c->sda,1);
    gpio_set_val(&i2c->sda,0);
    udelay(T_SDA);

    gpio_set_val(&i2c->scl,1);
    udelay(T_SCL);
};
static int i2c_write(struct rha_i2c *i2c,unsigned char n)
{
    int i;
    //write 8bit data
    gpio_set_mode(&i2c->scl,1);
    gpio_set_mode(&i2c->sda,1);
    for(i=7;i>=0;i--)
    {
        gpio_set_val(&i2c->scl,0);
        udelay(T_SCL);
        
        gpio_set_val(&i2c->sda,0x1&(n>>i));
        udelay(T_SDA);
        
        gpio_set_val(&i2c->scl,1);
        udelay(T_SCL);
    }
    //check ack
    gpio_set_val(&i2c->scl,0);
    udelay(T_SCL);
    gpio_set_val(&i2c->sda,0);
    gpio_set_mode(&i2c->sda,0);
    udelay(T_SCL);
    gpio_set_val(&i2c->scl,1);
    udelay(T_SCL); 

    return gpio_get_val(&i2c->sda);
};
static int  i2c_read_val(struct rha_i2c *i2c,struct rha_i2c_data *i2c_data)
{
    int ret;
    unsigned char val;
#ifdef __DEBUG
    printk ("%s addr:%x reg:%x isWord:%d\n",__func__,i2c_data->addr,i2c_data->reg,i2c_data->isWord);
#endif
    mutex_lock(&i2c->mutex);
    i2c_start(i2c);
    ret = i2c_write(i2c,i2c_data->addr&(~0x1));
    if(ret)
    {
#ifdef __DEBUG
    printk("%s error at line %d\n",__func__,__LINE__);
#endif 
        goto OUT;
    }
    ret = i2c_write(i2c,i2c_data->reg);
    if(ret)
    {
#ifdef __DEBUG
    printk("%s error at line %d\n",__func__,__LINE__);
#endif 
        goto OUT;
    }
    i2c_start(i2c);
    ret = i2c_write(i2c,i2c_data->addr|0x1);
    if(ret)
    {
#ifdef __DEBUG
    printk("%s error at line %d\n",__func__,__LINE__);
#endif 
        goto OUT;
    }
    i2c_read(i2c,&val);
    i2c_data->val = val;
    if(i2c_data->isWord)
    {
        i2c_read(i2c,&val);
        i2c_data->val |= val<<8;
    }
    i2c_stop(i2c);
OUT:
    mutex_unlock(&i2c->mutex);
#ifdef __DEBUG
    printk("%s %s\n",__func__,ret?"fail":"ok");
#endif 
    return ret;
}
//
static int  i2c_write_val(struct rha_i2c *i2c,struct rha_i2c_data *i2c_data)
{
    int ret;
#ifdef __DEBUG
    printk ("%s addr:%x reg:%x val:%x isWord:%d\n",__func__,i2c_data->addr,i2c_data->reg,i2c_data->val,i2c_data->isWord);
#endif 
    mutex_lock(&i2c->mutex);
    i2c_start(i2c);
    ret = i2c_write(i2c,i2c_data->addr&(~0x1));
    if(ret)
    {
#ifdef __DEBUG
    printk("%s error at line %d\n",__func__,__LINE__);
#endif 
        goto OUT;
    }
    ret = i2c_write(i2c,i2c_data->reg);
    if(ret)
    {
#ifdef __DEBUG
    printk("%s error at line %d\n",__func__,__LINE__);
#endif 
        goto OUT;
    }
    ret = i2c_write(i2c,i2c_data->val&0xff);
    if(ret)
    {
#ifdef __DEBUG
    printk("%s error at line %d\n",__func__,__LINE__);
#endif 
        goto OUT;
    }
    if(i2c_data->isWord)
    {
        ret = i2c_write(i2c,(i2c_data->val>>8)&0xff);
        if(ret)
        {
#ifdef __DEBUG
            printk("%s error at line %d\n",__func__,__LINE__);
#endif 
            goto OUT;
        }

    }
    i2c_stop(i2c);
OUT:
    mutex_unlock(&i2c->mutex);
#ifdef __DEBUG
    printk("%s %s\n",__func__,ret?"fail":"ok");
#endif 
    return ret;
}

static int __init rha_gpio_init(struct rha_gpio *gpio)
{
    volatile unsigned int val;
    printk(KERN_INFO"%s entery\n",__func__);
    //1.初始化HW
    //ioremap
    gpio->mux = ioremap(gpio->mux_phy_addr,4);
    if(gpio->mux==NULL)
    {
        printk(KERN_INFO"%s ioremap for gpio->mux_phy_addr failed",__func__);
        goto GPIO_ERR0;
    }
    gpio->pad = ioremap(gpio->pad_phy_addr,4);
    if(gpio->pad==NULL)
    {
        printk(KERN_INFO"%s ioremap for gpio->pad_phy_addr failed ",__func__);
        goto GPIO_ERR1;
    }
    gpio->gpio_base = ioremap(gpio->gpio_phy_addr,gpio->gpio_phy_size);
    if(gpio->gpio_base==NULL)
    {
        printk(KERN_INFO"%s ioremap for gpio->gpio_phy_addr failed",__func__);
        goto GPIO_ERR2;
    }

    //初始化iomux as gpio mode
    val = ioread32(gpio->mux);
    val = (val & (~0x17)) | 0x5;  //set gpio mode
    iowrite32(val,gpio->mux);
    val = ioread32(gpio->pad);
    val = (val & (~0x1f8f9)) | 0xb0b0;  //pull up
    iowrite32(val,gpio->pad);

    //初始化gpio
    val = ioread32(gpio->gpio_base+gpio->gpio_dr_offset);
    val |= (0x1<<gpio->gpio_nr); //output1
    iowrite32(val,gpio->gpio_base+gpio->gpio_dr_offset);
    
    val = ioread32(gpio->gpio_base+gpio->gpio_gdr_offset);
    val |= (0x1<<gpio->gpio_nr); //set output
    iowrite32(val,gpio->gpio_base+gpio->gpio_gdr_offset);

    printk(KERN_INFO"%s ok\n",__func__);
    return 0;
GPIO_ERR2:
    iounmap(gpio->pad);
GPIO_ERR1:
    iounmap(gpio->mux);
GPIO_ERR0:
    printk(KERN_INFO"%s failed\n",__func__);
    return -1;
}
static void __exit rha_gpio_uninit(struct rha_gpio *gpio)
{
    //4.反初始化HW
    iounmap(gpio->mux);
    iounmap(gpio->pad);
    iounmap(gpio->gpio_base);
    return;
}
static int __init rha_i2c_dev_init(struct rha_i2c *i2c)
{
    int ret;
    //gpio init
    ret = rha_gpio_init(&i2c->scl);
    if(ret)
    {
        printk ("rha_gpio_init scl fail\n");
        goto SCL_ERR;
    }
    else
        printk ("rha_gpio_init scl ok\n");
    ret = rha_gpio_init(&i2c->sda);
    if(ret)
    {
        printk ("rha_gpio_init sda fail\n");
        goto SDA_ERR;
    }
    else
        printk ("rha_gpio_init sda ok\n");
    //register misc dev
    ret = misc_register(&i2c->misc);  
    if(ret)
    {
        printk ("misc_register fail\n");
        goto MISC_ERR;
    }
    else
        printk ("misc_register ok\n");
    mutex_init(&i2c->mutex);
    return 0;
MISC_ERR:
    rha_gpio_uninit(&i2c->sda);
SDA_ERR:
    rha_gpio_uninit(&i2c->scl);
SCL_ERR:
    return ret;
}
static void __exit rha_i2c_dev_uninit(struct rha_i2c *i2c)
{
    mutex_destroy(&i2c->mutex);
    misc_deregister(&i2c->misc);
    rha_gpio_uninit(&i2c->scl);
    rha_gpio_uninit(&i2c->sda);
}
static int __init rha_i2c_init(void)
{
    int ret = -1;
    int i;
    printk(KERN_INFO"%s entery\n",__func__);
    for(i=0;i<RHA_I2C_DEV_NUM;i++)
    {
        ret = rha_i2c_dev_init(&rha_i2c_dev[i]);
        if(ret)
        {
            printk ("rha_i2c_init failed: name %s\n",rha_i2c_dev[i].misc.name);
            while(i){
                i--;
                rha_i2c_dev_uninit(&rha_i2c_dev[i]);
            }
            break;
        }
        else
            printk ("rha_i2c_init ok: name %s\n",rha_i2c_dev[i].misc.name);
    }
    printk(KERN_INFO"%s %s\n",__func__,ret==0?"ok":"fail");
    return ret;
}

static void __exit rha_i2c_exit(void)
{
    int i;
    printk(KERN_INFO"%s entery\n",__func__);
    for(i=0;i<RHA_I2C_DEV_NUM;i++)
                rha_i2c_dev_uninit(&rha_i2c_dev[i]);
    printk(KERN_INFO"%s ok\n",__func__);
}
module_init(rha_i2c_init);
module_exit(rha_i2c_exit);
MODULE_AUTHOR("RHA,Liu Lu");
MODULE_LICENSE("GPL"); 
MODULE_DESCRIPTION("i2c driver for RHA PIS");


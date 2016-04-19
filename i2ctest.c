#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "rha_i2c_ioctl.h"
int main (int argc,char *argv[])  
{  
    int fd;
    int ret;
    struct rha_i2c_data data;
    unsigned int val;
    if(argc<5)
    {
        printf("Usage:\n");
        printf("read 8bit val:%s </dev/rha_i2c_X> <addr Hex> <r> <reg Hex> \n",argv[0]);
        printf("read 16bit val:%s </dev/rha_i2c_X> <addr Hex> <R> <reg Hex> \n",argv[0]);
        printf("write 8bit val:%s </dev/rha_i2c_X> <addr Hex> <w> <reg Hex> <val Hex>\n",argv[0]);
        printf("write 16bit val:%s </dev/rha_i2c_X> <addr Hex> <W> <reg Hex> <val Hex>\n",argv[0]);
        return -1;
    }
    fd = open (argv[1], O_RDONLY);  
    if (fd <= 0)  
    {  
        printf ("open %s device error!\n",argv[1]);  
        return -1;  
    }
    else
        printf("open %s device successful!\n",argv[1]);
    data.isWord = 0; 
    switch(argv[3][0]){
        case 'R':
            data.isWord = 1;
        case 'r':
            if(argc!=5)
            {
                printf("Invaild parameter\n");
                return -1;
            }
            sscanf(argv[2],"%x",&val);
            data.addr = val;
            sscanf(argv[4],"%x",&val);
            data.reg = val;
            printf("read val:addr %x reg %x %dbit\n",data.addr,data.reg,data.isWord?16:8);
            ret = ioctl(fd,RHA_I2C_CTL_R,&data);
            if(ret)
                printf ("ioctl read error!\n");  
            else
                printf ("ioctl read val:%x\n",data.val);  
            break;
        case 'W':
            data.isWord = 1;
        case 'w':
            if(argc!=6)
            {
                printf("Invaild parameter\n");
                return -1;
            }
            sscanf(argv[2],"%x",&val);
            data.addr = val;
            sscanf(argv[4],"%x",&val);
            data.reg = val;
            sscanf(argv[5],"%x",&val);
            data.val = val;
            printf("write val:addr %x reg %x %dbit\n",data.addr,data.reg,data.isWord?16:8);
            ret = ioctl(fd,RHA_I2C_CTL_W,&data);
            if(ret)
                printf ("ioctl write error!\n");  
            else
                printf ("ioctl write ok!\n");  
            break;
        default:
                printf("Invaild parameter\n");
                return -1;
    }
    close (fd);  
    return 0;  
}  



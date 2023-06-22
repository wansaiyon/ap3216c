/*
********************************************************************************
  *Copyright (C), 2020, Consys All Rights Reserved
  *文 件 名: ap3216c.c
  *说    明: ap3216c测试程序
  *版    本: V000B00D00
  *创建日期: 2023/06/20		14:42:15
  *创 建 人: wansaiyon
  *修改信息：  
================================================================================
  *修 改 人    		  修改日期       	  修改内容 
  *<作者/修改者>      <YYYY/MM/DD>		  <修改内容>
********************************************************************************
*/
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#define I2C_BUS "/dev/i2c-1"   // I2C 总线设备节点
#define AP3216C_I2C_ADDR 0x1E  // 第一个 AP3216C 设备地址

// 定义 AP3216C 寄存器地址
#define AP3216C_REG_SYSCONFIG    0x00
#define AP3216C_REG_IRDATA_LO    0x0A
#define AP3216C_REG_IRDATA_HI    0x0B
#define AP3216C_REG_ALSDATA_LO   0x0C
#define AP3216C_REG_ALSDATA_HI   0x0D
#define AP3216C_REG_PSDATA_LO    0x0E
#define AP3216C_REG_PSDATA_HI    0x0F


struct ap3216_data
{
	unsigned short ir;
	unsigned short als;
	unsigned short ps;
};

static int i2c_read_reg(int fd, unsigned char reg_addr, unsigned char *data)
{
    if (write(fd, &reg_addr, 1) != 1)
    {
        printf("write failed\n");
        return -1;
    }
    if (read(fd, &data, 1) != 1)
    {
        printf("read failed\n");
        return -1;
    }
    return 0;
}

static int i2c_write_reg(int fd, unsigned char reg_addr, unsigned char value)
{
    unsigned char buffer[2];
    buffer[0] = reg_addr;
    buffer[1] = value;

    if (write(fd, &buffer, 2) != 2)
    {
        printf("write failed\n");
        return -1;
    }
    return 0;
}

int ap3216c_init(void)
{
	int i2c_fd;
	unsigned char buffer[2];
    char i2c_bus[] = I2C_BUS;
    unsigned char device_address = AP3216C_I2C_ADDR;
	
	// 打开 I2C 总线设备节点
    i2c_fd = open(i2c_bus, O_RDWR);
    if (i2c_fd < 0) {
        printf("无法打开 I2C 总线设备\n");
        return 1;
    }

    // 设置 I2C 设备地址
    if (ioctl(i2c_fd, I2C_SLAVE, device_address) < 0) {
        printf("无法连接到 AP3216 设备\n");
        return 1;
    }
	
	/*
		000: Power down (Default)
		001: ALS function active
		010: PS+IR function active
		011: ALS and PS+IR functions active
		100: SW reset
		101: ALS function once
		110: PS+IR function once
		111: ALS and PS+IR functions once
	*/
	buffer[0] = AP3216C_REG_SYSCONFIG;
	buffer[1] = 0b100; //SW reset
	
	// 写入初始配置
    if (i2c_write_reg(i2c_fd, buffer[0], buffer[1]) != 0)
    {
        printf("写入初始配置数据时出错\n");
        return 1;
    }
	
	usleep(50000);
	
	buffer[0] = AP3216C_REG_SYSCONFIG;
	buffer[1] = 0b011; //ALS and PS+IR functions active
	
	// 写入初始配置
    if (i2c_write_reg(i2c_fd, buffer[0], buffer[1]) != 0)
    {
        printf("写入初始配置数据时出错\n");
        return 1;
    }

    // 关闭 I2C 总线设备
    close(i2c_fd);

    return 0;
}	


int ap3216c_read_data(struct ap3216_data *data) 
{
    int i2c_fd;
	int i = 0;
    char i2c_bus[] = I2C_BUS;
    unsigned char buf[1+6];
    unsigned char device_address = AP3216C_I2C_ADDR;

    // 打开 I2C 总线设备节点
    i2c_fd = open(i2c_bus, O_RDWR);
    if (i2c_fd < 0) {
        printf("无法打开 I2C 总线设备\n");
        return -1;
    }

    // 设置 I2C 设备地址
    if (ioctl(i2c_fd, I2C_SLAVE, device_address) < 0) {
        printf("无法连接到 AP3216 设备\n");
        return -1;
    }   
	
	// 读取寄存器的值
	buf[0] = AP3216C_REG_IRDATA_LO;
	for(i = 0; i< 6; i++)
	{
		if (i2c_read_reg(i2c_fd, buf[0] + i, &buf[1+i]) != 0)
		{
			printf("读取输入寄存器时出错\n");
			return -1;
		}
	}
	
	if(buf[1] & 0X80) /* IR_OF 位为 1,则数据无效 */
		data->ir = 0;
	else 				 /* 读取 IR 传感器的数据 */
		data->ir = ((unsigned short)buf[2] << 2) | (buf[1] & 0X03);

	data->als = ((unsigned short)buf[4] << 8) | buf[3];/* ALS 数据 */

	if(buf[5] & 0x40) 	/* IR_OF 位为 1,则数据无效 */
		data->ps = 0;
	else 				/* 读取 PS 传感器的数据 */
		data->ps = ((unsigned short)(buf[6] & 0X3F) << 4) |
									(buf[5] & 0X0F);    

    // 关闭 I2C 总线设备
    close(i2c_fd);

    return 0;
}

int main (int argc, char *argv[]) 
{
    int opt;
	int i = 0;
    unsigned int cnt = 0;	
	struct ap3216_data *data;
	
	/* initial */
    if(ap3216c_init() == 0)
	{
		printf("ap3216 init success.\n");
	}
	else
	{
		printf("ap3216 init failed.\n");
	}		

    while ((opt = getopt(argc, argv, "c:h")) != -1)
    {
        switch (opt)
        {        
        case 'c':
            cnt = atoi(optarg);
			
			for(i = 0; i< cnt; i++)
			{			
				if (ap3216c_read_data(data) == 0)
				{
					printf("%u ap3216 ir: %u als: %u ps: %u\n", i, data->ir, data->als, data->ps);
				}
				else
				{
					printf("ap3216 read failed\n");
				}
				usleep(150000);
			}
            break;
        case 'h':
            printf("帮助信息         :\n");
            printf("-c <cnt>         :读取cnt次\n");
            printf("-h               :显示帮助信息\n");
            return 0;
        default:
            printf("unknow\n");
            return 1;
        }
    }

    return 0;
}

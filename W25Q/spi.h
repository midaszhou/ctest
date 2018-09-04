/*-------------------------------------------------
Base on:
        www.cnblogs.com/subo_peng/p/4848260.html
        Author: lzy

-------------------------------------------------*/
#ifndef __SPI_H_
#define __SPI_H_

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

//----- debug print
#define pr_debug printf
#define pr_err printf

//----- turn off 
#define SPI_DEBUG 0


extern const char *str_spi_device;
extern uint8_t spi_mode;
extern uint8_t spi_bits; // ８ｂiｔｓ读写，MSB first
extern uint32_t spi_speed;// min.1M 设置传输速度 */
extern uint16_t delay;
extern int g_SPI_Fd; //SPI device file descriptor


//----- FUCNTION DECLARATION -----
void pabort(const char *s);
int SPI_Transfer( const uint8_t *TxBuf,  uint8_t *RxBuf, int len,int ns);
int SPI_Write_then_Read(const uint8_t *TxBuf, int n_tx, uint8_t *RxBuf, int n_rx);
int SPI_Write_then_Write(const uint8_t *TxBuf1, int n_tx1, uint8_t *TxBuf2, int n_tx2);
int SPI_Write(const uint8_t *TxBuf, int len);
int SPI_Read(uint8_t *RxBuf, int len);
int SPI_Open(void);
int SPI_Close(void);
int SPI_LookBackTest(void);


#endif

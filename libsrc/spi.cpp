#define __SPI_LIB

//#include <math.h>

#define  USE_COMMON
#include "common.h"
#include "io.h"
#include "spi.h"
void spi_EnableCS(void);
void spi_DisableCS(void);

static unsigned char SPI_CLKMODE[15] =
    {
        7,  //21.4Mbps; assume RoBoard's DRAM clock is 300MHZ
        8,  //18.75Mbps
        10, //15Mbps
        12, //12.5Mbps
        15, //10Mbps
        14, //10.7Mbps
        13, //11.5Mbps
        11, //13.6Mbps
        9,  //16.6Mbps
        6,  //25Mbps
        5,  //30Mbps
        4,  //37Mbps
        3,  //50Mbps
        2,  //75Mbps
        1   //150Mbps
    };

static unsigned long OLD_SPIGPIO3FLAG = 0xffffffffL;

#define SB_MULTIFUNC_REG      (0xc0)
/************** South Bridge Multifunction Pin Control REG: 0xC0~0xC3 ************
 *    To use SPI interface, the user should set PINS0 (bit 0 of SB_MULTIFUNC_REG)
 *    to 1. (PINS0 = 1: SPI; PINS0 = 0: GPIO3[3:0])
 *********************************************************************************/

static int SPI_ioSection = -1;
RBAPI(bool) spi_InUse(void) {
    if (SPI_ioSection == -1) return false; else return true;
}

RBAPI(bool) spi_Initialize(int clkmode) {
    unsigned baseaddr;
    int i;
    
    if (SPI_ioSection != -1)
	{
        err_SetMsg(ERROR_SPI_INUSE, "SPI lib was already opened");
		return false;
	}
	
	if ((SPI_ioSection = io_Init()) == -1) return false;

    baseaddr = spi_SetDefaultBaseAddress();
    if ((baseaddr == 0x0000) || (baseaddr == 0xffff)) spi_SetBaseAddress(0xfc00);

    spi_DisableCS();
    spi_ClearErrors();

    //enable FIFO and set clock divisor
    if (spi_SetControlREG(0x10 + SPI_CLKMODE[clkmode]) == false)
    {
        spi_Close();
        err_SetMsg(ERROR_SPI_INITFAIL, "fail to write SPI Control Register");
        return false;
    }
    
    //clear the input buffer if it is not empty
    for (i=0; i<20; i++)
    {
        if (spi_InputReady() == false) break;
        if (spi_Read() == 0xffff)
        {
            spi_Close();
            err_SetMsg(ERROR_SPI_INITFAIL, "fail to clear SPI input buffer");
            return false;
        }
    }

    //switch GPIO3[3:0] to external SPI interface
    OLD_SPIGPIO3FLAG = read_sb_reg(SB_MULTIFUNC_REG);
    write_sb_reg(SB_MULTIFUNC_REG, OLD_SPIGPIO3FLAG | 1L);
    OLD_SPIGPIO3FLAG = OLD_SPIGPIO3FLAG & 1L;

    return true;
}

RBAPI(void) spi_Close(void) {
	if (SPI_ioSection == -1) return;

    spi_DisableCS();
    spi_ClearErrors();

    //restore GPIO3[3:0]/external SPI switch setting
    if (OLD_SPIGPIO3FLAG != 0xffffffffL)
        write_sb_reg(SB_MULTIFUNC_REG, (read_sb_reg(SB_MULTIFUNC_REG) & 0xfffffffeL) + OLD_SPIGPIO3FLAG);

	io_Close(SPI_ioSection);
	SPI_ioSection = -1;
	OLD_SPIGPIO3FLAG = 0xffffffffL;
}


//use GPIO port 3 pin 7 to simulate SPI_SS (on RoBoard)
#define GPIO3_DATA   (0x7b)
#define GPIO3_DIR    (0x9b)
RBAPI(bool) spi_EnableSS(void) {
    //avoid enable-after-write timing issue
    if (spi_OutputFIFOEmpty() == false) //&&
    if (spi_FIFOFlush() == false)
        return false;

	io_outpb(GPIO3_DIR, io_inpb(GPIO3_DIR) | 0x80);
    io_outpb(GPIO3_DATA, io_inpb(GPIO3_DATA) & 0x7f);
    return true;
}

RBAPI(bool) spi_DisableSS(void) {
    //avoid disable-after-write timing issue
    if (spi_OutputFIFOEmpty() == false) //&&
    if (spi_FIFOFlush() == false)
        return false;

	io_outpb(GPIO3_DIR, io_inpb(GPIO3_DIR) | 0x80);
    io_outpb(GPIO3_DATA, io_inpb(GPIO3_DATA) | 0x80);
    return true;
}


/*
//set SPI CLK to spiclk MHZ; return false if spiclk given by the user is not supported
double ROBOARD_DRAMClock = 300.0; //300MHZ
bool spi_SetClock(double spiclk) {
    unsigned char clkdiv;

    clkdiv = (unsigned char) ceil(ROBOARD_DRAMClock / (2.0 * spiclk));
	if (clkdiv > 1) //&&
	if ((ROBOARD_DRAMClock / (2.0 * clkdiv) - spiclk) > (spiclk - ROBOARD_DRAMClock / (2.0 * (clkdiv - 1))))
		clkdiv = clkdiv - 1;

	return spi_SetClockDivisor(clkdiv);
}
*/


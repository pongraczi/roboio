#include <stdio.h>
// #include <conio.h>
#include <stdlib.h>

#define USE_COMMON
#include "dxio.h"
//#include "common.h"
//#include "i2c.h"

bool i2ctest(void);

int main(void) { //(int argc, char* argv[]) {
    if (i2c_Initialize2(I2C_USEMODULE0 + I2C_USEMODULE1, I2CIRQ_DISABLE, I2CIRQ_DISABLE) == false)
    {
        printf("ERROR: fail to initialize I2C (%s)!\n", roboio_GetErrMsg());
        return 1;
    }

    i2c0slave_SetAddr(0x55);
    i2c1slave_SetAddr(0x54);

    i2c0_SetSpeed(I2CMODE_AUTO, 100000L);
    i2c0_SetSpeed(I2CMODE_AUTO, 100000L);
    printf("Testing I2C standard mode in 100 Kbps...\n");
    if (i2ctest() == false)
    {
        printf("\nThe I2C modules fail in this test!\n");
        return 1;
    }
    printf("Pass.\n\n");

    i2c0_SetSpeed(I2CMODE_AUTO, 400000L);
    i2c0_SetSpeed(I2CMODE_AUTO, 400000L);
    printf("Testing I2C fast mode in 400 Kbps...\n");
    if (i2ctest() == false)
    {
        printf("\nThe I2C modules fail in this test!\n");
        return 1;
    }
    printf("Pass.\n\n");

    printf("The I2C modules work correctly.\n");
    i2c_Close();
    return 0;
}


#define NUMBYTES        (1000)
#define TEST_TIMEOUT    (3000L)
bool i2ctest(void) {
    unsigned long nowtime;
    unsigned val;
    int i;

    printf("    I2C0 as master, I2C1 as slave; master writes to slave\n");
    if (i2c0master_Start(0x54, I2C_WRITE) == false)
    {
        printf("    ERROR: master fails to start (%s)!\n", roboio_GetErrMsg());
        return false;
    }
    
    i2c0master_SetRestart(0x54, I2C_READ);

    for (i=0; i<NUMBYTES; i++)
    {
        if (i < NUMBYTES - 1)
        {
            if (i2c0master_Write(0x55) == false)
            {
                printf("    ERROR: master fails to write %dth byte (%s)!\n", i, roboio_GetErrMsg());
                return false;
            }
        }
        else
        if (i2c0master_WriteLast(0x55) == false)
        {
            printf("    ERROR: master fails to write the last byte and stop (%s)!\n", roboio_GetErrMsg());
            return false;
        }

        nowtime = timer_nowtime();
        while ((val = i2c1slave_Listen()) != I2CSLAVE_READREQUEST)
        {
            if (val == 0xffff)
            {
                printf("    ERROR: slave fails when listening %dth byte (%s)!\n", i, roboio_GetErrMsg());
                return false;
            }
            if ((timer_nowtime() - nowtime) > TEST_TIMEOUT)
            {
                printf("    ERROR: slave don't respond in %dth byte (slave status REG: 0x%x)!\n", i, i2c1_ReadStatREG());
                return false;
            }
        }
        
        val = i2c1slave_Read();
        if (val == 0xffff)
        {
            printf("    ERROR: slave fails to read (%s)!\n", roboio_GetErrMsg());
            return false;
        }
        if (val != 0x55)
        {
            printf("    ERROR: slave gets wrong data (0x%x) in %dth byte!\n", val, i);
            return false;
        }
    }


    ///////////////////////////////////////////////////////////////////////////////
    printf("    I2C0 as master, I2C1 as slave; master reads from slave\n");
    i2c0master_SetRestart(0x54, I2C_READ);

    for (i=0; i<NUMBYTES; i++)
    {
        if (i > 0)
        {
            nowtime = timer_nowtime();
            while (i2c1slave_CheckSlaveWREQ() == false)
                if ((timer_nowtime() - nowtime) > TEST_TIMEOUT)
                {
                    printf("    ERROR: slave fails to wait write-request of the %dth byte!\n", i);
                    return false;
                }
        }

        i2c1slave_ClearSlaveWREQ();
        i2c1_WriteDataREG(0x55);

        if (i == NUMBYTES - 1)
            val = i2c0master_ReadLast();
        else
        if (i == NUMBYTES - 2)
            val = i2c0master_ReadSecondLast();
        else
            val = i2c0master_Read();

        if (val == 0xffff)
        {
            printf("    ERROR: master fails to read %dth byte (%s)!\n", i, roboio_GetErrMsg());
            return false;
        }
        if (val != 0x55)
        {
            printf("    ERROR: master gets wrong data (0x%x) in %dth byte!\n", val, i);
            return false;
        }

        nowtime = timer_nowtime();
        while (i2c1_CheckTXDone() == false)
            if ((timer_nowtime() - nowtime) > TEST_TIMEOUT)
            {
                printf("    ERROR: slave don't transmit the %dth byte successly!\n", i);
                return false;
            }
        i2c1_ClearTXDone();
    }
    i2c1slave_ClearSlaveWREQ();


    ///////////////////////////////////////////////////////////////////////////////
    printf("    I2C0 as master, I2C1 as slave; master reads from slave\n");
    i2c0master_SetRestart(0x54, I2C_WRITE);

    for (i=0; i<NUMBYTES; i++)
    {
        if (i > 0)
        {
            nowtime = timer_nowtime();
            while (i2c1slave_CheckSlaveWREQ() == false)
                if ((timer_nowtime() - nowtime) > TEST_TIMEOUT)
                {
                    printf("    ERROR: slave fails to wait write-request of the %dth byte!\n", i);
                    return false;
                }
        }

        i2c1slave_ClearSlaveWREQ();
        i2c1_WriteDataREG(0x55);

        if (i == NUMBYTES - 1)
            val = i2c0master_ReadLast();
        else
        if (i == NUMBYTES - 2)
            val = i2c0master_ReadSecondLast();
        else
            val = i2c0master_Read();

        if (val == 0xffff)
        {
            printf("    ERROR: master fails to read %dth byte (%s)!\n", i, roboio_GetErrMsg());
            return false;
        }
        if (val != 0x55)
        {
            printf("    ERROR: master gets wrong data (0x%x) in %dth byte!\n", val, i);
            return false;
        }

        nowtime = timer_nowtime();
        while (i2c1_CheckTXDone() == false)
            if ((timer_nowtime() - nowtime) > TEST_TIMEOUT)
            {
                printf("    ERROR: slave don't transmit the %dth byte successly!\n", i);
                return false;
            }
        i2c1_ClearTXDone();
    }
    i2c1slave_ClearSlaveWREQ();


    ///////////////////////////////////////////////////////////////////////////////
    printf("    I2C0 as master, I2C1 as slave; master writes to slave\n");

    for (i=0; i<NUMBYTES; i++)
    {
        if (i < NUMBYTES - 1)
        {
            if (i2c0master_Write(0x55) == false)
            {
                printf("    ERROR: master fails to write %dth byte (%s)!\n", i, roboio_GetErrMsg());
                return false;
            }
        }
        else
        if (i2c0master_WriteLast(0x55) == false)
        {
            printf("    ERROR: master fails to write the last byte and stop (%s)!\n", roboio_GetErrMsg());
            return false;
        }

        nowtime = timer_nowtime();
        while ((val = i2c1slave_Listen()) != I2CSLAVE_READREQUEST)
        {
            if (val == 0xffff)
            {
                printf("    ERROR: slave fails when listening %dth byte (%s)!\n", i, roboio_GetErrMsg());
                return false;
            }
            if ((timer_nowtime() - nowtime) > TEST_TIMEOUT)
            {
                printf("    ERROR: slave don't respond in %dth byte (slave status REG: 0x%x)!\n", i, i2c1_ReadStatREG());
                return false;
            }
        }
        
        val = i2c1slave_Read();
        if (val == 0xffff)
        {
            printf("    ERROR: slave fails to read (%s)!\n", roboio_GetErrMsg());
            return false;
        }
        if (val != 0x55)
        {
            printf("    ERROR: slave gets wrong data (0x%x) in %dth byte!\n", val, i);
            return false;
        }
    }
    nowtime = timer_nowtime();
    while (i2c1slave_Listen() != I2CSLAVE_END)
        if ((timer_nowtime() - nowtime) > TEST_TIMEOUT)
        {
            printf("    ERROR: slave don't receive END!\n");
            return false;
        }

    
    ///////////////////////////////////////////////////////////////////////////////
    printf("    I2C1 as master, I2C0 as slave; master writes to slave\n");
    if (i2c1master_Start(0x55, I2C_WRITE) == false)
    {
        printf("    ERROR: master fails to start (%s)!\n", roboio_GetErrMsg());
        return false;
    }

    //i2c1master_SetRestart(0x55, I2C_READ);

    for (i=0; i<NUMBYTES; i++)
    {
        if (i < NUMBYTES - 1)
        {
            if (i2c1master_Write(0xaa) == false)
            {
                printf("    ERROR: master fails to write %dth byte (%s)!\n", i, roboio_GetErrMsg());
                return false;
            }
        }
        else
        if (i2c1master_WriteLast(0xaa) == false)
        {
            printf("    ERROR: master fails to write the last byte and stop (%s)!\n", roboio_GetErrMsg());
            return false;
        }

        nowtime = timer_nowtime();
        while ((val = i2c0slave_Listen()) != I2CSLAVE_READREQUEST)
        {
            if (val == 0xffff)
            {
                printf("    ERROR: slave fails when listening %dth byte (%s)!\n", i, roboio_GetErrMsg());
                return false;
            }
            if ((timer_nowtime() - nowtime) > TEST_TIMEOUT)
            {
                printf("    ERROR: slave don't respond in %dth byte (slave status REG: 0x%x)!\n", i, i2c0_ReadStatREG());
                return false;
            }
        }
        
        val = i2c0slave_Read();
        if (val == 0xffff)
        {
            printf("    ERROR: slave fails to read (%s)!\n", roboio_GetErrMsg());
            return false;
        }
        if (val != 0xaa)
        {
            printf("    ERROR: slave gets wrong data (0x%x) in %dth byte!\n", val, i);
            return false;
        }
    }
    nowtime = timer_nowtime();
    while (i2c0slave_Listen() != I2CSLAVE_END)
        if ((timer_nowtime() - nowtime) > TEST_TIMEOUT)
        {
            printf("    ERROR: slave don't receive END!\n");
            return false;
        }


    ///////////////////////////////////////////////////////////////////////////////
    printf("    I2C1 as master, I2C0 as slave; master reads from slave\n");
    i2c1master_WriteAddrREG(0x55, I2C_READ);
    nowtime = timer_nowtime();
    while (i2c1_CheckTXDone() == false)
        if ((timer_nowtime() - nowtime) > TEST_TIMEOUT)
        {
            printf("    ERROR: master fails to start!\n");
            return false;
        }
    i2c1_ClearTXDone();

    i2c1_ReadDataREG();
    for (i=0; i<NUMBYTES; i++)
    {
        nowtime = timer_nowtime();
        while ((val = i2c0slave_Listen()) != I2CSLAVE_WRITEREQUEST)
        {
            if (val == 0xffff)
            {
                printf("    ERROR: slave fails when listening to write %dth byte (%s)!\n", i, roboio_GetErrMsg());
                return false;
            }
            if ((timer_nowtime() - nowtime) > TEST_TIMEOUT)
            {
                printf("    ERROR: slave don't respond in %dth byte (slave status REG: 0x%x)!\n", i, i2c0_ReadStatREG());
                return false;
            }
        }
        i2c0slave_Write(0xaa);

        nowtime = timer_nowtime();
        while (i2c1_CheckRXRdy() == false)
            if ((timer_nowtime() - nowtime) > TEST_TIMEOUT)
            {
                printf("    ERROR: master fails to read %dth byte!\n", i);
                return false;
            }

        if (i == NUMBYTES - 2) i2c1master_SetStopBit();
        if (i2c1_ReadDataREG() != 0xaa)
        {
            printf("    ERROR: master gets wrong data (0x%x) in %dth byte!\n", val, i);
            return false;
        }
    }
    nowtime = timer_nowtime();
    while (i2c0slave_Listen() != I2CSLAVE_END)
        if ((timer_nowtime() - nowtime) > TEST_TIMEOUT)
        {
            printf("    ERROR: slave don't receive END!\n");
            return false;
        }
    
    i2c0_ClearSTAT(I2CSTAT_ALL);
    i2c1_ClearSTAT(I2CSTAT_ALL);
    return true;
}


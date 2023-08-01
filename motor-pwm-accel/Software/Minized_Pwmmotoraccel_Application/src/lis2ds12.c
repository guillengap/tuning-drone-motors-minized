#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include "platform.h"
#include "xil_printf.h"

#include "xparameters.h"
#include "xiicps.h"
#include "xil_printf.h"

#include "lis2ds12.h"

/* The original code was
 * https://www.element14.com/community/thread/66802/l/i2csensor-app-source-code
 *
 */

/************************** Constant Definitions ******************************/

/*
 * The following constants map to the XPAR parameters created in the
 * xparameters.h file. They are defined here such that a user can easily
 * change all the needed parameters in one place.
 */


#define IIC_DEVICE_ID		XPAR_XIICPS_0_DEVICE_ID

/*
 * The slave address to send to and receive from.
 */
#define IIC_SLAVE_ADDR		0x55
#define IIC_SCLK_RATE		100000

XIicPs Iic;		/**< Instance of the IIC Device */

#define MINIZED_MOTION_SENSOR_ADDRESS_SA0_HI  0x1D /* 0011101b for LIS2DS12 on MiniZed when SA0 is pulled high*/

#define TEST_BUFFER_SIZE 32
/*
 * The following buffers are used in this example to send and receive data
 * with the IIC.
 */
u8 SendBuffer[TEST_BUFFER_SIZE];    /**< Buffer for Transmitting Data */
u8 RecvBuffer[TEST_BUFFER_SIZE];    /**< Buffer for Receiving Data */



int IicPsMasterPolledExample()
{
	int Status;
	XIicPs_Config *Config;
	int Index;

	/*
	 * Initialize the IIC driver so that it's ready to use
	 * Look up the configuration in the config table,
	 * then initialize it.
	 */
	Config = XIicPs_LookupConfig(IIC_DEVICE_ID);
	if (NULL == Config) {
		return XST_FAILURE;
	}

	Status = XIicPs_CfgInitialize(&Iic, Config, Config->BaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Perform a self-test to ensure that the hardware was built correctly.
	 */
	Status = XIicPs_SelfTest(&Iic);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Set the IIC serial clock rate.
	 */
	XIicPs_SetSClk(&Iic, IIC_SCLK_RATE);

	return XST_SUCCESS;

}



u8 LIS2DS12_WriteReg(u8 Reg, u8 *Bufp, u16 len)
{
	SendBuffer[0] = Reg;
	int Status;
	for (int ByteCount = 1;ByteCount <= len; ByteCount++)
	{
		SendBuffer[ByteCount] = Bufp[ByteCount-1];
	}

	/*
	 * Send the buffer using the IIC and ignore the number of bytes sent
	 * as the return value since we are using it in interrupt mode.
	 */
	Status = XIicPs_MasterSendPolled(&Iic, SendBuffer,	 (len+1), MINIZED_MOTION_SENSOR_ADDRESS_SA0_HI);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Wait until bus is idle to start another transfer.
	 */
	while (XIicPs_BusIsBusy(&Iic)) {
		/* NOP */
	}

	return XST_SUCCESS;
}

u8 LIS2DS12_ReadReg(uint8_t Reg, uint8_t *Bufp, uint16_t len)
{
	int Status;
	SendBuffer[0] = Reg;

	/*
	 * Send the buffer using the IIC and ignore the number of bytes sent
	 * as the return value since we are using it in interrupt mode.
	 */
	Status = XIicPs_MasterSendPolled(&Iic, SendBuffer,1, MINIZED_MOTION_SENSOR_ADDRESS_SA0_HI);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Wait until bus is idle to start another transfer.
	 */
	while (XIicPs_BusIsBusy(&Iic)) {
		/* NOP */
	}

	Status = XIicPs_MasterRecvPolled(&Iic, Bufp, len, MINIZED_MOTION_SENSOR_ADDRESS_SA0_HI);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	while (XIicPs_BusIsBusy(&Iic)) {
		/* NOP */
	}

	return XST_SUCCESS;
}


bool isSensorConnected()
{
	uint8_t who_am_i = 0;
	uint8_t send_byte;

	LIS2DS12_ReadReg(LIS2DS12_ACC_WHO_AM_I_REG, &who_am_i, 1);
	printf("With I2C device address 0x%02x received WhoAmI = 0x%02x\r\n", MINIZED_MOTION_SENSOR_ADDRESS_SA0_HI, who_am_i);
	if (who_am_i != LIS2DS12_ACC_WHO_AM_I)
	{
		//maybe the address bit was changed, try the other one:
		LIS2DS12_ReadReg(LIS2DS12_ACC_WHO_AM_I_REG, &who_am_i, 1);
		printf("With I2C device address 0x%02x received WhoAmI = 0x%02x\r\n", MINIZED_MOTION_SENSOR_ADDRESS_SA0_HI, who_am_i);
	}
	send_byte = 0x00; //No auto increment
	LIS2DS12_WriteReg(LIS2DS12_ACC_CTRL2, &send_byte, 1);

	//Write 60h in CTRL1	// Turn on the accelerometer.  14-bit mode, ODR = 400 Hz, FS = 2g
	send_byte = 0x60;
	LIS2DS12_WriteReg(LIS2DS12_ACC_CTRL1, &send_byte, 1);
	printf("CTL1 = 0x60 written\r\n");

	//Enable interrupt
	send_byte = 0x01; //Acc data-ready interrupt on INT1
	LIS2DS12_WriteReg(LIS2DS12_ACC_CTRL4, &send_byte, 1);
	printf("CTL4 = 0x01 written\r\n");
	return true;

}
bool sensor_init()
{
	if (XST_SUCCESS != IicPsMasterPolledExample())
	{
		return false;
	}
	isSensorConnected();

	return true;
}


int u16_2s_complement_to_int(u16 word_to_convert)
{
	u16 result_16bit;
	int result_14bit;
	int sign;

	if (word_to_convert & 0x8000)
	{ //MSB is set, negative number
		//Invert and add 1
		sign = -1;
		result_16bit = (~word_to_convert) + 1;
	}
	else
	{ //Positive number
		//No change
		sign = 1;
		result_16bit = word_to_convert;
	}
	//We are using it in 14-bit mode
	//All data is left-aligned.  So convert 16-bit value to 14-but value
	result_14bit = sign * (int)(result_16bit >> 2);
	return(result_14bit);
} //u16_2s_complement_to_int()


bool pollForAccel(int *x, int *z)
{
		int iacceleration_X;
		int iacceleration_Y;
		int iacceleration_Z;
		u8 read_value_LSB;
		u8 read_value_MSB;
		u16 accel_X;
		u16 accel_Y;
		u16 accel_Z;
		u8 accel_status;
		u8 data_ready;

		data_ready = 0;

		 //wait for DRDY
		LIS2DS12_ReadReg(LIS2DS12_ACC_STATUS, &accel_status, 1);
		data_ready = accel_status & 0x01; //bit 0 = DRDY
		if (!data_ready)
			return false;

		//wait for DRDY


		//Read X:
		LIS2DS12_ReadReg(LIS2DS12_ACC_OUT_X_L, &read_value_LSB, 1);
		LIS2DS12_ReadReg(LIS2DS12_ACC_OUT_X_H, &read_value_MSB, 1);
		accel_X = (read_value_MSB << 8) + read_value_LSB;
		iacceleration_X = u16_2s_complement_to_int(accel_X);
		//Read Y:
		LIS2DS12_ReadReg(LIS2DS12_ACC_OUT_Y_L, &read_value_LSB, 1);
		LIS2DS12_ReadReg(LIS2DS12_ACC_OUT_Y_H, &read_value_MSB, 1);
		accel_Y = (read_value_MSB << 8) + read_value_LSB;
		iacceleration_Y = u16_2s_complement_to_int(accel_Y);
		//Read Z:
		LIS2DS12_ReadReg(LIS2DS12_ACC_OUT_Z_L, &read_value_LSB, 1);
		LIS2DS12_ReadReg(LIS2DS12_ACC_OUT_Z_H, &read_value_MSB, 1);
		accel_Z = (read_value_MSB << 8) + read_value_LSB;
		iacceleration_Z = u16_2s_complement_to_int(accel_Z);

		if (x)
		{
			*x = iacceleration_X;
		}

//		if (y)
//		{
//			*y = iacceleration_Y;
//		}

		if (z)
		{
			*z = iacceleration_Z;
		}

	//	printf("  Acceleration = X: %+5d, Y: %+5d, Z: %+5d\r\n",iacceleration_X, iacceleration_Y, iacceleration_Z);
//		printf("  Acceleration = X: %+5d, Y: %+5d, Z: %+5d\r",iacceleration_X, iacceleration_Y, iacceleration_Z);

		return true;
}


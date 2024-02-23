/*
 * gyro.c
 *
 *  Created on: Aug 22, 2022
 *      Author: Amit
 */

/* Includes ------------------------------------------------------------------*/
#include "cmsis_os2.h"
#include "main.h"
#include "i2c.h"


// Enable and configure ICM-20948 gyroscope
// -------------------------------------------------------------------
void gyroStart() {
	writeByte(0x07, 0x07); // Write 8'b00000111 to ICM register 7 (PWR_MGMT_2), disable Gyroscope (all axes)
	osDelayUntil(10);
	writeByte(0x07, 0x00); // Enable Accelerometer and Gyroscope (all axes)
	osDelayUntil(10);
}

void gyroInit() {
	writeByte(0x06, 0x00);	// Write 0x00 to ICM register 6 (PWR_MGMT_1), wakes chip from sleep mode,
							//turn off low power, Internal 20MHz oscillator
	osDelayUntil(10);
	writeByte(0x03, 0x80);	// Enables DMP (Digital Motion Processor) features
	osDelayUntil(10);
	writeByte(0x07, 0x07);	// Disable Gyroscope (all axes)
	osDelayUntil(10);
	writeByte(0x06, 0x01);	// Auto select best available clock source
	osDelayUntil(10);
	writeByte(0x7F, 0x20);	// Switch to USER BANK 2
	osDelayUntil(10);
	writeByte(0x01, 0x2F);	// Config gyro, enable gyro DLPF, set gyro Full Scale to +-2000dps,
							// gyro low pass filter = 3'b101
	osDelayUntil(10);
	writeByte(0x0, 0x00);	// Set gyro sample rate divider = 1 + 0(GYRO_SMPLRT_DIV[7:0])
	osDelayUntil(10);
	writeByte(0x7F, 0x00);	// Switch to USER BANK 0
	osDelayUntil(10);
	writeByte(0x07, 0x00);	// Enable Gyroscope and Accelerometer
	osDelayUntil(10);
}
// -------------------------------------------------------------------
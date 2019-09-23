#include "SPI_LCD.h"
#include "DELAY.h"


void SPILCD_Init(ARM_DRIVER_SPI* spiDrv) {
	// Initilize SPI, enable SPI at full performance, and configure
	// the SPI as Master, using mode 3, bit order from MSB to LSB,
	// Slave Select signal is unused, use 8 bit data words and a
	// transmission speed of 20 Mbit/sec
	spiDrv->Initialize(NULL);
	spiDrv->PowerControl(ARM_POWER_FULL);
	spiDrv->Control(ARM_SPI_MODE_MASTER | ARM_SPI_CPOL1_CPHA1 |
	ARM_SPI_MSB_LSB | ARM_SPI_SS_MASTER_UNUSED |
	ARM_SPI_DATA_BITS(8), 20000000);
	GPIO_PortClock(1);
	GPIO_SetDir(LCD_PORT, LCD_RESET_PIN, GPIO_DIR_OUTPUT);
	GPIO_SetDir(LCD_PORT, LCD_A0_PIN, GPIO_DIR_OUTPUT);
	GPIO_SetDir(LCD_PORT, LCD_CS_PIN, GPIO_DIR_OUTPUT);
	PIN_Configure(LCD_PORT, LCD_RESET_PIN, PIN_FUNC_0, PIN_PINMODE_PULLUP,
	PIN_PINMODE_NORMAL);
	PIN_Configure(LCD_PORT, LCD_A0_PIN, PIN_FUNC_0, PIN_PINMODE_PULLUP,
	PIN_PINMODE_NORMAL);
	PIN_Configure(LCD_PORT, LCD_CS_PIN, PIN_FUNC_0, PIN_PINMODE_PULLUP,
	PIN_PINMODE_NORMAL);
}


void SPILCD_SendCommand(ARM_DRIVER_SPI* spiDrv, uint8_t cmd) {
	ARM_SPI_STATUS status;
	GPIO_PinWrite(LCD_PORT, LCD_A0_PIN, LCD_A0_COMMAND);
	GPIO_PinWrite(LCD_PORT, LCD_CS_PIN, LCD_CS_ACTIVE);
	spiDrv->Send(&cmd, 1);
	do { status = spiDrv->GetStatus(); } while(status.busy);
	GPIO_PinWrite(LCD_PORT, LCD_CS_PIN, LCD_CS_INACTIVE);
}


void SPILCD_SendData(ARM_DRIVER_SPI* spiDrv, uint8_t* data, uint32_t size) {
	ARM_SPI_STATUS status;
	GPIO_PinWrite(LCD_PORT, LCD_A0_PIN, LCD_A0_DATA);
	GPIO_PinWrite(LCD_PORT, LCD_CS_PIN, LCD_CS_ACTIVE);
	spiDrv->Send(data, size);
	do { status = spiDrv->GetStatus(); } while(status.busy);
	GPIO_PinWrite(LCD_PORT, LCD_CS_PIN, LCD_CS_INACTIVE);
}

 
void SPILCD_Reset(ARM_DRIVER_SPI* spiDrv) {
	GPIO_PinWrite(LCD_PORT, LCD_CS_PIN, LCD_CS_ACTIVE);
	GPIO_PinWrite(LCD_PORT, LCD_A0_PIN, LCD_A0_COMMAND);
	GPIO_PinWrite(LCD_PORT, LCD_RESET_PIN, LCD_RESET_ACTIVE);
	delay(50 * US);
	GPIO_PinWrite(LCD_PORT, LCD_RESET_PIN, LCD_RESET_INACTIVE);
	delay(5 * MS);
}
 

void SPILCD_Configure(ARM_DRIVER_SPI* spiDrv) {
	SPILCD_SendCommand(spiDrv, 0xAE);
	SPILCD_SendCommand(spiDrv, 0xA2);
	SPILCD_SendCommand(spiDrv, 0xA0);
	SPILCD_SendCommand(spiDrv, 0xC8);
	SPILCD_SendCommand(spiDrv, 0x22);
	SPILCD_SendCommand(spiDrv, 0x2F);
	SPILCD_SendCommand(spiDrv, 0x40);
	SPILCD_SendCommand(spiDrv, 0xAF);
	SPILCD_SendCommand(spiDrv, 0x81);
	SPILCD_SendCommand(spiDrv, 0x17);
	SPILCD_SendCommand(spiDrv, 0xA6);
}


void SPILCD_Transfer(ARM_DRIVER_SPI* spiDrv, uint8_t* buffer) {
uint32_t page = 0;
	for(page = 0; page < LCD_NUM_PAGES; page++) {
		SPILCD_SendCommand(spiDrv, 0x00); // set column low nibble 0
		SPILCD_SendCommand(spiDrv, 0x10); // set column hi nibble 0
		SPILCD_SendCommand(spiDrv, 0xB0 + page); // set page address
		SPILCD_SendData(spiDrv, &buffer[LCD_PAGE_SIZE * page], LCD_PAGE_SIZE);
	}
}

#include "I2C_ACCEL.h"

#define BIT_ALERT 6
#define BIT_CONTROL 5

 // Este método inicializa el driver I2C que hemos configurado previamente para controlar el acelerómetro
void I2CACCEL_Init(ARM_DRIVER_I2C* i2cDrv) {
 i2cDrv->Initialize(NULL);
 i2cDrv->PowerControl(ARM_POWER_FULL);
 i2cDrv->Control(ARM_I2C_BUS_SPEED, ARM_I2C_BUS_SPEED_FAST);
 i2cDrv->Control(ARM_I2C_BUS_CLEAR, 0);
}

// Este método permite modificar el valor de un registro interno de un periférico a través de I2C
void I2CACCEL_SetRegister(ARM_DRIVER_I2C* i2cDrv, uint32_t deviceAddr,
 uint8_t regId, uint8_t regValue) {
 ARM_I2C_STATUS status;
 static uint8_t buffer[2];
 buffer[0] = regId;
 buffer[1] = regValue;
 i2cDrv->MasterTransmit(deviceAddr, buffer, 2, false);
 do { status = i2cDrv->GetStatus(); } while(status.busy);
}

// Esta función manda un mensaje al periférico indicando que quiere acceder a un registro concreto,que especificamos por parámetro
// espera a que el periférico esté listo, ejecuta la función MasterReceive para recibir el valor del registro y espera a que la lectura se complete. Después, retorna el valor leído
uint8_t I2CACCEL_GetRegister(ARM_DRIVER_I2C* i2cDrv, uint32_t deviceAddr,
 uint8_t regId) {
 ARM_I2C_STATUS status;
 uint8_t result;
 i2cDrv->MasterTransmit(deviceAddr, &regId, 1, true);
 do { status = i2cDrv->GetStatus(); } while(status.busy);
 i2cDrv->MasterReceive(deviceAddr, &result, 1, false);
 do { status = i2cDrv->GetStatus(); } while(status.busy);
 return(result);
}
 
 // Estas funciones serán las que nos permitan interactuar con el acelerómetro.
void I2CACCEL_Configure(ARM_DRIVER_I2C* i2cDrv) {
 I2CACCEL_SetRegister(i2cDrv, ACCEL_ADDR, 0x07, 0x00);
 I2CACCEL_SetRegister(i2cDrv, ACCEL_ADDR, 0x05, 0x00);
 I2CACCEL_SetRegister(i2cDrv, ACCEL_ADDR, 0x06, 0x10);
 I2CACCEL_SetRegister(i2cDrv, ACCEL_ADDR, 0x09, 0xE0);
 I2CACCEL_SetRegister(i2cDrv, ACCEL_ADDR, 0x08, 0x62);
 I2CACCEL_SetRegister(i2cDrv, ACCEL_ADDR, 0x0A, 0x00);
 I2CACCEL_SetRegister(i2cDrv, ACCEL_ADDR, 0x07, 0x41);
}


int8_t I2CACCEL_GetValue(ARM_DRIVER_I2C* i2cDrv,uint8_t axis){
	uint8_t valor;

	// Leemos el valor del registro axis
	valor=I2CACCEL_GetRegister(i2cDrv, ACCEL_ADDR ,axis);

	while ((valor & (0x1 << BIT_ALERT))==0x40){
		// Hacemos una peticion de acceso al registro axis y guardamos su valor en la variable valor
		valor=I2CACCEL_GetRegister(i2cDrv, ACCEL_ADDR ,axis);
	}
	
	if ((valor & (0x1 << BIT_CONTROL))==0x20){
		// Hacemos una extensión de 1's
		valor= valor | (0xE0);
	}
	
	return valor;
}

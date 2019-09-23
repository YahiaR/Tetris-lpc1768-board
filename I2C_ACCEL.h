#ifndef I2C_ACCEL_H_
#define I2C_ACCEL_H_

#include <Driver_I2C.h>
#define ACCEL_ADDR 0x4C // Accelerator Address
#define ACCEL_AXIS_X 0x00 // Accelerator - XOUT Register
#define ACCEL_AXIS_Y 0x01 // Accelerator - YOUT Register
#define ACCEL_AXIS_Z 0x02 // Accelerator - ZOUT Register

// Inicializamos el driver  I2C para controlar el acelerometro
void I2CACCEL_Init(ARM_DRIVER_I2C* i2cDrv);
// Permitimos modificar valores de los registros internos de un periferico a traves del I2C
void I2CACCEL_SetRegister(ARM_DRIVER_I2C* i2cDrv, uint32_t deviceAddr,uint8_t regId, uint8_t regValue);
// Nos permite acceder a un periferico y recibir el valor de un registro leido
uint8_t I2CACCEL_GetRegister(ARM_DRIVER_I2C* i2cDrv, uint32_t deviceAddr, uint8_t regId);
// Configuramos el acelerometro
void I2CACCEL_Configure(ARM_DRIVER_I2C* i2cDrv);
// Función a implementar: leemos una medida y pre-procesar el valor (positivo, negativo o cero) en función de como este la placa.
int8_t I2CACCEL_GetValue(ARM_DRIVER_I2C* i2cDrv,uint8_t axis);


#endif /* I2C_ACCEL_H_ */

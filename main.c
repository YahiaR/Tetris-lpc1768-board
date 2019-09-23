#include <LPC17xx.h>
#include "DELAY.h"
#include <string.h>
#include <stdlib.h>
#include <system_LPC17xx.h>
#include "PIN_LPC17xx.h"
#include <GPIO_LPC17xx.h>
#include "SPI_LCD.h"
#include "I2C_ACCEL.h"

 //sacados de mbed NXP schematics PWM1.4
 #define PORT_PWM 2
 #define PIN_PWM 3
 #define PORT_POT 1
 #define PIN_POT 30
 
 
 // LEDS
#define PORT_LED 1 //para estos no hace falta mirar la leyenda de detras de la placa, solo con el esquematico del micro nos vale
#define PIN_LED1 18
#define PIN_LED2 20 
#define PIN_LED3 21
#define PIN_LED4 23
 
// JOYSTICK
#define PORT_JOY 0
#define PIN_JOY_UP 23 //necesitamos mirar la leyenda de detsras de la placa para ver a donde esta mapeado lo que queremos usar 
                      // y con este lo asociamos al esquematico de moodle de la placa (nxp lpc 1768) para saber a que pines del micro corresponden (P0.23).
 
#define PIN_JOY_DOWN 17
#define PIN_JOY_RIGHT 24
#define PIN_JOY_LEFT 15
#define PIN_JOY_CENTER 16

static uint32_t state_joy_up; // importante el 32 porque puede cambiar dependiendo de la arquitectura del micro si solo pones uint.
static uint32_t state_joy_down;
static uint32_t state_joy_right;
static uint32_t state_joy_left;
static uint32_t state_joy_center;
static uint32_t Valor_POT;


// Declaración de variables globales
int valor_y = 0;
int movimiento = 0; 	// Esta variable puede ser (-1,0,1) que diran si se mueve a izquierda, no se mueve o derecha
int cayendo = 1;
int first_square = 0;
int second_square = 0;
int previous_square1 = 0;
int previous_square2 = 0;
int byte;
int actualizarLCD=0;
uint32_t refresco = 0;

extern ARM_DRIVER_SPI Driver_SPI1;
extern ARM_DRIVER_I2C Driver_I2C2;

ARM_DRIVER_SPI* spiDrv1 = &Driver_SPI1;	
uint8_t buffer[LCD_MEM_SIZE];
ARM_DRIVER_I2C* i2cDrv2 = &Driver_I2C2;


// Declaración de las funciones que usaremos a lo largo de la práctica
void Buffer_Reset(uint8_t* buffer);
void Buffer_SetPixel(uint8_t* buffer, uint8_t x, uint8_t y);
void Buffer_DrawRect(uint8_t* buffer, uint8_t x, uint8_t y, uint8_t ancho, uint8_t alto);
void DrawSquare(uint8_t* buffer, uint8_t largo, uint8_t ancho);
int Buffer_GetPixel(uint8_t* buffer, uint8_t x, uint8_t y);
int Buffer_CheckRect(uint8_t* buffer, uint8_t x, uint8_t y, uint8_t ancho, uint8_t alto);
int CheckSquare(uint8_t* buffer, uint8_t largo, uint8_t ancho);
void Buffer_SetPixel0(uint8_t* buffer, uint8_t x, uint8_t y);
void Buffer_DrawRect0(uint8_t* buffer, uint8_t x, uint8_t y, uint8_t ancho, uint8_t alto);
void DrawSquare0(uint8_t* buffer, uint8_t largo, uint8_t ancho);
void  TIMER1_IRQHandler (void);
void ConfiguraGPIO(void);	
void ConfiguraTIMER1(void);


// Las dos siguientes funciones serán utilizadas si queremos añadir la mejora adicional 2 (uso del joystick) 
void initPin_LED(uint32_t port_number, uint32_t pin_number)
{
    // configura el pin como GPIO de salida con resistencia de pulldown
    GPIO_SetDir(port_number, pin_number, GPIO_DIR_OUTPUT);
    PIN_Configure(port_number, pin_number, PIN_FUNC_0, // el 00 porque queremos q funcione como gpio (tabla de las diapòsitivas)
                  PIN_PINMODE_PULLDOWN, PIN_PINMODE_NORMAL);
}
 
void initPin_JOY(uint32_t port_number, uint32_t pin_number)
{
    // configura el pin como GPIO de entrada con resistencia de pulldown
    GPIO_SetDir(port_number, pin_number, GPIO_DIR_INPUT);
    PIN_Configure(port_number, pin_number, PIN_FUNC_0,
                  PIN_PINMODE_PULLDOWN, PIN_PINMODE_NORMAL);
}
 

void  TIMER1_IRQHandler (void){
	// Actualizamos la variable usada para LCD
	actualizarLCD=1; 
	// Borramos la bandera de interrupcion de MR0
	LPC_TIM1->IR |= (0x01 << 0);
}

void ConfiguraTIMER1(void){
	// Activo Timer1 en el PCONP
	LPC_SC->PCONP |= (0X01 << 1);
	// Activo MROI y MROR
	LPC_TIM1->MCR |= (0X03 << 0);
	LPC_TIM1->PR = 0; 
	LPC_TIM1->MR0 = (5000000);
	// Reset de Timer0 
	LPC_TIM1->TCR |= (0x01 <<0);
}

// Esta funcion se encarga de limpiar el buffer poniendo a cero todas las posiciones de lavariable recibida  
void Buffer_Reset (uint8_t* buffer){
    uint32_t i;
    for(i=0;i<LCD_MEM_SIZE;i++)
    {
        buffer[i] = 0;
    }
}

// Pintamos un pixel en una posición exacta
void Buffer_SetPixel (uint8_t* buffer,uint8_t x,uint8_t y){
    // Tratamos la variable x como columnas
  	// Tratamos la variable y como filas
    int pagina=0;
	int mascara=0;
	int auxiliar=0;
    
    // Veo mi posición actual 
    pagina=(y/LCD_NUM_ROWS_PER_PAGE);
	mascara = (y%LCD_NUM_ROWS_PER_PAGE);
    // Veo mi byte actual
    auxiliar=pagina*LCD_NUM_COLS+x;
    buffer[auxiliar] |= (1<< mascara);
}
 
// Pintamos un rectangulo con el alto y ancho recibido en la posicion (x,y)
void Buffer_DrawRect(uint8_t* buffer, uint8_t x, uint8_t y, uint8_t ancho, uint8_t alto){
	int i,j;
	i = 0;
	j = 0;
	// Para cada posición de x e y vamos pintando
 	 for (j=0;j<alto;j++){
		for (i=0;i<ancho;i++){
			// Pintamos llamando a la función previamente realizada
			Buffer_SetPixel(buffer,x+i,y+j);
    	}
  	}
}
// Usaremos esta función para pintar un cuadro de 8x8
void DrawSquare (uint8_t* buffer, uint8_t largo, uint8_t ancho){
	Buffer_DrawRect(buffer, largo*4, ancho*4, 8,8);
}

int Buffer_GetPixel(uint8_t* buffer, uint8_t x, uint8_t y) {	
	int numero_pagina = 0;
	int mascara = 0;
	int resultado = 2;
	int auxiliar = 0;
	
	numero_pagina = (y/LCD_NUM_ROWS_PER_PAGE);
	auxiliar=numero_pagina*LCD_NUM_COLS+x;
	mascara = y % LCD_NUM_ROWS_PER_PAGE; 
	resultado = (buffer[auxiliar] & (0x1 << mascara)) >> mascara;
	
	return resultado;
}

// Usaremos esta función para comprobar si una posicion esta ocupada
int Buffer_CheckRect(uint8_t* buffer, uint8_t x, uint8_t y, uint8_t ancho, uint8_t alto) {
	int i,j;
	i = 0;
	j = 0;
  for (j=0;j<alto;j++){
		for (i=0;i<ancho;i++){
			if (Buffer_GetPixel(buffer, x+i,y+j) == 1){
				return 1;
			}
    }
  }
	return 0;
}

// Función auxiliar que llamara a la anterior devolviendonos un 1 en casa de estar ocupada esa posición
int CheckSquare (uint8_t* buffer, uint8_t largo, uint8_t ancho){
	if (Buffer_CheckRect(buffer, largo*4, ancho*4, 8,8) == 1){
		return 1;
	}
	return 0;
}

// Pintamos el cuadrado pixel a pixel
void Buffer_SetPixel0(uint8_t* buffer, uint8_t x, uint8_t y) {
	int pagina=0;
	int mascara=0;
	int auxiliar=0;
    
    // Veo mi posición actual 
    pagina=(y/LCD_NUM_ROWS_PER_PAGE);
	mascara = (y%LCD_NUM_ROWS_PER_PAGE);
    // Veo mi byte actual
    auxiliar=pagina*LCD_NUM_COLS+x;
    buffer[auxiliar] &= ~(1 << mascara);
}

void Buffer_DrawRect0(uint8_t* buffer, uint8_t x, uint8_t y, uint8_t ancho, uint8_t alto) {
	int i,j;
	for(i = x; i < ancho; i++){
		for(j=y; j < alto; j++){
			Buffer_SetPixel0(buffer, x+i,y+j);
		}
	}
}

void DrawSquare0(uint8_t* buffer, uint8_t largo, uint8_t ancho){
	Buffer_DrawRect0(buffer, largo*4, ancho*4, 8,8);
}

// Configuramos aqui todo lo relativo al PWM para ahorrarnos codigo en el main
void ConfiguraPWM(){   
    //PWM1MCR [1] = 0x1  Reset on PWMMR0
    LPC_PWM1->MCR = LPC_PWM1->MCR | (0x01 << 1);    
    // Cargar el valor del prescaler
    LPC_PWM1->PR = 0x1D; // Valor del prescaler     
    // Cargar los valores para los registros de comparación MR0 and MR4
    LPC_PWM1->MR0 = 0xfff; 
    LPC_PWM1->MR4 = 0x0;     
    // Habilitar la salida PWM4
    //PWM1PCR [12] = 0x1
    LPC_PWM1->PCR = LPC_PWM1->PCR | (0x01 << 12);
    // Paso final: habilitar el contador para que el contador del Timer
    // y el contador del prescaler comiencen a contar
    LPC_PWM1->TCR = LPC_PWM1->TCR | (0x01 << 0);
    LPC_PWM1->TCR = LPC_PWM1->TCR | (0x01 << 3);     
    // BLOQUE A/D             
    LPC_SC->PCONP = LPC_SC->PCONP | (1 << 12); //ENABLE POWER TO ADC & IOCONF
    LPC_ADC->ADCR = (LPC_ADC->ADCR & ~(0xFF << 0)) | (0x10 << 0) | (0x04 << 8) | (0x1 << 21) | (0x0 << 24);
	}	

// funcion que llamara a la anterior para configurar y despues controlar el potenciometro con la luz roja que sera usada en cualquier momento
void ControlarPotenciometro(){
		
	ConfiguraPWM();
						
	LPC_ADC->ADCR = LPC_ADC->ADCR | 0x001<<24u; 
    while((LPC_ADC->ADGDR & (0x1UL<<31))== 0);     
    Valor_POT = (LPC_ADC->ADGDR >> 4u) & 0xfff;         
    // Hacemos dos if para que no haya problemas de que no se apague o parpadee cuando llega casi al máximo 
    if(Valor_POT <= 0x015){
        Valor_POT = 0x000;
    }             
    if(Valor_POT >= 0xFE5){
        Valor_POT = 0xFFF;
    }              
    LPC_PWM1->MR4 = Valor_POT;
    LPC_PWM1->LER = LPC_PWM1->LER | (0x01 << 4);
}
	
int main(void) {
	
SystemInit();
	
//TIM1	
GPIO_PortClock(1);
ConfiguraTIMER1();
NVIC_EnableIRQ(TIMER1_IRQn);
	
//SPI
  
SPILCD_Init(spiDrv1);
SPILCD_Reset(spiDrv1);
SPILCD_Configure(spiDrv1);	
	
 
Buffer_Reset(buffer);
	
//I2C
	
I2CACCEL_Init(i2cDrv2);
I2CACCEL_Configure(i2cDrv2);
delay(10 * MS);

// Configuracion de pines para poder usar funciones adicionales de luces
PIN_Configure(PORT_PWM,PIN_PWM,PIN_FUNC_1,PIN_PINMODE_PULLDOWN,PIN_PINMODE_NORMAL);
PIN_Configure(PORT_POT,PIN_POT,PIN_FUNC_3,PIN_PINMODE_PULLDOWN,PIN_PINMODE_NORMAL);//la funcion 11 A/D

// Inicializamos los pines y los joystick	
initPin_LED(PORT_LED, PIN_LED1);
initPin_LED(PORT_LED, PIN_LED2);
initPin_LED(PORT_LED, PIN_LED3);
initPin_LED(PORT_LED, PIN_LED4);
     
initPin_JOY(PORT_JOY, PIN_JOY_UP);
initPin_JOY(PORT_JOY, PIN_JOY_DOWN);
initPin_JOY(PORT_JOY, PIN_JOY_RIGHT);
initPin_JOY(PORT_JOY, PIN_JOY_LEFT);
initPin_JOY(PORT_JOY, PIN_JOY_CENTER);

// Generamos y pintamos nuestro primer cuadrado en pantalla
DrawSquare(buffer, first_square, second_square);
SPILCD_Transfer(spiDrv1, buffer);

while(1){
	
	// Si hay un cuadrado cayendo ( cayendo == 1)	    
	while(cayendo){
		
		// Activamos inicializacion y puesta a punto del potenciometro para poder usarlo
		ControlarPotenciometro();
		// Guardamos el valor devuelto por el acelerometro para saber la inclinación de la placa
		valor_y = I2CACCEL_GetValue(i2cDrv2, ACCEL_AXIS_Y);
		
		// Decidimos hacia que lado se mueve el cuadrado en función del valor del acelerometro				
		if (valor_y > 10){
			movimiento = 1;
		}
		else if (valor_y < -10){
			movimiento = -1;
		}
		else
			movimiento = 0;
			
		// Dibujamos nuestro cuadrado del punto (0,0)
		DrawSquare0(buffer,first_square,second_square); // quitamos el cuadrado
		SPILCD_Transfer(spiDrv1, buffer);
		// Usamos dos variables auxiliares por si tenemos colisiones 
		previous_square1 = first_square; 
		previous_square2 = second_square;
			
		
		// Analizamos los casos de colisión
		// Si nuestro cuadrado llega al fondo de la pantalla
		if (first_square == 30){ 
			first_square = first_square;
			// Impedimos que pueda seguir bajando más
			cayendo = 0; 
			// Pintamos un cuadrado en esa posición y hacemos un delay
			DrawSquare(buffer,first_square,second_square);
			SPILCD_Transfer(spiDrv1, buffer);
			delay(0.25);
			break;
		}
		else
			// Si  nuestro cuadrado esta en algún otro punto de la pantalla aumentamos el valor de su posición
			first_square = first_square + 1;
			
		// Si estamos en el borde derecho y nos queremos mover más a la derecha
		if (second_square == 0 & movimiento == -1)
			//Actualizamos la posición para que siga bajando por ahi y no pueda irse a otra posición fuera de la pantalla
			second_square = second_square;
		// Si estamos en el borde izquierdo y nos queremos mover más a la izquierda
		else if (second_square == 6 & movimiento == 1) 
			second_square = second_square;
		// Sino actualizamos la posición
		else
			second_square = second_square + movimiento;
		
		// Comprobación de posiciones. Vamos comprobando la posicion siguiente (+1 abajo) y usando las variables previous
		if (CheckSquare(buffer, first_square,second_square) == 1){				
			first_square = previous_square1 + 1;
			second_square = previous_square2;
			// Impedimos el movimiento
			movimiento = 0;
			// Si no podemos seguir bajando porque hay algo ponemos la variable cayendo = 0
			if (CheckSquare(buffer, first_square,second_square) == 1){ 
				cayendo = 0;
				// Dibujamos el cuadrado en la posición fijo
				DrawSquare(buffer, previous_square1, previous_square2);
				break;
			}
		}
		
		// Pintamos un nuevo cuadrado	
		DrawSquare(buffer,first_square,second_square);
		SPILCD_Transfer(spiDrv1, buffer);
		delay(0.10);
	}
	
	// Si llegamos a la parte de arriba del todo (perdemos)	
	if (first_square == 1){
		// Vaciamos el buffer y actualizamos la pantalla para que aparezca vacia
		Buffer_Reset(buffer);	
		SPILCD_Transfer(spiDrv1, buffer);
			
		// Posible mejora adicional 2: en vez de hacer que se reinicie la partida 2 segundos despues de terminar la anterior, si descomentamos las 
		// siguientes lineas comentadas conseguiremos que al acabar la primera no se pueda jugar más al tetris y solo podamos encender leds. Un led 
		// por cada direccion del joystick. Si pulsamos el boton central se encenderán todos.
	//	while(1) {
//          state_joy_up = GPIO_PinRead(PORT_JOY, PIN_JOY_UP); //leo el estado del pin
//	        state_joy_down = GPIO_PinRead(PORT_JOY, PIN_JOY_DOWN);
//	        state_joy_right = GPIO_PinRead(PORT_JOY, PIN_JOY_RIGHT);
//	        state_joy_left = GPIO_PinRead(PORT_JOY, PIN_JOY_LEFT);
//	        state_joy_center = GPIO_PinRead(PORT_JOY, PIN_JOY_CENTER);
	         
	        // escribimos el valor de los state en el bit PIN_LEDX del puerto definido como una macro para cada uno de los sentidos del joystick
//	        GPIO_PinWrite(PORT_LED, PIN_LED1, state_joy_up);  //actualizo y escribo el estado
//	        GPIO_PinWrite(PORT_LED, PIN_LED2, state_joy_down);
//	        GPIO_PinWrite(PORT_LED, PIN_LED3, state_joy_right);
//	        GPIO_PinWrite(PORT_LED, PIN_LED4, state_joy_left);
	         
	        // hacemos lo mismo para el boton central, le pasamos el estado del boton central para ponerlo en los bits de los leds de cada boton lateral
//	        GPIO_PinWrite(PORT_LED, PIN_LED1, state_joy_center);
//	        GPIO_PinWrite(PORT_LED, PIN_LED2, state_joy_center);
//	        GPIO_PinWrite(PORT_LED, PIN_LED3, state_joy_center);
//	        GPIO_PinWrite(PORT_LED, PIN_LED4, state_joy_center);
         
 //  		 }
		// Reiniciamos a otra partida 2 segundos despues	
		delay(2);
	}

	// Reiniciamos los valores para empezar una nueva caida del cuadrado (lo pintamos arriba de nuevo)
	first_square = 0;
	second_square = 0;
	cayendo = 1;
	DrawSquare(buffer, first_square, second_square);
	SPILCD_Transfer(spiDrv1, buffer);
	delay(1);
}
}
 

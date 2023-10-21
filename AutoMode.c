// main.c
// Runs on TM4C1294
// realiza transferencias solicitadas por software de una region de memoria a otra
// El origen de la transferencia puede ser flash o ram, y el destino es un arreglo en ram

#include <stdint.h>
#include "uDMA_DRIVERS.h"
#include "RQST_PERIPH_DRIVERS.h"

#include "tm4c1294ncpdt.h"

#define SIZE 128
#define FLASH 1
#define RAM 0

//Localidad destino para transferencia por software
uint32_t DestBuf[SIZE];
uint32_t SrcBuf_RAM[SIZE];
uint32_t Src_FLASH = 0x30;  //Flash (ver mapa de memoria pag.103)

/*TRANSFERENCIAS DE MEMORIA A MEMORIA
 * DATOS = 32 bits
 * TOTAL = 128 DATOS "DestBuf"
 * MODO AUTO
 */

int main(void){
  volatile uint32_t delay; uint32_t i,t;
  //Configura el puerto F como GPIO para visualizar salida LED
  SYSCTL_RCGCGPIO_R |= 0x20; // 0) Reloj
  delay = SYSCTL_RCGCGPIO_R;              // retardo de activacion
  GPIO_PORTF_DIR_R |= 0x01;    // PF0 como salida
  GPIO_PORTF_AFSEL_R &= ~0x01; // deshabilita la funcion alterna de PF0
  GPIO_PORTF_DEN_R |= 0x01;    // modo digital en PF0
  GPIO_PORTF_PCTL_R = (GPIO_PORTF_PCTL_R&0xFFF0FFF0)+0x00000000;
  GPIO_PORTF_AMSEL_R = 0;      // deshabilita la funcion analógica en PF0
  GPIO_PORTF_DATA_R = 0;        // apaga LED
  DMA_Init(30,0);  // inicializa uDMA para una solicitud de transferencia por sofwr

  uint8_t origenFlash = 0;      //Controla el orgien de transferenicia
  if(origenFlash){
      Flash_Init();// inicializa controlador Flash para transferencias de lectura por softwr
      while(Auto_DMA_Status()); // espera a que termine la transferencia
          DMA_Auto_Start(Src_FLASH,DestBuf,SIZE);
          //TRansferencias de flash comprobadas
  }
  else{ //entonces el origen es RAM, una funcion rampa
  for(i=0;i<SIZE;i++){
        SrcBuf_RAM[i] = i; //Descomental para transferencias ram - ram
        DestBuf[i] = 0;
      }
  while(Auto_DMA_Status()); // espera a que termine la transferencia
      DMA_Auto_Start(SrcBuf_RAM,DestBuf,SIZE);
      //TRansferencias de flash comprobadas
  }

      GPIO_PORTF_DATA_R = 0X01;
  while(1);
}

// UART0 Rx uses uDMA channel 8 encoding 0
#define CH8 (8*4)
#define CH8ALT (8*4+128)
#define BIT8 0x00000100

#include <stdint.h>
#include "tm4c1294ncpdt.h"
#include "RQST_PERIPH_DRIVERS.h"
#include "uDMA_DRIVERS.h"


/*
 * TRANSFERENCIAS DE ENTRADA DMA POR BYTES DE UART
 * 1 BYTE = 1 DATO
 * TOTAL = INDEFINIDO "MiTabla"
 * MODO PING-PONG
 */

uint8_t MiTabla[256];


#define TOTALBYTES 4

int main(void){
    UART0Rx_init();
    DMA_Init(8,0);
    DMA_Ping_Start(0x4000C000, MiTabla,TOTALBYTES,8);  //Configura estructura de control de canale
    while(1){           // Programa principal

    }
}



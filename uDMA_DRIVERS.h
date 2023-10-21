#include <stdint.h>
#include <stdbool.h>
#include "tm4c1294ncpdt.h"
extern unsigned long CounTransfer;
//Constantes para la configuracion del DMA
// The control table used by the uDMA controller.  This table must be aligned to a 1024 byte boundary.
uint32_t ucControlTable[256]__attribute__((aligned(1024)));

#define ping 2
#define auto 3
#define basic 1



// *****DMA_Init******
// Inicializa en DMA para realizar transferenicas solicitadas por el timer3A
// Se ejecuta previo a cada solicitud de una transferencia completa

void DMA_Init(int CH_num, int encoding){
    uint32_t CH_bit = 1<<CH_num;
    volatile unsigned long delay;
      SYSCTL_RCGCDMA_R |= 0x01;                                 //0) Reloj uDMA 1
      delay = SYSCTL_RCGCDMA_R;                                 //retardo de activacion
      UDMA_CFG_R = 0x01;                                        //1) Habilitar operacion de DMA
      UDMA_CTLBASE_R = (uint32_t)ucControlTable;                //2) Ubicar la tabla de control para los canales
      if(CH_num < 7)
          UDMA_CHMAP0_R = (UDMA_CHMAP0_R&0xFFFFF0FF)|(encoding<<(4*CH_num));    //3) Fuente de solicitud. Canal 2 = timer3A

      if (CH_num > 7 && CH_num < 15){
          int shift = 4*(CH_num-8);
          UDMA_CHMAP1_R = (UDMA_CHMAP1_R&0xFF0FFFFF)|(encoding<<shift);         //3) Fuente de solicitud. Canal 8/9 = UART0
      }
      if(CH_num > 15 && CH_num <23){
          int shift = 4*(CH_num-16);
          UDMA_CHMAP2_R = (UDMA_CHMAP2_R&0xFF0FFFFF)|(encoding<<shift);         //3) Fuente de solicitud. Canal 21 = GPIOJ
      }    //3) Fuente de solicitud. Canal 21 = GPIOJ
      if(CH_num > 23){
          int shift = 4*(CH_num-24);
          UDMA_CHMAP2_R = (UDMA_CHMAP2_R&0xFF0FFFFF)|(encoding<<shift);         //3) Fuente de solicitud. Canal 30 = Software
      }
      UDMA_PRIOCLR_R = 0xFF;     //4) No hay prioridades
      UDMA_ALTCLR_R = CH_bit;      //5) Canal 2 usa control primario
      UDMA_USEBURSTSET_R = CH_bit; //6) Canal  2 acepta solicitudes de rafagas*timer=brst
      if(CH_num == 8){
                UDMA_USEBURSTCLR_R = CH_bit; //6) Canal  2 acepta solicitudes de rafagas*timer=brst
            }
      UDMA_REQMASKCLR_R = CH_bit;  //7) Deshabilita al uDMA a atender solicitudes del canal 2

}

uint8_t *SourcePt;                 //Direccion fija del origen
volatile uint32_t *DestinationPt;   //Direccion fija del destino
uint32_t Count_Word;                //Cantidad de transferencias segun el tamanio de palabra

//Funcion privada para reprogramar la parte principal de un canal de la estructura de control

void setRegular(int CH_num, uint8_t mode){
    ucControlTable[CH_num]   = (uint32_t)SourcePt;               //0) Direccion origen
    ucControlTable[CH_num+1] = (uint32_t)DestinationPt;          //1) Direccion destino
    if(mode == ping){
        ucControlTable[CH_num+2] = 0x0C008003+((Count_Word-1)<<4);   //2) DMA Channel Control Word (DMACHCTL) modo ping
    }
    /* DMACHCTL          Bits    Value Description
       DSTINC            31:30   00    incremento en la direccion destino = 8-bit
       DSTSIZE           29:28   00    Tamaño del dato = 8-bit en destino
       SRCINC            27:26   11    No incrementa la direccion de origen
       SRCSIZE           25:24   00    Tamaño del dato = 8-bit en origen
       reserved          23:18   0     Reservado
       ARBSIZE           17:14   0011  Arbitraje cada 4 transferencia
       XFERSIZE          13:4  count-1 Total de transferencias
       NXTUSEBURST       3       0     No aplica para modo ping-pong
       XFERMODE          2:0     011   Modo Ping Pong
      */
    if(mode == basic) {
        ucControlTable[CH_num+2] = 0xC0008001+((Count_Word-1)<<4);   //2) DMA Channel Control Word (DMACHCTL) simple mod
    }
/* DMACHCTL          Bits    Valor Descripcion
   DSTINC            31:30   11    no incremento en la direccion destino
   DSTSIZE           29:28   00    Tamano de dato = 8-bit en destino
   SRCINC            27:26   00    incremento en la direccion origen = 8 bit
   SRCSIZE           25:24   00    Tamano de dato = 8-bit en origen
   reserved          23:18   0     Reservado
   ARBSIZE           17:14   0011     Arbitrataje cada 4 transferencias
   XFERSIZE          13:4  countWord-1 Total de transferencias
   NXTUSEBURST       3       0      no aplica para modo basico
   XFERMODE          2:0     01     Modo basico
  */
}

void static Ping_setAlternate(int CH_num){
  ucControlTable[CH_num]   = (uint32_t)SourcePt;               //0) Direccion origen
  ucControlTable[CH_num+1] = (uint32_t)DestinationPt;          //1) Direccion destino
  ucControlTable[CH_num+2] = 0x0C008003+((Count_Word-1)<<4);        //2) DMA Channel Control Word (DMACHCTL)
  /* DMACHCTL          Bits    Value Description
     DSTINC            31:30   00    incremento en la direccion destino = 8-bit
     DSTSIZE           29:28   00    Tamaño del dato = 8-bit en destino
     SRCINC            27:26   11    No incrementa la direccion de origen
     SRCSIZE           25:24   00    Tamaño del dato = 8-bit en origen
     reserved          23:18   0     Reservado
     ARBSIZE           17:14   0011  Arbitraje cada 4 transferencia
     XFERSIZE          13:4  count-1 Total de transferencias
     NXTUSEBURST       3       0     No aplica para modo ping-pong
     XFERMODE          2:0     011   Modo Ping Pong
    */
}


#define CH30 (30*4)
#define BIT30 0x40000000
void DMA_Auto_Start(uint32_t *source, uint32_t *destination, uint32_t count){
  ucControlTable[CH30]   = (uint32_t)source+count*4-1;       // direccion origen
  ucControlTable[CH30+1] = (uint32_t)destination+count*4-1;  // direccion destino
  ucControlTable[CH30+2] = 0xAA00C002+((count-1)<<4);             // DMA Channel Control Word (DMACHCTL)
/* DMACHCTL          Bits    Value Description
   DSTINC            31:30   10    32-bit incremento en la direccion destino
   DSTSIZE           29:28   10    32-bit tamaño de dato en la localidad destino
   SRCINC            27:26   10    32-bit incremento en la direccion origen
   SRCSIZE           25:24   10    32-bit tamaño de dato en la localidad origen
   reserved          23:18   0     Reserved
   ARBSIZE           17:14   0011  Arbitrajecada 8 transferencias
   XFERSIZE          13:4  count-1 conteo de transferencias realizadas
   NXTUSEBURST       3       0     N/A para este tipo de transferencia
   XFERMODE          2:0     010   modo Auto-solicitud
  */
  UDMA_ENASET_R = BIT30;  // canal 30 del uDMA habilitado
  UDMA_SWREQ_R = BIT30;   // instruccion de solicitud por software
  // bit 30 en UDMA_ENASET_R se hace cero cuando la transferencia termina
  // bits 2:0 ucControlTable[CH30+2] se hacen cero cuando la transferencia termina
  //  existe la interrupcion 44 en el vector 60 del NVIC (0x0000.00F0) que permite
  //una rutina de interrupcion cuando la transferencia por software termina
}




// *****DMA_Star******
// configura las direcciones de origen y destino de las transferencias solicitadas
// transferencias de 8 bits en rafagas de 4 bytes
// La direccion del origen se incrementa en 8 bits
// Entradas: source Apuntador de un buffer en RAM que contiene valores de 0 a 255
//          destination Registro FIFO de transmision UART0Tx
//          count numero de bytes a transferir (max 1024 palabras)
// Esta rutina no espera la finalizacion de la transferencia
//Se deben haber habilitado todos los modulos antes (GPIO, UART, uDMA y Timer)
void DMA_Basic_Start(volatile uint8_t *source, volatile uint32_t *destination, uint32_t count, int CH_num){
    SourcePt = source+count-1;
    DestinationPt = destination;
    Count_Word = count;
    setRegular(CH_num*4,basic);              //0) Configura los parametros de la transmision

    if(CH_num == 9){
    UART0_DMACTL_R |= 0x02;
    UART0_IM_R |= 0x30020;
    UART0_CTL_R |= 0x00000010;//EOT bit set
    UART0_CTL_R |= 0x00000001;}                  //8) Habilita UART_0

    if(CH_num == 21){
    GPIO_PORTJ_DMACTL_R = 0x01;}       // Se habilita al GPIO a enviar señales de solicitud

    if(CH_num == 2){
    TIMER3_DMAEV_R |= 0x01;      //1) Activa dma_req signal para evento time-out pp1019
    NVIC_EN1_R = 1<<(35-32);     //2) Habilita interrupcion 35 para timer_3A
    TIMER3_CTL_R |= 0x00000001;}  //3) Habilita timer_3

    uint32_t CH_bit = 1<<CH_num;
    UDMA_ENASET_R |= CH_bit;       //4)Canal 21 habilitado
    // CH_bit en UDMA_ENASET_R se limpia cuando termina
    // bits 2:0 ucControlTable[CH+2] se limpia cuando terminan las transferenicas
}

void DMA_Ping_Start(volatile uint8_t *source,uint8_t *destination, uint32_t count,int CH_num){
  SourcePt = source;                    // Apuntador a la direccion origen
  DestinationPt = destination+count-1;  // Apuntador a la direccion destino
  Count_Word = count;                        // Numero de bytes

  setRegular(CH_num*4,ping);             //0) Configura los parametros de la transmision
  Ping_setAlternate((CH_num*4)+128);           //1) Configura los parametros de la transmision en la estructua alterna
  if(CH_num == 8){
  NVIC_EN0_R |= (1<<5);     //2) Habilita interrupcion 5 para UART0
  UART0_DMACTL_R |= UART_DMACTL_RXDMAE;   //3) Activa dma_req signal para evento nivel de FIFO
  UDMA_ENASET_R = 0x00000100;}     //4) Habilita el canal 8 a hacer solicitudes de transferencia
}

// *****DMA_Status******
// Checa cuantas transferencias se han realizado
// Entradas: nada
// Salidas: contador de transferencias completadas
uint32_t DMA_Status(void){
  return CounTransfer;     //contador de transferencias completadas
  // ï¿½DMA Channel 8 enable bit is high if active
}

// ************DMA_Status*****************
//Indica el estado de la transferencia
uint32_t Auto_DMA_Status(void){
  return (UDMA_ENASET_R&BIT30);  // 1 si la transferencia del canal 30 sigue activa
                                 // 0 si ha terminado o no ha epezado
}

//****DMA_Stop*******
//Inhabilita al DMA a recibir mas solicitudes de transferencia
void DMA_Stop(int CH_num){
    uint32_t CH_bit = 1<<CH_num;
    UDMA_ENACLR_R = CH_bit;       //0) Deshabilitar canal 2 a realizar solicitudes
    if(CH_num == 2){
        NVIC_DIS1_R = 0x00000008;   //1) Desactiva la interrupcion directo del NVIC
        TIMER3_CTL_R &=~0x01;       //2) Desactiva el timmer para evitar mas interrupciones dado el modo periodico
    }
    if(CH_num == 8){
        NVIC_DIS0_R |= (1<<5);      //1) Desactiva la interrupcion directo del NVIC
        UART0_IM_R &= ~0X00010000;  //2) Enmascara la interrupcion en el modulo UART
        }
    }


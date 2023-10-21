/* Drivers para el manejo de perifericos Timer, UART, y GPIO
 * Estos perifericos se usan como fuentes de solicictud de transferencias
 * configurando su respectivo canal (ver tabla de canales pag. 680)*/


//Librerias
#include <stdint.h>
#include <stdbool.h>
#include "tm4c1294ncpdt.h"

// Timer3A uses uDMA channel 2 encoding 1
#define CH2 (2*4)
#define CH2ALT (2*4+128)
#define BIT2 0x00000004
// UART0 Rx uses uDMA channel 9 encoding 0
#define CH8 (9*4)
#define CH8ALT (9*4+128)
#define BIT8 0x00000200
// GPIOJ uses uDMA channel 21 encoding 3
#define CH21 (21*4)
#define CH21ALT (21*4+128)
#define BIT21 0x00200000


unsigned long CounTransfer=0;


// ******Timer3A_Init********
// Configura y activa el timer 3A para una solicitud
// periodica de transferencias por DMA
// entradas: Periodo de cuenta en ms
void Timer3A_Init(uint16_t period){
    uint32_t ui32Loop;
        SYSCTL_RCGCTIMER_R |= 0x08;           //0) Activar Timer_3
        ui32Loop = SYSCTL_RCGCGPIO_R;         //retardo de activacion

/**** Afecta a los timers A y B ******/
        TIMER3_CTL_R = 0X00000000;       //1) Deshabilita el timer durante configuracion
        TIMER3_CFG_R = 0X00000004;       //2) Configura modo de 16-bit
/**** Afecta al timer A **********/
        TIMER3_TAMR_R = 0x00000002;      //3) Configura modo periodico (recarga automatica), y modo de cuenta hacia abajo
        TIMER3_TAILR_R = 65*(period-1);  //4) Valor de recarga de 16 bits
        TIMER3_TAPR_R  =       0xFF;     //5) Valor del prescalador-1 para 1ms timer_3A
        TIMER3_ICR_R   = 0x00000001;     //6) Limpia banderas pendientes del fin de cuenta
        TIMER3_IMR_R |= 0x00000001;      //7) Desenmascara interrupcion del timer_3A
}

void PORTA_UART0_init(void){
    SYSCTL_RCGCGPIO_R |= 0x01;          //0) Reloj Port_A
    GPIO_PORTA_AFSEL_R |= 0x03;         //1) Habilita Funcion Alterna - Pines A0 y A1
    GPIO_PORTA_DEN_R |= 0x03;           //3) Modo digital - Pines A0 y A1
    GPIO_PORTA_PCTL_R |= 0x00000011;    //4) Funcion Alterna para A0/A1 = UART
    GPIO_PORTA_AMSEL_R &= ~0x03;        //5) Deshabilita modo analogico de Pines A0 y A1
}
void UART0_init(void){
    PORTA_UART0_init();                         //Configura puertos
    SYSCTL_RCGCUART_R |=  0x00000001;           //0) Reloj UART_0 (p.505)
    while((SYSCTL_PRUART_R&0x01) == 0){};       //retardo de activacion
    UART0_CTL_R &= ~0x00000001;                 //1) Deshabilita el UART durante configuracion
                                                //2) Configura baud rate = 9600 bauds
    UART0_IBRD_R = 104;                         // Valor entero       IBRD = int(16,000,000 / (16 * 9,600)) = int(104.16666)
    UART0_FBRD_R = 11;                          // Valor fraccionario FBRD = round(0.1667 * 64) = 11
    UART0_LCRH_R = (UART_LCRH_WLEN_8|UART_LCRH_FEN);            //3) Configura tama�o de palabra = 8 bits y habilita el modo FIFO
    UART0_CC_R = (UART0_CC_R&~UART_CC_CS_M)+UART_CC_CS_PIOSC;   //4)  Fuente alterna de reloj ALTCLKCFG para el UART
    SYSCTL_ALTCLKCFG_R = (SYSCTL_ALTCLKCFG_R&~SYSCTL_ALTCLKCFG_ALTCLK_M)+SYSCTL_ALTCLKCFG_ALTCLK_PIOSC;
    //5) Fuente de reloj alterna = PIOSC
    UART0_CTL_R &= ~0x00000020;                 //6) Divisor de frecuencia = x16 para UART

    UART0_CTL_R |= 0x00000001;                  //8) Habilita UART_0
}

void UART0Rx_init(){
    UART0_init();
    UART0_CTL_R &= ~0x00000001;//Deshabilita

    UART0_ICR_R |= 0X00010000;       //7) Limpia la bandera de nivel de FIFO de recepcion
    UART0_IM_R |= 0X00010000;       //8) Desenmascara interrupcion
    NVIC_PRI1_R |= 0x0000E000;      //9) Habilita al NVIC a recibir interrupciones del UART0
    //UART0_IFLS_R = 0x08;            //10) Nivel de FIFORx = 1/8 de capacidad = 16 bits = Rafagas de 2 bytes

    UART0_CTL_R |= 0x00000001;//Habilita
}

void PORTJ_switch_init(){
    SYSCTL_RCGCGPIO_R |= 0x00000100; // activa el reloj para el puerto J
  //  Flancosdebajada = 0;                // inicializa el contador
  //*** Puerto J ***
    GPIO_PORTJ_DIR_R &= ~0x01;       //     PJ0 direccin entrada - boton SW1
    GPIO_PORTJ_DEN_R |= 0x01;        //     PJ0 se habilita
    GPIO_PORTJ_PUR_R |= 0x01;        //     habilita weak pull-up on PJ0
    GPIO_PORTJ_IS_R &= ~0x01;        //     PJ0 es sensible por flanco
    GPIO_PORTJ_IBE_R &= ~0x01;       //     PJ0 no es sensible a dos flancos
    GPIO_PORTJ_IEV_R &= ~0x01;       //     PJ0 detecta eventos de flanco de bajada
    GPIO_PORTJ_ICR_R = 0x01;         //     limpia la bandera 0
    GPIO_PORTJ_IM_R = 0x101;          //     Desenmascara interrupcion
}

//*************Flash_Init*****************
//Inicializa el controlador periferico Flash para realizar transferencias
//con direccion de origen en memoria Flash
void Flash_Init(void){
    while((FLASH_PP_R & 0x10000000)== 0);   //0) Revisa si el controlador periferico de flash permite transferencias de datos
    FLASH_DMASZ_R |= 0X01;                  //1) Tamano maximo de lectura de flash en (kB) 2KB = 2*(SZ +1) KB
    FLASH_DMAST_R = 0X00000020;             //2) Direccion de inicio de donde tiene acceso el uDMA para leer
}


//INTERRUPCIONES

void Timer3A_Handler(void){
        CounTransfer++;                //0) Transferencias completas
        if (CounTransfer>64){
            TIMER3_CTL_R &= 0X0;    //1) Desactiva el timmer para evitar mas interrupciones dado el modo periodico
        }
        TIMER3_ICR_R = 0x00000001; //2) Limpia la bandera de interrupcion fin de cuenta

        if((UDMA_ENASET_R & BIT2)==0){
            setRegular(2,1);          //3) Reconfigurar los parametros de transferencia
        }
}


//la interrupcion se levanta hasta que se llena MiTabla[]
//con la cantidad #TOTALBYTES no en cada transfer
void UART0_Handler(void){
    CounTransfer++;
    //Reconfigurar las estructuras
    uint8_t ping = 2;
    setRegular(8*4,ping);             //0) Configura los parametros de la transmision
    Ping_setAlternate((8*4)+128);           //1) Configura los parametros de la transmision en la estructua alterna
    UART0_DR_R = 0x02;
    UART0_ICR_R = 0X00010000;
}

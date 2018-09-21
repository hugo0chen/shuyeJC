#ifndef DRIVERSINIT_H_
#define DRIVERSINIT_H_

#include "msp430f2272.h"
#include "define.h"


//==================================主出从入====================================
#define MOSI_0 P3OUT &= ~BIT1
#define MOSI_1 P3OUT |= BIT1
//===================================SPI时钟端口================================
#define SCK_0 P3OUT &= ~BIT3
#define SCK_1 P3OUT |= BIT3
//===================================主入从出===================================
#define MISO P3IN & BIT2

inline void InitSysClk(void){
    // ACLK = LFXT1 = 32768Hz, MCLK = SMCLK = CALxxx_8MHZ = 8MHz
    //  //* External watch crystal on XIN XOUT is required for ACLK *//
    //
    //               MSP430F22x4
    //             -----------------
    //         /|\|              XIN|-
    //          | |                 | 32kHz
    //          --|RST          XOUT|-
    //            |                 |
    //            |             P2.0|-->ACLK = 32kHz
    //            |             P2.1|-->SMCLK = 8MHz
    //            |             P2.2|-->MCLK/10 = DCO/10
      if (CALBC1_1MHZ==0xFF)                  // If calibration constant erased
      {
        while(1);                               // do not load, trap CPU!!
      }
      DCOCTL = 0;                               // Select lowest DCOx and MODx settings
      BCSCTL1 = CALBC1_1MHZ;                    // Set DCO to 1MHz
      DCOCTL = CALDCO_1MHZ;
      BCSCTL1 |= DIVA_2;                       // ACLK/4
}

inline void InitTimerA(void){
	//timerA clock : aclk/8 = (32768 /4)/ 8 = 1024 (Hz)
	//default: up count mode, MC0
	TACTL = TASSEL_1 + ID_3 + MC_1 + TACLR;  
	TAR = 0;
	TACCR0 = 102;	//100ms interval time
}

inline void InitTimerB(void){
	//timerB clock : aclk/8 = (32768 /4)/ 8 = 1024 (Hz)
	TBCCR0 = 10;//10ms
	TBCTL = TBSSEL_1 + ID_3 + MC_1 + TACLR;
	TBCCTL0 = CCIE;
}
/*
 * Stauts LED   P1.6
 * Signal LED	P4.3
 * Alarm  LED   p4.4
 * Weight LEDs  P4.0 - P4.1 - P4.2 - P4.5 - P4.6 - P 4.7
 */
inline void init_led(void){
  	//status led P1.7
  	P1DIR |= BIT0 + BIT6 ;    // output direction
  	P1SEL &= ~(BIT0 + BIT6);   // select as I / O function
  	P1OUT |= BIT0 + BIT6;        
  	
  	P4DIR |= 0xFF; // output direction
  	P4OUT |= 0xFF; // initially all switch off
}
 /* 
  * Slide button   P1.4
  * Function       P1.5
  * ST/P		   P1.7
  * 
  */
 inline void init_button(void){
 	//buttons
 	P1IFG &= ~(BIT4 + BIT5 +BIT7);
  	P1DIR &= ~(BIT4 + BIT5 + BIT7); // input
  	P1IES |= BIT4 + BIT5 + BIT7;   // interrupt falling
  	P1REN |= BIT4 + BIT5 + BIT7;   // this line and the next 
  	P1OUT |= BIT4 + BIT5 + BIT7;   // enable the pullup resister
  	P1SEL &= ~(BIT4 + BIT5 + BIT7); // select as I/O function			
    P1IE  |= BIT4;  // enable swithcer interrupt
}

//ADS1230IPW
/*
 * ADS_CK		P1.2
 * ADS_D		P1.1
 * ADS_EN		P1.3
 * 
 */
inline void init_ex_adc(void){

  	P1DIR |= BIT2 | BIT3 ;    // output direction
  	P1OUT |= BIT2 + BIT3;     // enable the pullup resister
  	P1SEL &= ~(BIT2 + BIT3 ); // select as I / O function
  	P1OUT &= ~BIT3;           // Hold nPDWN High
  	P1OUT &= ~BIT2;
  	P1IFG &= ~( BIT1);
  	P1DIR &=~ BIT1; 	  // ads_Dout input direction
  	P1IES |= BIT1;        // interrupt falling
  	P1REN |= BIT1; 		  // this line and the next 
  	P1OUT |= BIT1; 		  // enable the pullup resister
  	P1SEL &= ~BIT1; 	  // select as I / O function	
  	
  	P2DIR |= BIT1; 		  // weighter power control
  	P2OUT &= ~BIT1; 	
}
/*
 * GDO0  P2.2
 * GDO2  P2.5
 * CSN   P3.0
 */
 inline void init_cc1101_gpio(void){
 	P2IES &= (BIT2 + BIT5); // IES = 0 ,上升沿设定IFG标志
  	P2DIR &= ~(BIT2 + BIT5); 
  	P2IFG &= ~(BIT2);
  	//P2IE |= BIT2;
  	//CS
  	P3DIR |= BIT0;
  	P3OUT |= BIT0; 	
 }
 inline void init_beep(void){
 	P2DIR |= BIT4;    // output direction
  	P2OUT &= ~BIT4;     // enable the pullup resister
 	
 }
/* MOSI  P3.1
 * MISO  P3.2
 * SCLK	 P3.3
*/
inline void init_spi(void){
	
	P3DIR |= (BIT1 + BIT3);
  	P3OUT |= (BIT1 + BIT3);
  	
  	P3DIR &=  ~BIT2;
}
/*
inline void init_adc(void){
   ADC10CTL0 = SREF_1 + ADC10SHT_3 + REFON  + ADC10ON + REF2_5V; 
   ADC10AE0 |= 0x01;                         // P2.0 ADC option select
}*/
inline void init_wdt(void){
  WDTCTL = WDT_ADLY_1000;                   // WDT 4s interval timer
  IE1 |= WDTIE;                             // Enable WDT interrupt
}
inline void _interrupt(void){
    IE1 &= ~WDTIE;                             // Enable WDT interrupt
}
inline void enable_wdt_interrupt(void){
	IE1 |= WDTIE;
}

inline void InitUCA0(void)
{
	P3SEL |= BIT4 + BIT5;                     // P3.4,5 = USCI_A0 TXD/RXD
	
	UCA0CTL1 |= UCSSEL_2;                     // SMCLK
  	UCA0BR0 = 208;                              // 1MHz 8 =115200， 26 -> 38400, 8Mhz 208-->38400
  	UCA0BR1 = 0;                              // 1MHz 115200
  	UCA0MCTL = UCBRS2 + UCBRS0;               // Modulation UCBRSx = 5
  	UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
}

/* MOSI  P3.1
 * MISO  P3.2
 * SCLK	 P3.3
*/
inline INT8U SPI_ExchangeByte(INT8U data){
	INT8U i,temp;
	
	SCK_0 ;
	for(i = 0; i < 8; i++){
		if(data&0x80){
			MOSI_1;
		}
		else{
			MOSI_0;
		}
		data <<= 1;
		SCK_1;
		temp <<= 1;
		if(MISO) temp++;  //MISO
		SCK_0 ;
	}
	return temp;
}
#endif /*DRIVERSINIT_H_*/

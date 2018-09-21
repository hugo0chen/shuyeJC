#ifndef TASK_H_
#define TASK_H_

#include <string.h>
#include "msp430f2272.h"

#define LIMITED_WEIGHT_DELTA  10
#define CAL_TIME_MINUTES	 30
#define LOW_BAT_VAL 		3300

// heart beat interval of timer B
#define HB_INTVL            2000
// bottle blink interval of timer B
#define BOT_BLINK_INTVL     200
// long pressing interval of timer A
#define L_PRESS_INTVL       20
// short pressing interval of timer A
#define S_PRESS_INTVL       1

// the flash memory address of weighter's iformation of ID and Channel
#define CTU_Addr            0x01000

// the flash memory address of weighter's sensor data (ADS1230)
#define FLASH_VALID			0x55
#define ADS_ADDR0G          0x01040
#define ADS_ADDR500G        0x01080//(ADS_ADDR0G + OFFSET_ADDR)

#define MAX_CALIBRATE_TIMES   10
#define WEIGHT_SENSOR_OFFSET  0
#define WIRE_PACKET_HEAD_1    0x55
#define WIRE_PACKET_HEAD_2    0xAA

#define FREE_DEST_ID		  0xEE
#define MAX_RETRY_TIMES		  5

#define MAX_WEIGHT_LEVEL			900 //glass bottle ,500ml, max weight = 853g
#define START_LOW_WEIGHT_LEVEL		100  //50 开始信号判断的重量低值
#define START_CONTI_TIMES			15

#define STOP_HIGH_WEIGHT_LEVEL		10  //10g 结束信号判断的重量高值
#define GAP_WEIGHT					20  //重新开始的前后重量差
#define EMPTY_BOTTLE_WEIGHT			30  //空液袋的重量

/*
	define NEW bottle with full water and Tube, total weight 
*/
#define PLASTIC_100ML_WEIGHT	100
#define PLASTIC_250ML_WEIGHT	250
#define PLASTIC_500ML_WEIGHT	500
#define GLASS_100ML_WEIGHT		100
#define GLASS_250ML_WEIGHT		250
#define GLASS_500ML_WEIGHT		500
#define DELTA_WEIGHT			30

#define PLASTIC_100ML  0
#define PLASTIC_250ML  1
#define PLASTIC_500ML  2
#define GLASS_100ML	   3
#define GLASS_250ML	   4
#define GLASS_500ML	   5

/*type类型*/

#define START	0x30	//输液开始
#define RUN		0x31	//输液进行中
#define STOP	0x32	//输液结束
#define BEAT	0x33	//输液单元心跳信号
#define W_REG	0x34	//输液单元配置
#define ALTOCAL_REPORT 0x43 //上报自动校准重量偏移值

#define MSG_LEN  9  // total length of message, first two octets are 0

// weight leds
#define LED51 (~BIT7)
#define LED50 (BIT7)
#define LED41 (~BIT6)
#define LED40 (BIT6)
#define LED31 (~BIT5)
#define LED30 (BIT5)
#define LED21 (~BIT2)
#define LED20 (BIT2)
#define LED11 (~BIT1)
#define LED10 (BIT1)
#define LED01 (~BIT0)
#define LED00 (BIT0)

#define WEIHGT_LED_1_ON P4OUT &= LED01	
#define WEIHGT_LED_2_ON P4OUT &= LED11
#define WEIHGT_LED_3_ON P4OUT &= LED21
#define WEIHGT_LED_4_ON P4OUT &= LED31
#define WEIHGT_LED_5_ON P4OUT &= LED41
#define WEIHGT_LED_6_ON P4OUT &= LED51
	
#define LED_WEIGHT_ON_6 \
		((((((P4OUT &= LED01) &= LED11) &= LED21) &= LED31) &= LED41) &= LED51)
#define LED_WEIGHT_ON_5 \
		((((((P4OUT &= LED01) &= LED11) &= LED21) &= LED31) &= LED41) |= LED50)
#define LED_WEIGHT_ON_4 \
		((((((P4OUT &= LED01) &= LED11) &= LED21) &= LED31) |= LED40) |= LED50)
#define LED_WEIGHT_ON_3 \
		((((((P4OUT &= LED01) &= LED11) &= LED21) |= LED30) |= LED40) |= LED50)
#define LED_WEIGHT_ON_2 \
		((((((P4OUT &= LED01) &= LED11) |= LED20) |= LED30) |= LED40) |= LED50)
#define LED_WEIGHT_ON_1 \
		((((((P4OUT &= LED01) |= LED10) |= LED20) |= LED30) |= LED40) |= LED50)
#define LED_WEIGHT_ON_0 \
		((((((P4OUT |= LED00) |= LED10) |= LED20) |= LED30) |= LED40) |= LED50)

// Type leds
#define LED_TYPE_BOTTLE_BLINK    P1OUT ^= BIT6
#define _TYPE_BOTTLE_ON       P1OUT &= ~BIT6
#define LED_TYPE_BOTTLE_OFF      P1OUT |= BIT6

#define LED_TYPE_BAG_BLINK    P1OUT ^= BIT0
#define _TYPE_BAG_ON       P1OUT &= ~BIT0
#define LED_TYPE_BAG_OFF      P1OUT |= BIT0

// sign led
#define LED_SIGN_BLINK      P4OUT ^= BIT3
#define _SIGN_ON         P4OUT &= ~BIT3
#define LED_SIGN_OFF        P4OUT |= BIT3
    
// alarm led
#define LED_ALARM_ON        P4OUT &= ~BIT4
#define LED_ALARM_OFF       P4OUT |= BIT4

// beeper
#define BEEP_ON             P2OUT |= BIT4
#define BEEP_OFF            P2OUT &= ~BIT4

// dial switcher
#define DIAL_SWITCH1_ON     IsClr((P3IN & BIT6))
#define DIAL_SWITCH2_ON     IsClr((P3IN & BIT7))

// led switcher
#define LED_SWITCH_ON       (!IsClr(P1IN & BIT4))
#define LED_SWITCH_OFF      (IsClr(P1IN & BIT4))
#define LED_SWITCH_INT      (!IsClr(P1IFG & BIT4))
#define LED_SWITCH_INT_D    (P1IE &= ~BIT4)
#define LED_SWITCH_INT_E    (P1IE |= BIT4)
#define LED_SWITCH_INT_C    (P1IFG &= ~BIT4)

// start button
#define START_BUTTON_DW     (IsClr(P1IN & BIT5))
#define START_BUTTON_UP     (!IsClr(P1IN & BIT5))
#define START_BUTTON_INT    (!IsClr(P1IFG & BIT5))
#define START_BUTTON_INT_D  (P1IE &= ~BIT5)
#define START_BUTTON_INT_E  (P1IE |= BIT5)
#define START_BUTTON_INT_C  (P1IFG &= ~BIT5)

// function button
#define FUNCT_BUTTON_DW     (IsClr(P1IN & BIT7))
#define FUNCT_BUTTON_UP     (!IsClr(P1IN & BIT7))
#define FUNCT_BUTTON_INT    (!IsClr(P1IFG & BIT7))
#define FUNCT_BUTTON_INT_D  (P1IE &= ~BIT7)
#define FUNCT_BUTTON_INT_E  (P1IE |= BIT7)
#define FUNCT_BUTTON_INT_C  (P1IFG &= ~BIT7)
// ads Dout
#define ADS_DOUT_DW     	(IsClr(P1IN & BIT1))
#define ADS_DOUT_UP     (!IsClr(P1IN & BIT1))
#define ADS_DOUT_INT    (!IsClr(P1IFG & BIT1))
#define ADS_DOUT_INT_D  (P1IE &= ~BIT1)
#define ADS_DOUT_INT_E  (P1IE |= BIT1)
#define ADS_DOUT_INT_C  (P1IFG &= ~BIT1)

#define ADS_CLk_SET() 	(P1OUT |= BIT2)
#define ADS_CLK_CLR() 	(P1OUT &= ~BIT2)
#define ADS_DOUT()	  	(P1IN & BIT1)

#define TIMER_A_START       (TACTL |= MC_1)
#define TIMER_A_STOP        (TACTL &= 0XFFCF)
#define TIMER_B_START       (TBCTL |= MC_1)
#define TIMER_B_STOP        (TBCTL &= 0XFFCF)
// RF receiving port
#define RF_GDO0_INT         (!IsClr(P2IFG & BIT2))
#define RF_GDO0_INT_D       (P2IE &= ~BIT2)
#define RF_GDO0_INT_E       (P2IE |= BIT2)
#define RF_GDO0_INT_C       (P2IFG &= ~BIT2)

#define SET_FLG(f)     ((f) = 1)
#define CLR_FLG(f)     ((f) = 0)


typedef enum{
	BAG,
	BOTTLE,
	PLASTIC_100ML_TYPE,
	PLASTIC_250ML_TYPE,
	PLASTIC_500ML_TYPE,
	GLASS_100ML_TYPE,
	GLASS_250ML_TYPE,
	GLASS_500ML_TYPE,
	UNKNOWN_TYPE,
}eContainerType;

enum SYS_STATE {
	SLEEP_STATE = 0,
	WAKE_WDT_STATE = 1,
	CAL_STATE = 2,
	BOND_STATE = 3,
	SY_STATE = 4,
	TYPE_STATE = 5,
	KEY_SCAN = 6,
	GET_WEIGHT_STATE = 7
};
	
enum SY_OP{
	bNONE = 0,
	bSTART = 1,
	bSTOP = 2,
	bRUN = 3
};

typedef struct {
	INT8U channel_freq_num;
	INT8U channel_getted ;
	INT8U b_node_configured;
	INT8U req_ack;
	ulong req_tick_time;
	INT8U re_try_times;
} BOND_PARA;

typedef struct {
	ulong	zero_calibrated_value ;
	ulong 	half_kilo_calibrated_value;
	INT8U offset_value;
/*	INT8U delt_direction; // 1 偏大，需要减，2 偏小，需要增加 0 不增减。*/
}WEIGHT_STRUCT;

typedef struct {
	INT8U MyId ; 
	INT8U  MyChannel;
	eContainerType  ContainerType;
    WEIGHT_STRUCT weight_def;
	INT16U	empty_weight;
} DEV_PARA;

typedef struct{
	INT8U retry_times;
	INT8U dest_id;
	ulong wait_resp_tickTime;
	bool resp_success; // TRUE: success ;
} SHAKE_HAND;


extern BOND_PARA bond_para;
extern DEV_PARA dev_para;
extern INT8U ADS1230_notified_flag;
extern SHAKE_HAND shake_hand;

void ads1230_Calibrate(void);
void beep_beep(INT8U beep_times);
void send_packet(INT8U *buf, INT8U len);
INT8U heartBeat(void);
void start(INT8U dest, INT8U r_time);
void run(INT8U dest, INT8U r_time);
void stop(INT8U dest);
INT8U process_rx_packet(INT8U *rbuf, INT8U rlen, enum SYS_STATE status);
INT8U buf_comp(INT8U* str1, INT8U* str2, INT8U len);
void usart_send_bytes(INT8U *buf, INT8U len);
INT8U change_shuye_state(void);

inline INT8U IsSet(INT8U flag){
	return (1 == flag);
}

inline INT8U IsClr(INT8U flag){
	return (0 == flag);
}

inline void LED_SIGN_ON(void){        
 if(LED_SWITCH_ON) P4OUT &= ~BIT3;
}

inline void LED_TYPE_BAG_ON(void){
  if(LED_SWITCH_ON) P1OUT &= ~BIT0;
}

inline void LED_TYPE_BOTTLE_ON(void){
	if(LED_SWITCH_ON) P1OUT &= ~BIT6;
}

inline void LedAllSwitchOff(void){
	P4OUT = 0xff;
	P1OUT |= BIT6;
	P1OUT |= BIT0;
}
 
inline void SendBlink(void){
    Delay_nms(50);
    LED_SIGN_OFF;
}

inline void AllWeightLedBlinkOnce(void){
    LED_WEIGHT_ON_6;
    Delay_nms(50);
    LED_WEIGHT_ON_0;
}

inline void AllWeightLedBlinkTwice(void){
    LED_WEIGHT_ON_6;
    Delay_nms(50);
    LED_WEIGHT_ON_0; 
    Delay_nms(100);
    LED_WEIGHT_ON_6;  
    Delay_nms(50);
    LED_WEIGHT_ON_0;
}

inline void AllWeightLedWaterFlow(void){	
	WEIHGT_LED_1_ON;
	Delay_nms(50);
	WEIHGT_LED_2_ON;
	Delay_nms(50);
	WEIHGT_LED_3_ON;
	Delay_nms(50);
	WEIHGT_LED_4_ON;
	Delay_nms(50);
	WEIHGT_LED_5_ON;
	Delay_nms(50);
	WEIHGT_LED_6_ON;
	Delay_nms(50);
	LED_WEIGHT_ON_0;
}

inline void disable_wdt(void){
	WDTCTL = WDTPW + WDTHOLD;  
	IE1 &= ~WDTIE;
}
inline void WEIGHTER_POWER_OFF(void){
	P2OUT &= ~BIT1;	
}
inline void WEIGHTER_POWER_ON(void){
	P2OUT |= BIT1;	
}
inline void ADS_POWER_OFF(void){
	P1OUT &= ~BIT3;	
}
inline void ADS_POWER_ON(void){
	P1OUT |= BIT3;	
}


#endif /*TASK_H_*/

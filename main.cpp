/*
 * 编辑日期 2018/08/03
 * 修改内容：
 * 	自动称重的判断，称重器周期获取重量信息，周期间隔4秒钟
 * 	心跳周期保持不变，60秒
 * 	当获取重量信息大于100g时，连续获取时间1分钟，则提示开始
 * 	根据重量和采集周期，算出输液速度，用输液速度判断出结束时间。
 * 编辑日期 2018/09/10
 * 液袋类型指示灯，在采集信息发送时指示
 * 
 2018/09/11
1、开关按键只有结束功能，之前版本是开始和结束之间切换。

2、在输液过程中，液袋类型指示灯显示当前的液袋类型，伴随信号灯一起闪烁

3、从无输液袋挂上，到有输液袋挂上，判断液袋类型，开始输液

4、输液过程中，输液袋的切换，判断新液袋类型，开始输液

5、液袋类型切换按键无效。

6、开始信号发出时的重量指示LED用于指示6种对应的液袋类型
	LED 1   100ml Plastic
   LED 1+2  250ml Plastic
   LED 1~3  500ml Plastic
   LED 1~4  100ml Glass
   LED 1~5  250ml Glass
   LED 1~6  500ml Glass

  2018/09/20
  *增加无线通信的频偏问题纠正，方法：发送时计数加，接受数据时清零，如果该标志增加到 某个阀值，重新设置cc1101的channel
  *去掉对串口的使用，不需要这个module。usart_send_bytes(),initUCA0();
  *函数命名全部使用小写，main.c task.c flash.c
  *测试功能，并针对发现的问题修复bug，主要测试不同输液袋的输液切换是否正确。
  *重新开始时，需要清除上次记录的剩余时间，remaining_time_count = 0；
  *修改GAP_WEIGHT 由40 修改为20，因为250ml玻璃空瓶重228，100ml的玻璃初始219，差距GAP_WEIGHT过大也会导致液袋类型判断出错。
  *
  2018/09/21
  *
 */

#include "define.h"
#include "task.h"
#include "flash.h"
#include "timer.h"
#include "driversInit.h"
#include "stdlib.h"
#include "cc1101.h"

enum SYS_STATE fsm_sta = SLEEP_STATE;
enum SY_OP bSyState = bNONE;
volatile INT8U wdt_times_cnt = 0;
ulong keyScan_tick;
INT16U wireless_commnucation_flag = 0;

static void read_configured_data_from_flash(void){
	INT8U temp[6] = {0};
	
	read_flash(CTU_Addr, temp, 5);
    if(temp[0] == FLASH_VALID){
	    dev_para.MyId = temp[2];
	    dev_para.MyChannel = temp[3];
	    bond_para.b_node_configured = temp[4];
    }
    read_flash(ADS_ADDR0G, temp, 4);
    if(temp[0] == FLASH_VALID){
	    dev_para.weight_def.zero_calibrated_value = temp[1];
	    dev_para.weight_def.zero_calibrated_value =  (dev_para.weight_def.zero_calibrated_value << 8) + temp[2];
	    dev_para.weight_def.zero_calibrated_value =  (dev_para.weight_def.zero_calibrated_value << 8) + temp[3];
    	
	    read_flash(ADS_ADDR500G, temp, 4);
	    if(temp[0] == FLASH_VALID){
		    dev_para.weight_def.half_kilo_calibrated_value = temp[1] ;
		    dev_para.weight_def.half_kilo_calibrated_value = (dev_para.weight_def.half_kilo_calibrated_value << 8)+ temp[2];
		    dev_para.weight_def.half_kilo_calibrated_value = (dev_para.weight_def.half_kilo_calibrated_value << 8) + temp[3];
	    	
	    	if(dev_para.weight_def.half_kilo_calibrated_value > dev_para.weight_def.zero_calibrated_value){	    	
	  			dev_para.weight_def.offset_value = WEIGHT_SENSOR_OFFSET;
	    	}	
	    }
    }
}

void init_drivers(void){
	disable_wdt();
	_DINT(); 
  	InitSysClk(); //1MHZ
  	read_configured_data_from_flash();
  	init_led();
  	init_button();
  	init_cc1101();
	cc1101_set_channel(dev_para.MyChannel);	
  	init_ex_adc();
  	init_beep();
  	/*InitUCA0();*/
   	InitTimerA(); 
  	InitTimerB();
 	init_wdt();	
}

void start_menu(void){
	LED_WEIGHT_ON_6;
	_TYPE_BOTTLE_ON;
	_TYPE_BAG_ON;
	_SIGN_ON;
	LED_ALARM_ON;
	beep_beep(1);
	LED_WEIGHT_ON_0;
	LED_TYPE_BOTTLE_OFF;
	LED_TYPE_BAG_OFF;
	LED_SIGN_OFF;
	LED_ALARM_OFF;	
}

void bond_led_indicate(void){
	static ulong cal_time_tick;
	static INT8U led_status = 0;
	
	if(timeout(cal_time_tick, 500)){
		if(led_status == 0){
			led_status = 1;
			if(LED_SWITCH_ON) 
				LED_WEIGHT_ON_6;
		}
		else {
			led_status = 0;
			LED_WEIGHT_ON_0;
		}
		cal_time_tick = local_ticktime();
	}
}

void bond_result_callback(INT8U res){
	if(res == 0){//成功
		INT8U tbuf[5];
		INT8U readFromFlash[5];
		INT8U i;
		
		dev_para.MyChannel = dev_para.MyChannel - 1;
		tbuf[0] = FLASH_VALID;
		tbuf[1] = 0;
		tbuf[2] = dev_para.MyId;
		tbuf[3] = dev_para.MyChannel;
		tbuf[4] = bond_para.b_node_configured;
		for( i = 0; i < 3; i++){
			write_Seg(CTU_Addr, tbuf, sizeof(tbuf));
			read_flash(CTU_Addr, readFromFlash, sizeof(readFromFlash));
			if(buf_comp(tbuf, readFromFlash, sizeof(tbuf)) == 0){
				break;	
			}
		}
		if(i < 3){
			if(LED_SWITCH_ON) 
				LED_WEIGHT_ON_6;
			LED_WEIGHT_ON_0;
			beep_beep(2);
		}else{
			LED_WEIGHT_ON_0;
			beep_beep(4);
			bond_para.b_node_configured = 0;
		}	
	}else{
		LED_WEIGHT_ON_0;
		beep_beep(4);	
	}
}


#define FREQ_GET_RETRY_TIMES	5
INT8U sweep_freq_ask(void){
	static ulong sweep_tick_time;
	INT8U freq_req_send_buf[MSG_LEN - 2] = {0x5A, 0x5A, 1,2,3,4,5};
	INT8U i;
	
	if(timeout(sweep_tick_time, 300)){
		if(bond_para.channel_freq_num < MAX_CHANNEL_NUM){
			if(dev_para.MyChannel > (MAX_CHANNEL_NUM - 1)){
				dev_para.MyChannel = 0;
			}
			cc1101_set_channel(dev_para.MyChannel);

			for(i = 0; i < FREQ_GET_RETRY_TIMES; i++){
				send_packet(freq_req_send_buf, sizeof(freq_req_send_buf));
				Delay_nms(20);
				if(bond_para.channel_getted == 1)
					break;
			}
			dev_para.MyChannel++;			
		}
		else{
			bond_para.channel_freq_num = 0;
			return 1;
		}
		bond_para.channel_freq_num++;
		sweep_tick_time = local_ticktime();
	}
	return 0;
}

#define MAX_DEV_REQ_TIMES   5
#define BOND_WAIT_TIME  	30000
#define BOND_REQ_GAP_TIME   300
INT8U dev_request(void){
	INT8U  tbuf[MSG_LEN - 2] = {W_REG,0};
	if(bond_para.req_ack== 1){
		if(timeout(bond_para.req_tick_time, BOND_WAIT_TIME)){
			bond_para.re_try_times = 0;
			bond_para.req_ack = 0;
			return 1;
		}
	}
	else{ 
		if(bond_para.re_try_times <= MAX_DEV_REQ_TIMES){
			if(timeout(bond_para.req_tick_time, BOND_REQ_GAP_TIME)){
				send_packet(tbuf, sizeof(tbuf));
				bond_para.re_try_times++;
				bond_para.req_tick_time = local_ticktime();
			}
		}
		else{
			bond_para.re_try_times = 0;
			bond_para.req_ack  = 0;
			return 1;
		}
	}
	return 0;
}

void enter_lp_mode(void){
#ifdef DEBUG_SUPPORT
INT8U sleep_string[] = "sleep!";
usart_send_bytes(sleep_string, sizeof(sleep_string));
#endif
	_DINT(); 
	ADS_POWER_OFF();
	LedAllSwitchOff();
	cc1101_enter_power_down_mode();
	TIMER_B_STOP;
	TIMER_A_STOP;
	ADS_DOUT_INT_D;
	RF_GDO0_INT_D;
	P1IFG = 0;
	P2IFG = 0;
	FUNCT_BUTTON_INT_E;
	START_BUTTON_INT_E;
	init_wdt();
	__bis_SR_register(LPM3_bits + GIE); 
}
/*
void ContainerType_change_op(void){
	if (dev_para.ContainerType == BAG){
   		dev_para.ContainerType = BOTTLE;
   		LED_TYPE_BAG_OFF;
   		LED_TYPE_BOTTLE_ON();
   	}else{
   		dev_para.ContainerType = BAG;
   		LED_TYPE_BAG_ON();
   		LED_TYPE_BOTTLE_OFF;
	}
	Delay_nms(1000);  // keep led status time	
	fsm_sta = SLEEP_STATE;
}
*/
void band_op(void){
	bond_led_indicate();
	if(bond_para.channel_getted == 0){
		if(sweep_freq_ask()){
			bond_result_callback(1);
			fsm_sta = SLEEP_STATE;	
		}
	}else{
		if(dev_request()){
			bond_result_callback(1);
			fsm_sta = SLEEP_STATE;
		}
	}
}
	
void process_rf_op(void){
	INT8U  rbuf[32] = {0}; 
	INT8U  rlen;
	
	TBCCTL0 &= ~CCIE;
	rlen = CC1101RecPacket(rbuf);
	TBCCTL0 = CCIE;
	if(rlen){
	    wireless_commnucation_flag = 0;
		switch (process_rx_packet(rbuf, rlen, fsm_sta))
		{
			case 1: // channel right
				bond_para.channel_freq_num = 0;
				bond_para.req_ack = 0;
				bond_para.channel_getted = 1;					
				break;
			case 2:// bed bond ok
				bond_para.b_node_configured = 1;
				bond_para.req_tick_time = 0;
				bond_para.re_try_times = 0;
				//init_wdt();
				bond_result_callback(0);
				fsm_sta = SLEEP_STATE;
				break;
			case 3: //req_ack
				bond_para.req_ack = 1;
				bond_para.req_tick_time = local_ticktime();
				break;
			case 4:
				shake_hand.resp_success = true;
				break;
			default:
				break;	
		}
	}
    CC1101SetTRMode( RX_MODE );
}

#define OLD_DEST_TRY_TIMES  3
#define MAXT_TRY_TIMES      6
#define COM_TIMEOUT         100

int main(void){
	init_drivers();	
	start_menu();
	
	while(1){
	    if(wireless_commnucation_flag > COM_TIMEOUT){
	        INT8U temp[6] = {0};

	        read_flash(CTU_Addr, temp, 5);
            if(temp[0] == FLASH_VALID){
                dev_para.MyChannel = temp[3];
            }
            cc1101_set_channel(dev_para.MyChannel);
            wireless_commnucation_flag = 0;
	    }
		switch(fsm_sta){
			case GET_WEIGHT_STATE:
				if(change_shuye_state() == 0){
					fsm_sta = SLEEP_STATE;
				}
				break;
			case KEY_SCAN:
				if(timeout(keyScan_tick, 10000)){
					fsm_sta = SLEEP_STATE;
				}
				break;
			case CAL_STATE:
				ads1230_Calibrate();
				fsm_sta = SLEEP_STATE;
				break;
			case BOND_STATE:
				band_op();
				break;
			case WAKE_WDT_STATE:
				if((bond_para.b_node_configured == 1)){
					if(bSyState == bNONE){ 
						heartBeat();
						fsm_sta = SLEEP_STATE;
						break;
					}else{
						fsm_sta = SY_STATE;
						goto RUN_LABEL;
					}
				}else{
					fsm_sta = SLEEP_STATE;
					break;
				}
			case SY_STATE:
				if(timeout(shake_hand.wait_resp_tickTime, 300)){
					RUN_LABEL:
					shake_hand.retry_times++;
					if(shake_hand.resp_success == false){
						if(shake_hand.retry_times <= OLD_DEST_TRY_TIMES){
							switch(bSyState){
								case bSTART:
									start(shake_hand.dest_id, shake_hand.retry_times);
									break;
								case bSTOP:
									stop(shake_hand.dest_id);
									break;
								case bRUN:
									run(shake_hand.dest_id, shake_hand.retry_times);
									break;
								default:
									break;
							}
						}else if(shake_hand.retry_times > OLD_DEST_TRY_TIMES &&
								shake_hand.retry_times <= MAXT_TRY_TIMES){
							switch(bSyState){
								case bSTART:
									start(FREE_DEST_ID, shake_hand.retry_times);
									break;
								case bSTOP:
									stop(FREE_DEST_ID);
									break;
								case bRUN:
									run(FREE_DEST_ID, shake_hand.retry_times);
									break;
								default:
									break;
							}
						}else{//失败
							if((bSyState == bSTART) || (bSyState == bSTOP)){
								if(bSyState == bSTART){//返回之前状态 
									bSyState = bNONE;
								}
								else if(bSyState == bSTOP){ 
									bSyState = bRUN;
								}										
							}	
							shake_hand.retry_times = 0;
							shake_hand.resp_success = false;			
							fsm_sta = SLEEP_STATE;
						}	
					}else{//通信成功
						if(bSyState == bSTART){
							bSyState = bRUN;
							beep_beep(1);
						}else if(bSyState == bSTOP){
							bSyState = bNONE;						
							/*dev_para.ContainerType = BAG;*/
							dev_para.ContainerType =  UNKNOWN_TYPE;
							beep_beep(2);	
						}
						shake_hand.retry_times = 0;
						shake_hand.resp_success = false;
						fsm_sta = SLEEP_STATE;
					}
					shake_hand.wait_resp_tickTime = local_ticktime();
				}
				break;
			/*case TYPE_STATE:
				ContainerType_change_op();
				break;*/
			case SLEEP_STATE:
				enter_lp_mode();
				break;
			default:
				break;	
		}	
	}
}

#define HEART_COUNT	15 //heartbeat period 60s
#define RUN_COUNT	5  //run period 20s

#pragma vector = WDT_VECTOR  //4s周期 WDT
__interrupt void watchdog_timer(void){
	wdt_times_cnt++;
	if(bSyState == bRUN){
		if(wdt_times_cnt >= RUN_COUNT){ 
			wdt_times_cnt = 0;
			fsm_sta = WAKE_WDT_STATE;
			TIMER_B_START;
			RF_GDO0_INT_E;	
			__bic_SR_register_on_exit(LPM3_bits);		
		}		
	}else{
		if(wdt_times_cnt >= HEART_COUNT){ 
			wdt_times_cnt = 0;
			fsm_sta = WAKE_WDT_STATE;
			TIMER_B_START;
			RF_GDO0_INT_E;
			__bic_SR_register_on_exit(LPM3_bits);
		}else {
			fsm_sta = GET_WEIGHT_STATE;
			__bic_SR_register_on_exit(LPM3_bits);
		}
	}
	
}
INT8U pressedKeyNum  = 0; //按下的是功能键还是开关键

// interrupts for the function keys
#pragma vector=PORT1_VECTOR
__interrupt void PORT1_ISR (void){
	__bic_SR_register_on_exit(LPM3_bits);
	
	if (FUNCT_BUTTON_INT){
		FUNCT_BUTTON_INT_D; 
		FUNCT_BUTTON_INT_C; 
		pressedKeyNum = 1;
		TIMER_A_START;
		TACCTL0 = CCIE;  
		TIMER_B_START;
		fsm_sta = KEY_SCAN;
		disable_wdt();
		reset_local_ticktime();
		keyScan_tick = local_ticktime();
	}
	else if (START_BUTTON_INT){
		START_BUTTON_INT_D; 
		START_BUTTON_INT_C;  
		pressedKeyNum = 2;
		TIMER_A_START;  
		TACCTL0 = CCIE;
		TIMER_B_START;
		fsm_sta = KEY_SCAN;
		disable_wdt();
		reset_local_ticktime();
		keyScan_tick = local_ticktime();
	}
	else if (LED_SWITCH_INT){
		LED_SWITCH_INT_D;   
		LED_SWITCH_INT_C;   
		LedAllSwitchOff(); 
  		LED_SWITCH_INT_E;   
  		fsm_sta = SLEEP_STATE;
	}
	else if(ADS_DOUT_INT){
		ADS_DOUT_INT_D;  
		ADS_DOUT_INT_C;  	
		ADS1230_notified_flag = 1;
	}
}

extern INT8U tx_enable;
extern INT8U tx_done ;

// interrupt for receiving data
#pragma vector=PORT2_VECTOR
__interrupt void PORT2_ISR (void){
	if(RF_GDO0_INT){
		RF_GDO0_INT_D;
		RF_GDO0_INT_C;
		if(tx_enable == 1){
			tx_done = 1;	
		}
		else{
			process_rf_op();
		}
	    RF_GDO0_INT_E;	
	}
}

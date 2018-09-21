#include "define.h"
#include "timer.h"
#include "task.h"
#include "cc1101.h"

#define INTERVAL_TIME 10 // 10ms

static ulong  _local_time_tick;

extern enum SYS_STATE fsm_sta;
extern enum SY_OP bSyState;
extern INT8U pressedKeyNum;


ulong local_ticktime(void){
	return _local_time_tick	;
}

void reset_local_ticktime(void){
	_local_time_tick = 0;
}

bool timeout(ulong last_time, ulong ms){
	if(_local_time_tick > last_time)
		return (bool)((_local_time_tick - last_time) > ms);
	else if(last_time >_local_time_tick )
		return (bool)((last_time - _local_time_tick) > ms);
	else {
		return false;
	}
}

#pragma vector = TIMERB0_VECTOR
__interrupt void Timer_B(void){	 
	if(_local_time_tick > 0xFFFFFFF0)
		_local_time_tick = 0;
	_local_time_tick += INTERVAL_TIME;
}

// 100ms peroid
#pragma vector = TIMERA0_VECTOR
__interrupt void Timer_A (void){
	static INT8U   FunctionKeyPressCnt   = 0;
	static INT8U   SKeyPressCnt   = 0;
	
	if (FUNCT_BUTTON_DW){
		if (++FunctionKeyPressCnt >= L_PRESS_INTVL){
			FunctionKeyPressCnt = 0;
			fsm_sta = CAL_STATE;
			reset_local_ticktime();
			TIMER_A_STOP;TACCTL0 &= ~CCIE;
		}
	}
	else if (START_BUTTON_DW){
		if (++SKeyPressCnt >= L_PRESS_INTVL){
			SKeyPressCnt = 0;			
			fsm_sta = BOND_STATE;
			RF_GDO0_INT_E;CC1101SetIdle();
			reset_local_ticktime();
			bond_para.channel_getted = 0;
			TIMER_A_STOP;TACCTL0 &= ~CCIE;
		}
	}
	else if ((pressedKeyNum == 1)&&FUNCT_BUTTON_UP){
		if ((S_PRESS_INTVL <= FunctionKeyPressCnt) && (FunctionKeyPressCnt < L_PRESS_INTVL)){
			if(bond_para.b_node_configured == 1){
				/*fsm_sta = TYPE_STATE;*/
				fsm_sta = SLEEP_STATE;
			}
		}
		else{
			fsm_sta = SLEEP_STATE;
		}
		pressedKeyNum = 0;
		FunctionKeyPressCnt = 0;
		TIMER_A_STOP;
		TACCTL0 &= ~CCIE;
	}
	else if ( (pressedKeyNum == 2) && START_BUTTON_UP){
		if (SKeyPressCnt < L_PRESS_INTVL){
			if(bond_para.b_node_configured == 1){
				fsm_sta = SY_STATE;
				RF_GDO0_INT_E;
				bSyState = bSTOP;
				/*if(bSyState == bNONE){
					bSyState = bSTART;
				}else if(bSyState == bRUN){
					bSyState = bSTOP;	
				}*/
			}
		}
		pressedKeyNum = 0;
		SKeyPressCnt = 0;
		TIMER_A_STOP;
		TACCTL0 &= ~CCIE;
	}
	else{
		fsm_sta = SLEEP_STATE;
	}
}



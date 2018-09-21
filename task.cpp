#include "define.h"
#include "task.h"
#include "flash.h"
#include "timer.h"
#include "algorithm"
#include "cc1101.h"

extern INT16U wireless_commnucation_flag;
SHAKE_HAND shake_hand = {0, FREE_DEST_ID, 0, false};
DEV_PARA dev_para = {0,0,UNKNOWN_TYPE, {0, 0, 0}, 0};
BOND_PARA bond_para = {0, 0, 0, 0, 0, 0};
INT8U ADS1230_notified_flag = 0;
static INT16U TransactionID = 0;
INT8U auto_cal_times_count = 0;

static INT16U buf_for_cal_speed[10];
static float speed;
static ulong remaining_time_count; //20秒的次数
/*
TYPE:	Plastic
	volume weight
	100ml	37
	250ml	39
	500ml	45
	
TYPE:	Glass	
	volume weight
	100ml	119
	250ml	228
	500ml	353
*/
const INT16U ConstEmptyWeight[] = {37, 39, 45, 119, 228, 353};


INT8U buf_comp(INT8U* str1, INT8U* str2, INT8U len){
	INT8U i;
	
	for(i = 0; i < len; i++){
		if(str1[i] != str2[i])
			break;	
	}
	if(i == len)
		return 0;
	return 1;	
}

 void weight_led(INT8U num){
 	if(LED_SWITCH_ON) {
	 	LED_WEIGHT_ON_0;
	 	switch(num){
	 		case 1:
	 			WEIHGT_LED_1_ON;break;
	 		case 2:
	 			WEIHGT_LED_2_ON;break;
	 		case 3:
	 			WEIHGT_LED_3_ON;break;
	 		case 4:
	 			WEIHGT_LED_4_ON;break;
	 		case 5:
	 			WEIHGT_LED_5_ON;break;
	 		case 6:
	 			WEIHGT_LED_6_ON;break;
	 		default:
	 			break;
	 	}
 	}	
 }
 
INT8U adc_con_finish = 0;
INT16U get_battery_value(void){
	INT16U voltage;
	ulong sum = 0;
	INT8U ctl0 = ADC10CTL0;
	INT8U ae0 = ADC10AE0;
	
	ADC10CTL0 = SREF_1 + ADC10SHT_3 + REFON  + ADC10ON + REF2_5V; 
   	ADC10AE0 |= 0x01;                         // P2.0 ADC option select*/
	ADC10CTL0 |= ADC10IE;
	adc_con_finish = 0;
	ADC10CTL0 |= ENC + ADC10SC;    
 	while(!adc_con_finish);
 	while(ADC10CTL1&ADC10BUSY);
 	sum = ADC10MEM;
	ADC10CTL0 &= ~ADC10IE;
	ADC10CTL0 &= ~(ENC + ADC10SC);
 	ADC10CTL0 = ctl0;
 	ADC10AE0  = ae0;
 	voltage = (INT16U)((sum * 2500)/1024);
 	return voltage;
}

#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR (void)
{
	adc_con_finish = 1;
}

INT16U multi_adc_average(INT8U times){
	ulong sum = 0;
	INT16U get_val;
	if(times == 0) return 0;
	for(INT8U i = 0; i < times; i++){
		sum += get_battery_value();
	}	
	get_val = (INT16U)(sum/times)*2.1;
	return get_val;
}

/*void usart_send_bytes(INT8U *buf, INT8U len){
	for (INT8U i = 0; i < len; i++){
		UCA0TXBUF = buf[i];
		while(!(IFG2&UCA0TXIFG));
		IFG2 &= ~UCA0TXIFG;
	}
}*/

void cal_led_indicate(void){
	static ulong cal_time_tick;
	static INT8U led_num = 1;
	
	if(timeout(cal_time_tick, 200)){
		if(led_num <= 6){
			weight_led(led_num++);	
			if(led_num == 7) 
				led_num = 1;
		}
		cal_time_tick = local_ticktime();
	}
}

void beep_beep(INT8U beep_times){
	INT16U i;
	
	for(INT8U j = 0; j < beep_times; j++){
		for(i = 0; i< 100; i++){
			BEEP_ON;
			Delay_nms(1);
			BEEP_OFF;
		}
		Delay_nms(200);
	}
}

/*******************************************************************************
 **********************            ADS1230    **********************************
 ******************************************************************************/
void ads1230_start_calibrate(void){
	unsigned long temp = 0;
	
	ADS1230_notified_flag = 0;
   	ADS_DOUT_INT_E;  				// enable interrupt of P1.1
	while( !ADS1230_notified_flag); // todo timeout
	_DINT();
	ADS_CLK_CLR();
	
	for(int i = 0; i < 24; i++){
		ADS_CLk_SET();
		Delay_nms(1);
		ADS_CLK_CLR();
		
		if(ADS_DOUT()){
			temp = (temp << 1) | 0x01; 
		}
		else{
			temp = temp << 1;	
		}
		Delay_nms(1);
	}
	//25th clock
	ADS_CLk_SET();
	Delay_nms(1);
	ADS_CLK_CLR();
	Delay_nms(1);
	//26th clock
	ADS_CLk_SET();
	Delay_nms(1);
	ADS_CLK_CLR();
	Delay_nms(1);
	_EINT();
	ADS_DOUT_INT_D; 
	ADS1230_notified_flag = 0;
	
}
void ads_delay(INT16U _delay_time){
	for(INT16U j = _delay_time; j > 0; j--)
		for(INT16U i = 0; i < 10; i++);	
}
static unsigned long read_weightSensor(void) {
	unsigned long temp;
	
	_DINT();
	ADS_CLK_CLR();
	temp = 0;
	for(int i = 0; i < 20; i++)	{
		ADS_CLk_SET();
		ads_delay(1);
		temp = temp << 1;
		ADS_CLK_CLR();	
		if(ADS_DOUT()){
			temp++; 
		}
		ads_delay(1);
	}
	ADS_CLk_SET();
	ads_delay(1);
	ADS_CLK_CLR();
	ads_delay(3);
	_EINT();
	return (temp&0x0FFFFF); //20bit adc
}
#define OFFSET_VALUE 0x0F0000
INT8U ads1230_sample_data(ulong*data, INT8U times, INT8U type){
	INT8U i, valid_data_num = 0 ;
	ulong temp_weight;
	ulong sampled_data[MAX_CALIBRATE_TIMES] = {0};   
   	ulong sum_weight = 0;
   		
   if((times > MAX_CALIBRATE_TIMES) || (times < 3)){
   		return 1;
   }
  
   	for( i = 0; i < times; i++){
   		ADS1230_notified_flag = 0;
   		ADS_DOUT_INT_E;  	
   		while(ADS1230_notified_flag == 0);
   		temp_weight = read_weightSensor();
   		// add
   		if(temp_weight & 0x0F0000){
   			temp_weight = OFFSET_VALUE	- (0x0FFFFF - temp_weight);
   		}else{
   			temp_weight += OFFSET_VALUE;
   		}
   		// end add
   		if(temp_weight != 0xFFFFF){
   			valid_data_num++;
   			sampled_data[i] = temp_weight;
   		}
   		ADS1230_notified_flag = 0;  
   		sum_weight += sampled_data[i];
   		if(type == 1)	
   			cal_led_indicate();	
   	}
   	*data = (sum_weight / valid_data_num);
   return 0;
}

static INT8U ads1230_DoCalibrate(INT16U addr){
	INT8U  buf_flash[5];
	ulong data;
	INT8U readFromFlash[5];
	INT8U i;
	
   	if((addr != ADS_ADDR0G) && (addr != ADS_ADDR500G)){
   		return 1;
   	}
   	ads1230_sample_data(&data, 6, 1);
   	buf_flash[0] = FLASH_VALID;
   	buf_flash[1] = (INT8U)(data >> 16);
   	buf_flash[2] = (INT8U)(data >> 8);
   	buf_flash[3] = (INT8U)data;
   	
   	/*usart_send_bytes(buf_flash, 4);*/

   	if(addr == ADS_ADDR0G){
   		dev_para.weight_def.zero_calibrated_value = data;   		
   	}
   	else{
   		dev_para.weight_def.half_kilo_calibrated_value = data;
   	}
	for(i = 0; i < 3; i++){
		write_Seg(addr, buf_flash, 4);
		read_flash(addr, readFromFlash, 4);
		if(buf_comp(buf_flash, readFromFlash, 4) == 0){
			return 0;
		}
	}
   	return 1;
}

void calibrate_success_indicate(void){
	if(LED_SWITCH_ON)
	    LED_WEIGHT_ON_6;
	beep_beep(2);
	LED_WEIGHT_ON_0;
}

void ads1230_Calibrate(void){
	INT8U ret = 0xff;
	
	WEIGHTER_POWER_ON();
	ADS_POWER_ON();
	ads1230_start_calibrate();
	//0g
	if(DIAL_SWITCH1_ON){
      	ret = ads1230_DoCalibrate(ADS_ADDR0G);
  		if(0 == ret){
  			calibrate_success_indicate();
  		}
  	}
  	//500g
  	else if(DIAL_SWITCH2_ON){
  	  	ret = ads1230_DoCalibrate(ADS_ADDR500G);
  		if(0 == ret){
  			calibrate_success_indicate();
  			//dev_para.weight_def.weight_ratio = (dev_para.weight_def.half_kilo_calibrated_value - dev_para.weight_def.zero_calibrated_value)/500;
  			dev_para.weight_def.offset_value = WEIGHT_SENSOR_OFFSET;
  		}
  	}
  	WEIGHTER_POWER_OFF();
	ADS_POWER_OFF();
}
 //ret = 1 小于0
 //ret = 2, 大于0
INT8U read_weight(INT16U* weight_g, INT8U cal_indicate){
	ulong get_weight_adc = 0;
	INT16U temp_weight;
	
	WEIGHTER_POWER_ON();
	ADS_POWER_ON();
	ads1230_start_calibrate();
	
	ads1230_sample_data(&get_weight_adc, 6, 0);
	if(get_weight_adc < dev_para.weight_def.zero_calibrated_value){
		temp_weight = (( dev_para.weight_def.zero_calibrated_value - get_weight_adc )*500/ 
				(dev_para.weight_def.half_kilo_calibrated_value - dev_para. weight_def.zero_calibrated_value));
		if(cal_indicate == 1){
			dev_para.weight_def.half_kilo_calibrated_value -= dev_para.weight_def.zero_calibrated_value - get_weight_adc;
			dev_para.weight_def.zero_calibrated_value = get_weight_adc;
			temp_weight = 0;
		}
	}else{
		temp_weight = ((get_weight_adc - dev_para.weight_def.zero_calibrated_value )*500/ 
				(dev_para.weight_def.half_kilo_calibrated_value -dev_para. weight_def.zero_calibrated_value));
		if(cal_indicate == 1){
			if((temp_weight < LIMITED_WEIGHT_DELTA) && (temp_weight > 0)){
				dev_para.weight_def.half_kilo_calibrated_value += get_weight_adc - dev_para.weight_def.zero_calibrated_value;
				dev_para.weight_def.zero_calibrated_value = get_weight_adc;
				temp_weight = 0;
			}	
		}
	}
	*weight_g = temp_weight;

	WEIGHTER_POWER_OFF();
	ADS_POWER_OFF();
    return 0;
}
 
static void weightLed_flush(INT16U weight){
	weight  = weight - dev_para.empty_weight;
	if(LED_SWITCH_ON) {
		if(weight >= 875){
		   LED_WEIGHT_ON_6;
		}
		else if((weight >= 625) && (weight < 875)){
		   LED_WEIGHT_ON_5;
		}
		else if((weight >= 375)&&(weight < 625)){
		   LED_WEIGHT_ON_4;
		}
		else if((weight >= 175)&&(weight < 375)){
		   LED_WEIGHT_ON_3;
		}
		else if((weight >= 75)&&(weight < 175)){
		   LED_WEIGHT_ON_2;
		}
		else{
		   LED_WEIGHT_ON_1;
		}
		/*else{
		   LED_WEIGHT_ON_0;
		}*/
	}
}

 void send_packet(INT8U *buf, INT8U len){
 	INT8U send_buf[MSG_LEN] = {WIRE_PACKET_HEAD_1, WIRE_PACKET_HEAD_2, 0};
	if ((MSG_LEN-2) != len){
		return;
	}
	LED_SIGN_ON();
	memcpy(&send_buf[2], buf, len);
	CC1101SendPacket(send_buf, MSG_LEN, BROADCAST);
	Delay_nms(30);
	LED_SIGN_OFF;
	wireless_commnucation_flag++;
}


INT8U process_rx_packet(INT8U *rbuf, INT8U rlen, enum SYS_STATE status){
	INT8U i;
	if(rlen != MSG_LEN)
		return 0;
	// aa 55 + 
	if((rbuf[0] == WIRE_PACKET_HEAD_2)&&((rbuf[1] == WIRE_PACKET_HEAD_1))){
		if(status == BOND_STATE){
				// sweep check;
			if((rbuf[2] == 0x5A)&&(rbuf[3] == 0x5A)){
				for( i = 4; i < MSG_LEN; i++){
					if(rbuf[i] != (MSG_LEN-i))
						break;	
				}
				if(i == MSG_LEN){
					return 1;	
				}
			}
			// bed bond
			else if(rbuf[2] == 0x34){
					if(rbuf[8] == 0x12){
						dev_para.MyId = rbuf[4] ;
						rbuf[8] = 0x01;
						send_packet(&rbuf[2], (MSG_LEN - 2) );
						return 2;
					}
					else if(rbuf[8] == 0x11){
						return 3;	
					}
			}
		}
		else if((status == SY_STATE) || (status == WAKE_WDT_STATE)){
			if(rbuf[4] == dev_para.MyId){
				if((rbuf[2] == START)|| (rbuf[2] == STOP) ||(rbuf[2] == RUN)){
					shake_hand.dest_id = rbuf[3];
				}
				return 4;
			}
		}
	}
	return 0;
}
extern enum SYS_STATE fsm_sta ;
extern enum SY_OP bSyState;

static INT16U preWeightArray[4]; //记录4组历史重量数据

void mem_16u_move(INT16U* dst, INT16U* src, INT8U len, INT16U newData){
	INT8U i ;
	for(i = 0; i < len; i++){
		dst[i] = src[i];	
	}
	dst[i] = newData;
}

void mem_16u_set(INT16U* src, INT16U data, INT8U len){
	for(INT8U i = 0; i < len; i++){
		src[i] = data;
	}	
}
INT8U bag_type(INT16U weight)
{
	if(weight > ( GLASS_500ML_WEIGHT + ConstEmptyWeight[GLASS_500ML] - DELTA_WEIGHT )){
		dev_para.ContainerType = GLASS_500ML_TYPE;
		dev_para.empty_weight = ConstEmptyWeight[GLASS_500ML];
	}else if(weight > ( PLASTIC_500ML_WEIGHT + ConstEmptyWeight[PLASTIC_500ML] - DELTA_WEIGHT )){
		dev_para.ContainerType = PLASTIC_500ML_TYPE;
		dev_para.empty_weight = ConstEmptyWeight[PLASTIC_500ML];
	}else if(weight > ( GLASS_250ML_WEIGHT + ConstEmptyWeight[GLASS_250ML] - DELTA_WEIGHT )){
		dev_para.ContainerType = GLASS_250ML_TYPE;
		dev_para.empty_weight = ConstEmptyWeight[GLASS_250ML];
	}else if(weight > ( PLASTIC_250ML_WEIGHT + ConstEmptyWeight[PLASTIC_250ML] - DELTA_WEIGHT )){
		dev_para.ContainerType = PLASTIC_250ML_TYPE;
		dev_para.empty_weight = ConstEmptyWeight[PLASTIC_250ML];
	}else if(weight > ( GLASS_100ML_WEIGHT + ConstEmptyWeight[GLASS_100ML] - DELTA_WEIGHT )){
		dev_para.ContainerType = GLASS_100ML_TYPE;
		dev_para.empty_weight = ConstEmptyWeight[GLASS_100ML];
	}else{// if(weight > ( PLASTIC_100ML_WEIGHT + ConstEmptyWeight[PLASTIC_100ML] - DELTA_WEIGHT)){
		dev_para.ContainerType = PLASTIC_100ML_TYPE;
		dev_para.empty_weight = ConstEmptyWeight[PLASTIC_100ML];
	}
	return 0;
}
eContainerType container_type_show(void)
{
    /*  Bottle Type Indicate */
    if (dev_para.ContainerType == GLASS_500ML_TYPE ||
        dev_para.ContainerType == GLASS_250ML_TYPE ||
        dev_para.ContainerType == GLASS_100ML_TYPE){
        LED_TYPE_BAG_OFF;
        LED_TYPE_BOTTLE_ON();
        return BOTTLE;
    }else if(dev_para.ContainerType ==  PLASTIC_500ML_TYPE ||
        dev_para.ContainerType == PLASTIC_250ML_TYPE ||
        dev_para.ContainerType == PLASTIC_100ML_TYPE){
        LED_TYPE_BAG_ON();
        LED_TYPE_BOTTLE_OFF;
        return BAG;
    }else{
        LED_TYPE_BOTTLE_ON();
        LED_TYPE_BAG_ON();
        return UNKNOWN_TYPE;
    }
}
void start(INT8U dest, INT8U r_time){
	/*static INT16U temp_weight;*/
	INT8U  tbuf[MSG_LEN - 2] = {0};
	static INT8U tempContainerType;
	
	if(bond_para.b_node_configured == 1){
		if(r_time == 1){
		    /*read_weight(&temp_weight, 0);
			mem_16u_move( preWeightArray, &preWeightArray[1], 3, temp_weight);*/
			mem_16u_set(buf_for_cal_speed, 0, sizeof(buf_for_cal_speed));
			remaining_time_count = 0;
			tempContainerType = container_type_show();
		}
	
		TransactionID = 0;
		tbuf[0] = START;
		tbuf[1] = dest;
		tbuf[2] = dev_para.MyId;
		tbuf[3] = (INT8U)(preWeightArray[3] >> 8);
		tbuf[4] = (INT8U)preWeightArray[3];
		tbuf[5] = (INT8U)tempContainerType;
		/*tbuf[5] = (INT8U)dev_para.ContainerType;*/
		send_packet(tbuf, sizeof(tbuf));
	
		/*WeightLedFlush(temp_weight);*/
		switch(dev_para.ContainerType){
			case PLASTIC_100ML_TYPE:
				LED_WEIGHT_ON_1;
				break;
			case PLASTIC_250ML_TYPE:
				LED_WEIGHT_ON_2;
				break;
			case PLASTIC_500ML_TYPE:
				LED_WEIGHT_ON_3;
				break;
			case GLASS_100ML_TYPE:
				LED_WEIGHT_ON_4;
				break;
			case GLASS_250ML_TYPE:
				LED_WEIGHT_ON_5;
				break;
			case GLASS_500ML_TYPE:
				LED_WEIGHT_ON_6;
				break;
			default:
				break;		
		}
	}
}

void stop(INT8U dest){
	INT8U  tbuf[MSG_LEN - 2] = {0};
	mem_16u_set(preWeightArray, 0, sizeof(preWeightArray));
	mem_16u_set(buf_for_cal_speed, 0, sizeof(buf_for_cal_speed));
	if(bond_para.b_node_configured == 1){
		tbuf[0] = STOP;
		tbuf[1] = dest;
		tbuf[2] = dev_para.MyId;
		send_packet(tbuf, sizeof(tbuf));
	}
}
/*
	重新开始信号判断
	前两组数据小于开始的重量标准，后两组数据大于开始的重量标准
*/
INT8U weight_higher(void)
{
	if(((preWeightArray[0] > STOP_HIGH_WEIGHT_LEVEL )||(preWeightArray[1] > STOP_HIGH_WEIGHT_LEVEL ))
		&&(preWeightArray[2] > preWeightArray[0] )&&(preWeightArray[3] > preWeightArray[1] ))
	{
		if( ((preWeightArray[2] - preWeightArray[0]) > GAP_WEIGHT) && 
			((preWeightArray[3] - preWeightArray[1]) > GAP_WEIGHT) )
		{
			bag_type(preWeightArray[3]);
			shake_hand.retry_times = 0;
			bSyState = bSTART;
			return 1;
		}				
	}
	return 0;
}

INT8U weight_lower(void)
{
	if( (preWeightArray[2] > STOP_HIGH_WEIGHT_LEVEL )&&
		(preWeightArray[3] > STOP_HIGH_WEIGHT_LEVEL )&&
		(preWeightArray[0] < preWeightArray[2] )&&
		(preWeightArray[1] < preWeightArray[3] ))
	{
		if( ((preWeightArray[0] - preWeightArray[2]) > GAP_WEIGHT) && 
			((preWeightArray[1] - preWeightArray[3]) > GAP_WEIGHT) )
		{
			bag_type(preWeightArray[3]);
			shake_hand.retry_times = 0;
			bSyState = bSTART;
			return 1;
		}				
	}
	return 0;
}

INT8U restart_check(INT16U new_weight)
{
	INT8U ret;
	/*上次输液未结束，液袋取走，获取的重量连续为0，不能重新开始，
	 * 采取方法：将最近一次非零数据排在靠后位置，保证前两个数据一直非0*/
	if((preWeightArray[0] != 0) && (preWeightArray[1] == 0) &&
	        (preWeightArray[2] == 0) && (preWeightArray[3] == 0) ){
	    preWeightArray[2] = preWeightArray[0];
	}else if((preWeightArray[0] != 0) && (preWeightArray[1] == 0) &&
            (preWeightArray[2] != 0) && (preWeightArray[3] == 0)){
	    preWeightArray[1] = preWeightArray[0];
	    preWeightArray[3] = preWeightArray[2];
	}
	mem_16u_move( preWeightArray, &preWeightArray[1], 3, new_weight);
	switch(dev_para.ContainerType )
	{
		case GLASS_500ML_TYPE:
		case GLASS_250ML_TYPE:
		case GLASS_100ML_TYPE:
			if(new_weight < dev_para.empty_weight)
			{
				ret = weight_lower();
			}else{
				ret = weight_higher();				
			}
			break;
		case PLASTIC_500ML_TYPE:
		case PLASTIC_250ML_TYPE:
		case PLASTIC_100ML_TYPE:
			ret = weight_higher();
			break;
		default:
			ret = weight_higher();
			break;
	}
	return ret;
}
/*
	计算剩余时间
	重量信息偏移过大时，小于输液的最低重量或是高于输液的最大重量，是否需要将重量信息发送出去
	合理重量的范围界定，最大，最小
*/
INT8U stopStatus_check(INT16U new_weight)
{
	float delta_weight[10];
	
	if(new_weight > dev_para.empty_weight && new_weight < MAX_WEIGHT_LEVEL)
	{
		mem_16u_move(buf_for_cal_speed, &buf_for_cal_speed[1], 9, new_weight);
		INT8U j = 0;
		for(INT8U i = 9; i > 0; i--) 
		{
			if(buf_for_cal_speed[i-1] >= buf_for_cal_speed[i])
			{
				delta_weight[j++] = buf_for_cal_speed[i-1] - buf_for_cal_speed[i];
			}
		}
		float sum_delta = 0;
		for( INT8U i = 0; i < j ; i++)
		{
				sum_delta += delta_weight[i];
		}
		speed = sum_delta / j;
		if(speed > 0)
		{
			remaining_time_count = (ulong)((new_weight - dev_para.empty_weight )/speed);
		}			
	}
	else 
	{
		if( remaining_time_count > 0)
		{
			remaining_time_count--;					
		}
		else
		{
			fsm_sta = SLEEP_STATE;
			bSyState = bSTOP;
			return 1;
		} 
	}
	return 0;
}

void run(INT8U dest, INT8U r_time){
	INT8U  tbuf[MSG_LEN - 2] = { 0 };
	INT16U temp_weight = 0;
	static INT16U bat_voltage = 0;
	
	auto_cal_times_count = 0;
	if(bond_para.b_node_configured == 1){
		if(r_time == 1){
		    container_type_show();
			/*  battery check */
			bat_voltage = multi_adc_average(10);
		 	if(bat_voltage < LOW_BAT_VAL) 
		 	{
				LED_ALARM_ON;
			}else{
				LED_ALARM_OFF;	
			}	
			/*get total weight*/
		 	read_weight(&temp_weight, 0);
			/*net weight cal*/
			weightLed_flush(temp_weight);
		 	
			TransactionID++;
			if( restart_check(temp_weight))
			{
				return;	
			}			
			if(stopStatus_check(temp_weight))
			{
				return;
			}
		}
		/* send Run frame by wireless  */
		tbuf[0] = RUN;
		tbuf[1] = dest;
		tbuf[2] = dev_para.MyId;
		tbuf[3] = (INT8U)(temp_weight >> 8);
		tbuf[4] = (INT8U)temp_weight;
		tbuf[5] = (INT8U)(TransactionID >> 8) ;
		tbuf[6] = (INT8U)TransactionID;
		send_packet(tbuf, sizeof(tbuf));
	}
}

INT8U heartBeat(void)
{
	INT8U tbuf[MSG_LEN - 2] = {0};
 	INT16U bat_voltage = 0;
 	INT16U delta_weight = 0;
 	bat_voltage = multi_adc_average(5);
 	
 	if(bat_voltage < LOW_BAT_VAL) 
 		LED_ALARM_ON;
 	else
 		LED_ALARM_OFF;
 		
 	read_weight(&delta_weight, 0);
 	if((delta_weight > 0) && (delta_weight < LIMITED_WEIGHT_DELTA)){
	 	if(auto_cal_times_count++ > CAL_TIME_MINUTES){
	 		INT16U temp_weight = 0;
	 		read_weight(&temp_weight, 1); //校准动作
	 		auto_cal_times_count = 0;
	 		tbuf[0] = ALTOCAL_REPORT;
	 		tbuf[1] = 0;
			tbuf[2] = dev_para.MyId;
			tbuf[3] = (INT8U)(delta_weight >> 8);
			tbuf[4] = (INT8U)delta_weight;
			send_packet(tbuf, sizeof(tbuf));
			return 0;
	 	}
 	}else{
 		auto_cal_times_count = 0;	
 	}
 	
	tbuf[0] = BEAT;
	tbuf[1] = 0;
	tbuf[2] = (INT8U)dev_para.MyId;
	tbuf[3] = (INT8U)(bat_voltage >> 8);
	tbuf[4] = (INT8U)bat_voltage;
	send_packet(tbuf, sizeof(tbuf));
	return 0;
}

/*
	continous weight > START_LOW_WEIGHT_LEVEL
	coutinous count time START_CONTI_TIMES
*/
INT8U change_shuye_state(void){
	static INT8U start_weight_count;
	INT16U temp_weight = 0;

	read_weight(&temp_weight, 0);
	if( temp_weight > START_LOW_WEIGHT_LEVEL ) {
		start_weight_count++;
		if(start_weight_count > START_CONTI_TIMES){
			start_weight_count = 0;
			if(bond_para.b_node_configured == 1){
				bag_type(temp_weight);
				mem_16u_move( preWeightArray, &preWeightArray[1], 3, temp_weight);
				fsm_sta = SY_STATE;
				TIMER_B_START;
				RF_GDO0_INT_E;
				bSyState = bSTART;
				return 1;
			}			
		}
	}else{
		start_weight_count = 0;
	}
	return 0;
}

/*
================================================================================
Description : This module contains the low level operations for CC1101
================================================================================
*/
#include "cc1101.h"
#include "task.h"
#include "driversInit.h"

INT8U tx_enable = 0;
INT8U tx_done = 0;
const INT8U Channel_Table[MAX_CHANNEL_NUM] = {0x01, 0x05, 0x09, 0x0D, 0x11, 0x15, 0x19, 0x1D, 0x21, 0x25};

//10, 7, 5, 0, -5, -10, -15, -20, dbm output power, 0x12 == -30dbm
//const INT8U PaTabel[] = { 0xc0, 0xC8, 0x84, 0x60, 0x34, 0x1D, 0x0E, 0x12};
INT8U PaTabel[] = { 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0};
// Sync word qualifier mode = 30/32 sync word bits detected 
// CRC autoflush = false 
// Channel spacing = 199.951172 
// Data format = Normal mode 
// Data rate = 2.00224 
// RX filter BW = 58.035714 
// PA ramping = false 
// Preamble count = 4 
// Whitening = false 
// Address config = No address check 
// Carrier frequency = 400.199890 
// Device address = 0 
// TX power = 10 
// Manchester enable = false 
// CRC enable = true 
// Deviation = 5.157471 
// Packet length mode = Variable packet length mode. Packet length configured by the first byte after sync word 
// Packet length = 255 
// Modulation format = GFSK 
// Base frequency = 399.999939 
// Modulated = true 
// Channel number = 1 

//  125kbaud
#define SetNum  28
static const INT8U CC1101InitData[SetNum][2]= 
{
  {CC1101_IOCFG0,      0x46},// assert when sync word has been sent and received
  {CC1101_IOCFG2,      0x0E},// carrier sense,  high if RSSI level above threshold,
  //cleard when entering IDLE mode
  {CC1101_FIFOTHR,     0x47},//set the threshold of RXFIFO and TXFIFO
  {CC1101_PKTCTRL0,    0x05},//whitening off,crc check,variable packet length mode
  {CC1101_FSCTRL1,     0x07},//RX��ʹ�õ�����IF��subtracted from RX signal
  {CC1101_FREQ2,       0x10},// ���û�׼Ƶ�� 433MHZ
  {CC1101_FREQ1,       0xB1},
  {CC1101_FREQ0,       0x3B},
  {CC1101_MDMCFG4,     0xF9},//���ƽ��������
  {CC1101_MDMCFG3,     0x93}, 
  {CC1101_MDMCFG2,     0x13},
  {CC1101_DEVIATN,     0x24},//���ƽ������������ 
  {CC1101_MCSM1,       0x33},//cca mode,
  {CC1101_MCSM0,       0x18},//main radio control state machine configuration
  {CC1101_FOCCFG,      0x1D},//frequency ƫ�Ʋ�������
  {CC1101_BSCFG,       0x1C},//wake up radio control
  {CC1101_AGCCTRL2,    0x03},//Ƶ�ʺϳ���У׼
  {CC1101_AGCCTRL1,    0x00},// frequency synthesizer calibration
  {CC1101_AGCCTRL0,    0xA0},
  {CC1101_WORCTRL,     0xFB},
	{CC1101_FREND1,      0xB6},
	{CC1101_FSCAL3,      0xEA},
	{CC1101_FSCAL2,      0x2A},
  {CC1101_FSCAL1,      0x00},//various test settings
  {CC1101_FSCAL0,      0x1F},
  {CC1101_TEST0,       0x09},		
	{CC1101_TEST1,       0x35},
	{CC1101_TEST2,       0x81}, 
};
/*read a byte from the specified register*/
INT8U CC1101ReadReg( INT8U addr );

/*Read some bytes from the rigisters continously*/
void CC1101ReadMultiReg( INT8U addr, INT8U *buff, INT8U size );

/*Write a byte to the specified register*/
void CC1101WriteReg( INT8U addr, INT8U value );

/*Flush the TX buffer of CC1101*/
void CC1101ClrTXBuff( void );

/*Flush the RX buffer of CC1101*/
void CC1101ClrRXBuff( void );

/*Get received count of CC1101*/
INT8U CC1101GetRXCnt( void );

/*Reset the CC1101 device*/
void CC1101Reset( void );

/*Write some bytes to the specified register*/
void CC1101WriteMultiReg( INT8U addr, INT8U *buff, INT8U size );

/*
================================================================================
Function : CC1101WORInit( )
    Initialize the WOR function of CC1101
INPUT    : None
OUTPUT   : None
================================================================================
*/
void  CC1101WORInit( void )
{
    CC1101WriteReg(CC1101_MCSM0,0x18); // when calibrate,and timeout after xosc start
    CC1101WriteReg(CC1101_WORCTRL,0x78); //Wake On Radio Control,enable RC calibration
    CC1101WriteReg(CC1101_MCSM2,0x00);
		CC1101WriteReg(CC1101_WOREVT1,0x8C);//Event0 timeout [15:8]
    CC1101WriteReg(CC1101_WOREVT0,0xA0); //Event0 timeout [7:0]
	
	CC1101WriteCmd( CC1101_SWORRST );
}
/*
================================================================================
Function : CC1101ReadReg( )
    read a byte from the specified register
INPUT    : addr, The address of the register
OUTPUT   : the byte read from the rigister
================================================================================
*/
INT8U CC1101ReadReg( INT8U addr )
{
    INT8U i;
    CC_CSN_LOW( );
    SPI_ExchangeByte( addr | READ_SINGLE);
    i = SPI_ExchangeByte( 0xFF );
    CC_CSN_HIGH( );
    return i;
}
/*
================================================================================
Function : CC1101ReadMultiReg( )
    Read some bytes from the rigisters continously
INPUT    : addr, The address of the register
           buff, The buffer stores the data
           size, How many bytes should be read
OUTPUT   : None
================================================================================
*/
void CC1101ReadMultiReg( INT8U addr, INT8U *buff, INT8U size )
{
    INT8U i, j;
    CC_CSN_LOW( );
    SPI_ExchangeByte( addr | READ_BURST);   // burst read, that is read continously
    for( i = 0; i < size; i ++ )
    {
        for( j = 0; j < 20; j ++ );
        *( buff + i ) = SPI_ExchangeByte( 0xFF );
    }
    CC_CSN_HIGH( );
}
/*
================================================================================
Function : CC1101ReadStatus( )
    Read a status register
INPUT    : addr, The address of the register
OUTPUT   : the value read from the status register
================================================================================
*/
INT8U CC1101ReadStatus( INT8U addr )
{
    INT8U i;
    CC_CSN_LOW( );
    SPI_ExchangeByte( addr | READ_BURST);
    i = SPI_ExchangeByte( 0xFF );
    CC_CSN_HIGH( );
    return i;
}
/*
================================================================================
Function : CC1101SetTRMode( )
    Set the device as TX mode or RX mode
INPUT    : mode selection
OUTPUT   : None
================================================================================
*/
void CC1101SetTRMode( TRMODE mode )
{
	CC1101SetIdle();
    if( mode == TX_MODE ){
        CC1101WriteReg(CC1101_IOCFG0,0x46);// invert output ,active low
        CC1101WriteCmd( CC1101_STX );
    }
    else if( mode == RX_MODE ){
        CC1101WriteReg(CC1101_IOCFG0,0x46);//
        CC1101WriteCmd( CC1101_SRX );
    }
}
/*
================================================================================
Function : CC1101WriteReg( )
    Write a byte to the specified register
INPUT    : addr, The address of the register
           value, the byte you want to write
OUTPUT   : None
================================================================================
*/
void CC1101WriteReg( INT8U addr, INT8U value ){
    CC_CSN_LOW( );
    SPI_ExchangeByte( addr );
    SPI_ExchangeByte( value );
    CC_CSN_HIGH( );
}
/*
================================================================================
Function : CC1101WriteMultiReg( )
    Write some bytes to the specified register
INPUT    : addr, The address of the register
           buff, a buffer stores the values
           size, How many byte should be written
OUTPUT   : None
================================================================================
*/
void CC1101WriteMultiReg( INT8U addr, INT8U *buff, INT8U size )
{
    INT8U i;
    CC_CSN_LOW( );
    SPI_ExchangeByte( addr | WRITE_BURST );//����д
    for( i = 0; i < size; i ++ )
    {
			SPI_ExchangeByte( *( buff + i ) );
    }
    CC_CSN_HIGH( );
}
/*
================================================================================
Function : CC1101WriteCmd( )
    Write a command byte to the device
INPUT    : command, the byte you want to write
OUTPUT   : None
================================================================================
*/
void CC1101WriteCmd( INT8U command )
{
    CC_CSN_LOW( );
    SPI_ExchangeByte( command );
    CC_CSN_HIGH( );
}
/*
================================================================================
Function : CC1101Reset( )
    Reset the CC1101 device
INPUT    : None
OUTPUT   : None
================================================================================
*/
void CC1101Reset( void )
{
    INT8U x;

    CC_CSN_HIGH( );
    CC_CSN_LOW( );
    CC_CSN_HIGH( );
    for( x = 0; x < 100; x ++ );
    CC1101WriteCmd( CC1101_SRES );
}
/*
================================================================================
Function : CC1101SetIdle( )
    Set the CC1101 into IDLE mode
INPUT    : None
OUTPUT   : None
================================================================================
*/
void CC1101SetIdle( void )
{
    CC1101WriteCmd(CC1101_SIDLE);
}
void CC1101Calibrate(void){
	CC1101WriteCmd(CC1101_SCAL);
}
void cc1101_enter_power_down_mode(void){
	
	CC1101WriteReg(CC1101_IOCFG0, 0x2F);
	CC1101WriteReg(CC1101_IOCFG1, 0x2F);
	CC1101WriteReg(CC1101_IOCFG2, 0x2F);
	CC1101SetIdle();//MUST BE IDLE MODE
	CC1101WriteCmd(CC1101_SPWD);
}
/*
================================================================================
Function : CC1101ClrTXBuff( )
    Flush the TX buffer of CC1101
INPUT    : None
OUTPUT   : None
================================================================================
*/
void CC1101ClrTXBuff( void )
{
    CC1101SetIdle();//MUST BE IDLE MODE
    CC1101WriteCmd( CC1101_SFTX );
}
/*
================================================================================
Function : CC1101ClrRXBuff( )
    Flush the RX buffer of CC1101
INPUT    : None
OUTPUT   : None
================================================================================
*/
void CC1101ClrRXBuff( void )
{
    CC1101SetIdle();//MUST BE IDLE MODE
    CC1101WriteCmd( CC1101_SFRX );
}

INT8U cc1101_set_channel(INT8U ch_num){
	//CC1101Init();
	if(ch_num < MAX_CHANNEL_NUM){
		CC1101SetIdle();
		CC1101WriteReg(CC1101_CHANNR, Channel_Table[ch_num]);
		return 1;
	}
	return 0;	
}
/*
================================================================================
Function : CC1101SendPacket( )
    Send a packet
INPUT    : txbuffer, The buffer stores data to be sent
           size, How many bytes should be sent
           mode, Broadcast or address check packet
OUTPUT   : None
================================================================================
*/
void CC1101SendPacket( INT8U *txbuffer, INT8U size, TX_DATA_MODE mode )
{
	INT16U CC1101_OVT;
    INT8U address;
    INT8U wait_times = 0;
    
    //���Ӵ˴��룬����RX MODE�������Ѻ��һ�����߷���ʧ��
    CC1101SetTRMode(RX_MODE);
    //CCA
    while(1){
	    CC1101SetTRMode(RX_MODE); 
	    Delay_nms(2);
	    if(P2IN&BIT5 == 1){
	    	CC1101ClrRXBuff( );
	    }else{
	    	break;	
	    }
	    if(wait_times++ > 250)
	    	break;
    }
    
    tx_enable = 1;
	tx_done = 0;
	CC1101SetIdle();
    if( mode == BROADCAST )             { address = 0; }
    else if( mode == ADDRESS_CHECK )    { address = CC1101ReadReg( CC1101_ADDR ); }

    CC1101ClrTXBuff( );
    
    if( ( CC1101ReadReg( CC1101_PKTCTRL1 ) & 0x03 ) != 0 )  // address check mode
    {
        CC1101WriteReg( CC1101_TXFIFO, size + 1 );
        CC1101WriteReg( CC1101_TXFIFO, address );
    }
    else             // no address check
    {
        CC1101WriteReg( CC1101_TXFIFO, size );
    }
	
    CC1101WriteMultiReg( CC1101_TXFIFO, txbuffer, size );
	
    CC1101SetTRMode( TX_MODE ); 
    CC1101_OVT = 0;
    while(!tx_done){
    	Delay_nms(1);
    	if(CC1101_OVT++ > 200)
    		break;	
    }
    tx_enable = 0;
    tx_done = 0;
    Delay_nms(5);
    CC1101SetTRMode(RX_MODE);  
}
/*
================================================================================
Function : CC1101GetRXCnt()
    Get received count of CC1101
INPUT    : None
OUTPUT   : How many bytes has been received
================================================================================
*/
INT8U CC1101GetRXCnt( void ){
    return ( CC1101ReadStatus( CC1101_RXBYTES )  & BYTES_IN_RXFIFO );
}
/*
================================================================================
Function : CC1101SetAddress( )
    Set the address and address mode of the CC1101
INPUT    : address, The address byte
           AddressMode, the address check mode
OUTPUT   : None
================================================================================
*/
void CC1101SetAddress( INT8U address, ADDR_MODE AddressMode)// set address check configuration of received packages
{
    INT8U btmp = CC1101ReadReg( CC1101_PKTCTRL1 ) & ~0x03;
    CC1101WriteReg(CC1101_ADDR, address);//device address setting
		
    if     ( AddressMode == BROAD_ALL )     {}
    else if( AddressMode == BROAD_NO  )     { btmp |= 0x01; }
    else if( AddressMode == BROAD_0   )     { btmp |= 0x02; }
    else if( AddressMode == BROAD_0AND255 ) { btmp |= 0x03; }   
}
/*
================================================================================
Function : CC1101SetSYNC( )
    Set the SYNC bytes of the CC1101
INPUT    : sync, 16bit sync 
OUTPUT   : None
================================================================================
*/
void CC1101SetSYNC( INT16U sync )
{
    CC1101WriteReg(CC1101_SYNC1, 0xFF & ( sync>>8 ) );
    CC1101WriteReg(CC1101_SYNC0, 0xFF & sync ); 
}
/*
================================================================================
Function : CC1101RecPacket( )
    Receive a packet
INPUT    : rxBuffer, A buffer store the received data
OUTPUT   : 1:received count, 0:no data
================================================================================
*/
INT8U CC1101RecPacket( INT8U *rxBuffer )
{
	INT8U temp_rx_buf[32];
    INT8U status[2];
    INT8U pktLen;
    INT16U addr;
    
	Delay_nms(10);
    if ( CC1101GetRXCnt( ) != 0 ){
        pktLen = CC1101ReadReg(CC1101_RXFIFO);           // Read length byte
		if( pktLen == 0 ){
			CC1101ClrRXBuff( );
			return 0; 
		}
        //address check
        if( ( CC1101ReadReg( CC1101_PKTCTRL1 ) & 0x03 ) != 0 ){
            addr = CC1101ReadReg(CC1101_RXFIFO);  // get address
            if(addr != CC1101_ADDRESS){
            	CC1101ClrRXBuff( );
            	return 0;
            }
			pktLen --; 
        }
        if(pktLen == MSG_LEN){
        	// Pull data
        	CC1101ReadMultiReg(CC1101_RXFIFO, rxBuffer, MSG_LEN); 
        }
        else{
			if(pktLen <= 32)
        		CC1101ReadMultiReg(CC1101_RXFIFO, temp_rx_buf, pktLen); 	
        }
         // Read  CRC status bytes
        CC1101ReadMultiReg(CC1101_RXFIFO, status, 2);       
        CC1101ClrRXBuff( );
        if( status[1] & CRC_OK ){
        	if(pktLen == MSG_LEN) 
       			return MSG_LEN;
       	}
    }
    return 0;                               
}
/*
================================================================================
Function : CC1101Init( )
    Initialize the CC1101, User can modify it
INPUT    : None
OUTPUT   : None
================================================================================
*/
INT8U CC1101Init( void ){
    volatile INT8U version_pn, i, j;

    CC1101Reset( );    
    for( i = 0; i < SetNum; i++ ){
		CC1101WriteReg( CC1101InitData[i][0], CC1101InitData[i][1] );// ��ʼ���Ĵ���������
    }
    //CC1101SetAddress( 0x05, BROAD_0AND255 );//��ʼ��device��ַ��ģʽΪaddress_check and broadcast 255
    CC1101SetSYNC( 0x8799 );// set sync ox8799
    CC1101WriteReg(CC1101_MDMCFG1,0x22); //Modem Configuration

    CC1101WriteMultiReg(CC1101_PATABLE, PaTabel, 8 );// set the TX power table
	//  chip part-number and chip version number
    version_pn = CC1101ReadStatus( CC1101_PARTNUM );//for test, 
    version_pn = CC1101ReadStatus( CC1101_VERSION );//for test, refer to the datasheet
    if(version_pn == 0x14){
    	return 1;
    }
    return 0;
}

void init_cc1101(void){
	init_cc1101_gpio();
	init_spi();
	if(CC1101Init()){
		beep_beep(4);	
	}
}

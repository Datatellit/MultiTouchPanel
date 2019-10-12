#include "_global.h"
#include "delay.h"
#include "rf24l01.h"
#include "timer4.h"
#include "button.h"
#include "MyMessage.h"
#include "ProtocolParser.h"
#include "UsartDev.h"
#include "timer_2.h"
#include "lightPwmDrv.h"
#include "mbcrc.h"
#include "XlightComBus.h"
/*
Xlight Remoter Program
License: MIT

Auther: Baoshi Sun
Email: bs.sun@datatellit.com, bs.sun@uwaterloo.ca
Github: https://github.com/sunbaoshi1975
Please visit xlight.ca for product details

RF24L01 connector pinout:
GND    VCC
CE     CSN
SCK    MOSI
MISO   IRQ

Connections:
  PB4 -> CE
  PD4 -> CSN (10 buttons), PC6 -> CSN (11 buttons)
  PB5 -> SCK
  PB6 -> MOSI
  PB7 -> MISO
  PD5 -> IRQ

*/

#ifdef TEST
void testio()
{
  GPIO_Init(GPIOC , GPIO_Pin_1 , GPIO_Mode_Out_PP_Low_Slow);
  GPIO_Init(GPIOC , GPIO_Pin_3 , GPIO_Mode_Out_PP_Low_Slow);
  GPIO_Init(GPIOC , GPIO_Pin_5 , GPIO_Mode_Out_PP_Low_Slow);
}
#endif

#define MAX_RF_FAILED_TIME              20      // Reset RF module when reach max failed times of sending


// Timeout
#define RTE_TM_CONFIG_MODE              12000  // timeout in config mode, about 120s (12000 * 10ms)


bool isBtnAction = FALSE;

// Public variables
Config_t gConfig;

MyMessage_t sndMsg;
MyMessage_t rcvMsg;
uint8_t *psndMsg = (uint8_t *)&sndMsg;
uint8_t *prcvMsg = (uint8_t *)&rcvMsg;
bool gNeedSaveBackup = FALSE;
bool gIsStatusChanged = FALSE;
bool gIsConfigChanged = FALSE;
bool gResetRF = FALSE;
bool gResetNode = FALSE;

uint8_t gDelayedOperation = 0;
uint8_t _uniqueID[UNIQUE_ID_LEN];
uint8_t m_cntRFSendFailed = 0;
uint8_t gSendScenario = 0;
uint8_t gSendDelayTick = 0;

// add favorite function
int8_t gLastFavoriteIndex = -1;
uint8_t gLastFavoriteTick = 255;

uint8_t lastswitch = 1;
#define MSGSEND_TIMEOUT_TICK    500 //5000ms
uint16_t msgSendTimeout = MSGSEND_TIMEOUT_TICK;

// Moudle variables
bool bPowerOn = FALSE;
uint8_t mutex;
uint8_t oldCurrentDevID = 0;
uint16_t configMode_tick = 0;
void tmrProcess();
uint8_t tmrtick = 0;


static void clock_init(void)
{
  CLK_DeInit();
  CLK_HSICmd(ENABLE);
  CLK_SYSCLKDivConfig(SYS_CLOCK_DIVIDER);
  CLK_PeripheralClockConfig(CLK_Peripheral_TIM4, ENABLE);
  CLK_ClockSecuritySystemEnable();
}


void SetRGBlightNormal()
{
  if(tmrtick%2 == 0)
  {
    GPIO_WriteBit(GPIOB,GPIO_Pin_4,RESET);
    GPIO_WriteBit(GPIOB,GPIO_Pin_6,RESET);
  }
  else
  {
    GPIO_WriteBit(GPIOB,GPIO_Pin_4,SET);
    GPIO_WriteBit(GPIOB,GPIO_Pin_6,SET);
  }
}


/**
  * @brief  configure GPIOs before entering low power
	* @caller lowpower_config
  * @param None
  * @retval None
  */  
void GPIO_LowPower_Config(void)
{
  GPIO_Init(GPIOA, GPIO_Pin_2|GPIO_Pin_4, GPIO_Mode_In_FL_No_IT);
  GPIO_Init(GPIOA, GPIO_Pin_3|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7, GPIO_Mode_Out_PP_Low_Slow);
  GPIO_Init(GPIOC, GPIO_Pin_All, GPIO_Mode_Out_PP_Low_Slow);
  GPIO_Init(GPIOE, GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_5, GPIO_Mode_Out_PP_Low_Slow);
  GPIO_Init(GPIOF,GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7 ,GPIO_Mode_Out_PP_Low_Slow);
}

// Enter Low Power Mode, which can be woken up by external interupts
void lowpower_config(void) {
  // Set STM8 in low power
  PWR->CSR2 = 0x2;
 
  // Stop Timers
  TIM4_DeInit();
  
  // Set GPIO in low power
  GPIO_LowPower_Config();
  
  // RF24 Chip in low power
  RF24L01_DeInit();
  
  // Stop RTC Source clock
  CLK_RTCClockConfig(CLK_RTCCLKSource_Off, CLK_RTCCLKDiv_1);
  
  CLK_LSICmd(DISABLE);
  while ((CLK->ICKCR & 0x04) != 0x00)
  {
    feed_wwdg();
  }
  
  // Stop peripheral clocks
  CLK_PeripheralClockConfig(CLK_Peripheral_TIM5, DISABLE);
  CLK_PeripheralClockConfig(CLK_Peripheral_TIM4, DISABLE);
  CLK_PeripheralClockConfig(CLK_Peripheral_TIM3, DISABLE);
  CLK_PeripheralClockConfig(CLK_Peripheral_TIM2, DISABLE);
  CLK_PeripheralClockConfig(CLK_Peripheral_TIM1, DISABLE);
  CLK_PeripheralClockConfig(CLK_Peripheral_I2C1, DISABLE);
  CLK_PeripheralClockConfig(CLK_Peripheral_SPI1, DISABLE);
  CLK_PeripheralClockConfig(CLK_Peripheral_USART1, DISABLE);
  CLK_PeripheralClockConfig(CLK_Peripheral_DAC, DISABLE);
  CLK_PeripheralClockConfig(CLK_Peripheral_ADC1, DISABLE);
  CLK_PeripheralClockConfig(CLK_Peripheral_RTC, DISABLE);
  CLK_PeripheralClockConfig(CLK_Peripheral_LCD, DISABLE);
  CLK_PeripheralClockConfig(CLK_Peripheral_AES, DISABLE);
}

// Resume Normal Mode
void wakeup_config(void) {
  clock_init();
  timer_init();
  RF24L01_init();
  NRF2401_EnableIRQ();
}

// Save config to Flash
void SaveBackupConfig()
{
  if( gNeedSaveBackup ) {
    // Overwrite entire config FLASH
    if(Flash_WriteDataBlock(BACKUP_CONFIG_BLOCK_NUM, (uint8_t *)&gConfig, sizeof(gConfig)))
    {
      gNeedSaveBackup = FALSE;
    }
  }
}

// Save status to Flash
void SaveStatusData()
{
    // Skip the first byte (version)
    uint8_t pData[50] = {0};
    uint16_t nLen = (uint16_t)(&(gConfig.nodeID)) - (uint16_t)(&gConfig);
    memcpy(pData, (uint8_t *)&gConfig, nLen);
    if(Flash_WriteDataBlock(STATUS_DATA_NUM, pData, nLen))
    {
      gIsStatusChanged = FALSE;
    }
}

// Save config to Flash
void SaveConfig()
{
  if( gIsStatusChanged ) {
    // Overwrite only Static & status parameters (the first part of config FLASH)
    SaveStatusData();
    gIsConfigChanged = TRUE;
  } 
  if( gIsConfigChanged ) {
    // Overwrite entire config FLASH
    if(Flash_WriteDataBlock(0, (uint8_t *)&gConfig, sizeof(gConfig)))
    {
      gIsStatusChanged = FALSE;
      gIsConfigChanged = FALSE;
      gNeedSaveBackup = TRUE;
      return;
    }
  } 
}


bool IsConfigInvalid() {

  return ( gConfig.version > XLA_VERSION || gConfig.version < XLA_MIN_VER_REQUIREMENT
           || gConfig.nodeID == 0
           || gConfig.rfPowerLevel > RF24_PA_MAX || gConfig.rfChannel > 127 || gConfig.rfDataRate > RF24_250KBPS );
}

bool isNodeIdInvalid(uint8_t nodeid) {
  return( !IS_REMOTE_NODEID(nodeid)  );
}

void UpdateNodeAddress(uint8_t _tx) {
  memcpy(rx_addr, gConfig.NetworkID, ADDRESS_WIDTH);
  rx_addr[0] = gConfig.nodeID;
  memcpy(tx_addr, gConfig.NetworkID, ADDRESS_WIDTH);
  
  if( _tx == NODEID_RF_SCANNER ) {
    tx_addr[0] = NODEID_RF_SCANNER;
  } else {  
    tx_addr[0] = NODEID_GATEWAY;
  }
  RF24L01_setup(gConfig.rfChannel, gConfig.rfDataRate, gConfig.rfPowerLevel, BROADCAST_ADDRESS);     // With openning the boardcast pipe
}  

bool NeedUpdateRFAddress(uint8_t _dest) {
  bool rc = FALSE;
  if( sndMsg.header.destination == NODEID_RF_SCANNER && tx_addr[0] != NODEID_RF_SCANNER ) {
    UpdateNodeAddress(NODEID_RF_SCANNER);
    rc = TRUE;
  } else if( sndMsg.header.destination != NODEID_RF_SCANNER && tx_addr[0] != NODEID_GATEWAY ) {
    UpdateNodeAddress(NODEID_GATEWAY);
    rc = TRUE;
  }
  return rc;
}


bool WaitMutex(uint32_t _timeout) {
  while(_timeout--) {
    if( mutex > 0 ) return TRUE;
    feed_wwdg();
  }
  return FALSE;
}

// reset rf
void ResetRFModule()
{
  if(gResetRF)
  {
    RF24L01_init();
    NRF2401_EnableIRQ();
    UpdateNodeAddress(NODEID_GATEWAY);
    gResetRF=FALSE;
    RF24L01_set_mode_RX();
  }
}

bool SendMyMessageuart()
{
  if( bMsgReady ) {  
      uint16_t crcCheckRet = usMBCRC16((unsigned char*)&sndMsg,32);
      uint8_t data[34];
      memset(data,0x00,sizeof(data));
      memcpy(data,&sndMsg,32);
      data[32] = crcCheckRet>>8;
      data[33] = crcCheckRet&0xFF;
      disableInterrupts();
      UsartSendByteByLen(data,34);
      enableInterrupts();
      bMsgReady = 0;
  }
  return TRUE;
}

uint16_t GetDelayTick(uint8_t ds)
{
  uint16_t delaytick = 0;
  if(ds == BROADCAST_ADDRESS)
  {  
    delaytick = ((gConfig.nodeID - NODEID_MIN_REMOTE+1)%32)*40;
  }
  else
  {
    delaytick = 0;
  }
  return delaytick;
}

// Send message and switch back to receive mode
bool SendMyMessage() {
#ifdef RF24
  if( bMsgReady && msgSendTimeout == 0 )
  {
    bMsgReady = 0;
  }
  if( bMsgReady ) {  
    // Change tx destination if necessary
    NeedUpdateRFAddress(sndMsg.header.destination);
    
    uint8_t lv_tried = 0;
    uint16_t delay;
    while (lv_tried++ <= gConfig.rptTimes ) {
      feed_wwdg();
      mutex = 0;
      RF24L01_set_mode_TX();
      RF24L01_write_payload(psndMsg, PLOAD_WIDTH);
      WaitMutex(0x1FFFF);
      if (mutex == 1) {
        m_cntRFSendFailed = 0;   
        msgSendTimeout = MSGSEND_TIMEOUT_TICK;
        bMsgReady = 0;
        break; // sent sccessfully
      } else if( m_cntRFSendFailed++ > MAX_RF_FAILED_TIME ) {
        // Reset RF module
        m_cntRFSendFailed = 0;
        if(msgSendTimeout == 0) 
        {
          WWDG->CR = 0x80;
        }
        // RF24 Chip in low power
        /*RF24L01_DeInit();
        delay = 0x1FFF;
        while(delay--)feed_wwdg();
        RF24L01_init();
        NRF2401_EnableIRQ();
        UpdateNodeAddress(NODEID_GATEWAY);*/
        continue;
      }
      
      //The transmission failed, Notes: mutex == 2 doesn't mean failed
      //It happens when rx address defers from tx address
      //asm("nop"); //Place a breakpoint here to see memory
      // Repeat the message if necessary
      uint16_t delay = 0xFFF;
      while(delay--)feed_wwdg();
    }
    // Switch back to receive mode
    RF24L01_set_mode_RX();
  }

  return(mutex > 0);
#else
  return SendMyMessageuart();
#endif
}

// Change LED or Laser to indecate execution of specific operation
void OperationIndicator() {
 //todo
}

int main( void ) {
  // Init clock, timer and button
  clock_init();
  timer_init();
  button_init();
  button_bus_init();
#ifdef RF24
  // Go on only if NRF chip is presented
  RF24L01_init();
  while(!NRF24L01_Check());
#endif  
  // Load config from Flash
  FLASH_DeInit();
  Read_UniqueID(_uniqueID, UNIQUE_ID_LEN);
  LoadConfig();

#ifdef RF24
  // NRF_IRQ
  NRF2401_EnableIRQ();
#endif
  initTimPWMFunction();
  // Init Watchdog
  wwdg_init();
#ifndef RF24
  GPIO_ExternalPullUpConfig(GPIOC,GPIO_Pin_3,ENABLE);
  usart_config(9600);
#endif
  // Set PowerOn flag
  bPowerOn = TRUE;
  TIM4_10ms_handler = tmrProcess;
  //TIM2_10us_handler = shortTmrProcess;
#ifdef TEST
   testio();
#endif 
  printlog("start..."); 
  UpdateNodeAddress(NODEID_GATEWAY);
  Msg_Presentation();
  SendMyMessage();              

  while (1) {   
    // Feed the Watchdog
    feed_wwdg();
    ////////////rfscanner process///////////////////////////////
    ProcessOutputCfgMsg(); 
    // reset rf
    ResetRFModule();
    if(gResetNode)
    {
      gResetNode = FALSE;
    }
    ////////////rfscanner process/////////////////////////////// 
    
    if(isBtnAction)
    {/*
#ifdef SCENARIO_PANEL
#else
      Msg_RemoteKeyTable();
      memset(btn_action,0,sizeof(btn_action));
#endif     
      isBtnAction = FALSE;*/
    }
    // Send message if ready
    SendMyMessage();
    // Save Config if Changed
    SaveConfig(); 
    // Save config into backup area
#ifdef TEST
  PC5_High;
#endif  
    SaveBackupConfig();
#ifdef TEST
  PC5_Low;
#endif   
  }
}

void tmrProcess()
{
  if(bMsgReady && msgSendTimeout>0)
  {
    msgSendTimeout--;
  }
}

void RF24L01_IRQ_Handler() {
  //tmrIdleDuration = 0;
  if(RF24L01_is_data_available()) {
    //Packet was received
    RF24L01_clear_interrupts();
    RF24L01_read_payload(prcvMsg, PLOAD_WIDTH);
    ParseProtocol();
    return;
  }
 
  uint8_t sent_info;
  if (sent_info = RF24L01_was_data_sent()) {
    //Packet was sent or max retries reached
    RF24L01_clear_interrupts();
    mutex = sent_info; 
    return;
  }

   RF24L01_clear_interrupts();
}
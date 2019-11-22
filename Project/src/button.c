/*
 xlight remoter button functions

- Dimmer keys:
      PD2 -> keyUp
      PD3 -> keyDown
      PD0 -> keyLeft
      PD1 -> KeyRight
      PB0 -> KeyCenter

  - Functuon keys:
      PB1 -> Fn1
      PB2 -> Fn2
      PB3 -> Fn3
      //PD6 -> Fn4

LEDs
  Flashlight (White LED) -> PC1
  On/Off (Green LED) -> PC6
  Device Selection (Red LEDs) -> PC0 to PC3

*/

#include "_global.h"
#include "stm8l15x.h"
#include "stm8l15x_gpio.h"
#include "stm8l15x_exti.h"

#include "timer4.h"
#include "button.h"
#include "ProtocolParser.h"
#include "UsartDev.h"
#include "LightPublicDefine.h"

#define BTN_STEP_SHORT_BR       10
#define BTN_STEP_SHORT_CCT      350

// button map table
uint8_t btn_map[BTN_NUM] = {0,2,4,3,1};
// button action table
uint8_t btn_action[BTN_NUM] = {0};

// button scenario map table
uint8_t btn_scenariomap[BTN_NUM] = {1,2,3,4,5,6};


//---------------------------------------------------
// PIN Map
//---------------------------------------------------

// Button pin map
#define BUTTONS_PORT1           (GPIOB)
#define BUTTONS_PORT2           (GPIOD)
#define LEDS_PORT               (GPIOA)

// Port 1    
#define BUTTON_PIN_1           (GPIO_Pin_3)
#define BUTTON_PIN_2           (GPIO_Pin_2)
#define BUTTON_PIN_3           (GPIO_Pin_1)
#define BUTTON_PIN_4           (GPIO_Pin_0)
// Port 2
#define BUTTON_PIN_5           (GPIO_Pin_7)
#define BUTTON_PIN_6           (GPIO_Pin_3)

#define LEDS_PORT1               (GPIOD)
#define LED_PIN_6              (GPIO_Pin_1)

//---------------------------------------------------
#define LED_PIN_1              (GPIO_Pin_2)
#define LED_PIN_2              (GPIO_Pin_3)
#define LED_PIN_3              (GPIO_Pin_4)
#define LED_PIN_4              (GPIO_Pin_5)
#define LED_PIN_5              (GPIO_Pin_6)


// Get Button pin input
#define pinKey1                ((BitStatus)(BUTTONS_PORT1->IDR & (uint8_t)BUTTON_PIN_1))
#define pinKey2                ((BitStatus)(BUTTONS_PORT1->IDR & (uint8_t)BUTTON_PIN_2))
#define pinKey3                ((BitStatus)(BUTTONS_PORT1->IDR & (uint8_t)BUTTON_PIN_3))
#define pinKey4                ((BitStatus)(BUTTONS_PORT1->IDR & (uint8_t)BUTTON_PIN_4))
#define pinKey5                ((BitStatus)(BUTTONS_PORT2->IDR & (uint8_t)BUTTON_PIN_5))
#define pinKey6                ((BitStatus)(BUTTONS_PORT2->IDR & (uint8_t)BUTTON_PIN_6))

#define BUTTON_DEBONCE_DURATION                 1       // The unit is 10 ms, so the duration is 10 ms.
#define BUTTON_WAIT_2S                          100     // The unit is 10 ms, so the duration is 1 s.
#define BUTTON_WAIT_3S                          300     // The unit is 10 ms, so the duration is 3 s.
#define BUTTON_DOUBLE_BTN_DURATION              50      // The unit is 10 ms, so the duration is 500 ms.
#define BUTTON_DOUBLE_BTN_TRACK_DURATION        300     // The unit is 10 ms, so the duration is 3 s.

static button_timer_status_t  m_btn_timer_status[BTN_NUM] = {BUTTON_STATUS_INIT};
static bool detect_double_btn_press[BTN_NUM] = {FALSE};
static bool btn_is_pushed[BTN_NUM] = {FALSE};
static uint16_t btn_bit_postion[BTN_NUM];

static uint8_t m_timer_id_btn_detet[BTN_NUM];
static uint8_t m_timer_id_double_btn_detet[BTN_NUM];

static bool double_button_track = FALSE;
static uint16_t button_status = 0xFFFF;
static uint16_t button_first_detect_status = 0xFFFF;

static uint8_t m_timer_id_debonce_detet;

void app_button_event_handler(uint8_t _btn, button_event_t button_event);
void button_push(uint8_t _btn);
void button_release(uint8_t _btn);

uint8_t led_btn[keylstDummy] = {LED_PIN_1,LED_PIN_2,LED_PIN_3,LED_PIN_4,LED_PIN_5,LED_PIN_6};


void button_bus_init()
{

}

static void btn_duration_timeout_handler(uint8_t _btn)
{
  // Assert button
  if( !IS_VALID_BUTTON(_btn) ) return;
  
  button_event_t button_event = BUTTON_INVALID;
  switch (m_btn_timer_status[_btn]) {
  case BUTTON_STATUS_INIT:
    break;
    
  case BUTTON_STATUS_LESS_2S:
    button_event = BUTTON_LONG_HOLD;
    timer_start(m_timer_id_btn_detet[_btn], BUTTON_WAIT_3S);
    m_btn_timer_status[_btn] = BUTTON_STATUS_MORE_2S;
    break;
    
  case BUTTON_STATUS_MORE_2S:
    button_event = BUTTON_VERY_LONG_HOLD;
    m_btn_timer_status[_btn] = BUTTON_STATUS_MORE_5S;
    break;
    
  case BUTTON_STATUS_MORE_5S:
    break;
    
  case BUTTON_STATUS_DOUBLE_TRACK:
    button_event = DOUBLE_BTN_TRACK;
    m_btn_timer_status[_btn] = BUTTON_STATUS_INIT;
    break;
    
  default:
    break;
  }
  
  if( button_event != BUTTON_INVALID ) {
    app_button_event_handler(_btn, button_event);
  }
}

void double_btn_timeout_handler(uint8_t _btn)
{
  // Assert button
  if( !IS_VALID_BUTTON(_btn) ) return;
  
  button_event_t button_event = BUTTON_SHORT_PRESS;
  detect_double_btn_press[_btn] = FALSE;
  m_btn_timer_status[_btn] = BUTTON_STATUS_INIT;
  timer_stop(m_timer_id_double_btn_detet[_btn]);
  app_button_event_handler(_btn, button_event);
}

void btn_debonce_timeout_handler(uint8_t _tag)
{
  uint16_t valid_button;
  uint16_t current_button;
  uint16_t changed_button;
  
  current_button = GPIO_ReadInputData(BUTTONS_PORT1);
  current_button <<= 8;
  current_button |= GPIO_ReadInputData(BUTTONS_PORT2);

  valid_button = ~(current_button ^ button_first_detect_status);    
  changed_button = ((current_button^button_status) & valid_button);
  button_status = current_button;
  
  // Scan all buttons
  uint8_t _btn;
  for( _btn = 0; _btn < BTN_NUM; _btn++ ) {    
    if ((changed_button & btn_bit_postion[_btn]) != 0)
    {
      timer_stop(m_timer_id_btn_detet[_btn]);
      if ((current_button & btn_bit_postion[_btn]) == 0)
      {
        button_push(_btn);    
      }
      else
      {
        button_release(_btn);
      }
    }
  }
}
void led_btn_init()
{
    GPIO_Init(LEDS_PORT, ( LED_PIN_1|LED_PIN_2 |LED_PIN_3 |LED_PIN_4 |LED_PIN_5), GPIO_Mode_Out_PP_High_Fast);
    GPIO_Init(LEDS_PORT1, LED_PIN_6, GPIO_Mode_Out_PP_High_Fast);
}

void button_init()
{
  uint8_t _btn;
  
  // Set button bit postion
  btn_bit_postion[keylst1] = BUTTON_PIN_1;
  btn_bit_postion[keylst1] <<= 8;
  btn_bit_postion[keylst2] = BUTTON_PIN_2;
  btn_bit_postion[keylst2] <<= 8;
  btn_bit_postion[keylst3] = BUTTON_PIN_3;
  btn_bit_postion[keylst3] <<= 8;
  btn_bit_postion[keylst4] = BUTTON_PIN_4;
  btn_bit_postion[keylst4] <<= 8;
  btn_bit_postion[keylst5] = BUTTON_PIN_5;
  btn_bit_postion[keylst6] = BUTTON_PIN_6;
  
  // Setup Interrupts
  disableInterrupts();
  //TODO
/*#ifdef ENABLE_FLASHLIGHT_LASER  
  GPIO_Init(LEDS_PORT, (LED_PIN_FLASHLIGHT | LED_PIN_LASERPEN), GPIO_Mode_Out_PP_Low_Fast);
#endif*/
  GPIO_Init(BUTTONS_PORT1, ( BUTTON_PIN_1 | BUTTON_PIN_2| BUTTON_PIN_3 | BUTTON_PIN_4 ), GPIO_Mode_In_PU_IT);
  GPIO_Init(BUTTONS_PORT2, (BUTTON_PIN_5), GPIO_Mode_In_PU_IT);
  GPIO_Init(BUTTONS_PORT2, (BUTTON_PIN_6), GPIO_Mode_In_PU_IT);
  EXTI_DeInit();
  EXTI_SelectPort(EXTI_Port_D);
  EXTI_SetPinSensitivity(EXTI_Pin_7, EXTI_Trigger_Rising_Falling);
  EXTI_SetPinSensitivity(EXTI_Pin_3, EXTI_Trigger_Rising_Falling);
  
  EXTI_SelectPort(EXTI_Port_B);
  EXTI_SetPinSensitivity(EXTI_Pin_0, EXTI_Trigger_Rising_Falling);
  EXTI_SetPinSensitivity(EXTI_Pin_1, EXTI_Trigger_Rising_Falling);
  EXTI_SetPinSensitivity(EXTI_Pin_2, EXTI_Trigger_Rising_Falling);
  EXTI_SetPinSensitivity(EXTI_Pin_3, EXTI_Trigger_Rising_Falling);
  enableInterrupts();

  // Create all timers
  for( _btn = 0; _btn < BTN_NUM; _btn++ ) {
    timer_create(&m_timer_id_btn_detet[_btn], _btn, btn_duration_timeout_handler);
    timer_create(&m_timer_id_double_btn_detet[_btn], _btn, double_btn_timeout_handler);
  }
  timer_create(&m_timer_id_debonce_detet, 0, btn_debonce_timeout_handler);
  led_btn_init();
}

void btn_short_button_press(uint8_t _btn)
{
  // Assert button
  if( !IS_VALID_BUTTON(_btn) ) return;
  printnum(_btn);
  printlog(" short press\r\n");
  isBtnAction = TRUE;
  btn_action[_btn] = 1;
  switch( _btn ) {
  case keylst1:
#ifdef XLA_PANEL_Type_CCT
      // 3000K
      Msg_DevCCT(OPERATOR_SET, 3000);
#else    
      // darker
      Msg_DevBrightness(OPERATOR_SET, 25);
#endif
    break;
  
  case keylst2:
#ifdef XLA_PANEL_Type_CCT
      // warmer
      Msg_DevCCT(OPERATOR_SUB, BTN_STEP_SHORT_CCT);
#else    
      // decrease BR
      Msg_DevBrightness(OPERATOR_SUB, BTN_STEP_SHORT_BR);
#endif
      break;
  
  case keylst3:
#ifdef XLA_PANEL_Type_CCT
      // 4000K
      Msg_DevCCT(OPERATOR_SET, 4000);
#else    
      // modest
      Msg_DevBrightness(OPERATOR_SET, 50);
#endif
    break;
  
  case keylst4:
#ifdef XLA_PANEL_Type_CCT
      // Coldest
      Msg_DevCCT(OPERATOR_SET, 6500);
#else    
      // brightest
      Msg_DevBrightness(OPERATOR_SET, 100);
#endif
      break;
  
  case keylst5:
#ifdef XLA_PANEL_Type_CCT
      // 5500K
      Msg_DevCCT(OPERATOR_SET, 5500);
#else
      // darker
      Msg_DevBrightness(OPERATOR_SET, 75);
#endif
      break;
  
  case keylst6:
#ifdef XLA_PANEL_Type_CCT
      // Cooler
      Msg_DevCCT(OPERATOR_ADD, BTN_STEP_SHORT_CCT);    
#else
      // increase BR
      Msg_DevBrightness(OPERATOR_ADD, BTN_STEP_SHORT_BR);
#endif
      break;
  
  default:
    break;    
  }
}

void btn_double_button_press(uint8_t _btn)
{
  // Assert button
  if( !IS_VALID_BUTTON(_btn) ) return;
  printnum(_btn);
  printlog(" double press\r\n");
  isBtnAction = TRUE;
  btn_action[_btn] = 3;
  /*switch( _btn ) {
     case keylst1:
      break;
  } */
}

void btn_long_hold_button_press(uint8_t _btn)
{
  // Assert button
  if( !IS_VALID_BUTTON(_btn) ) return;
  printnum(_btn);
  printlog(" long_hold press\r\n");  
  switch( _btn ) {
  case keylst2:
      // minum br
      Msg_DevBrightness(OPERATOR_SET, 10);
    break;
  case keylst4:
      // max br
      Msg_DevBrightness(OPERATOR_SET, 100);
    break;
  default:
    break;
  }
}

void btn_long_button_press(uint8_t _btn)
{
  // Assert button
  if( !IS_VALID_BUTTON(_btn) ) return;
  printnum(_btn);
  printlog(" long press\r\n");    
  isBtnAction = TRUE;
  btn_action[_btn] = 2;
  /*switch( _btn ) {
     case keylst1:
      break;
  }*/
}

void btn_very_long_hold_button_press(uint8_t _btn)
{
  // Assert button
  if( !IS_VALID_BUTTON(_btn) ) return;
  printnum(_btn);
  printlog(" very_long_hold press\r\n"); 
  switch( _btn ) {
     case keylst1:
      break;
  }
}

void btn_very_long_button_press(uint8_t _btn)
{
  // Assert button
  if( !IS_VALID_BUTTON(_btn) ) return;
  printnum(_btn);
  printlog(" very_long press\r\n");      
  switch( _btn ) {
     case keylst1:
      break;
  }
}

void btn_double_long_hold_press(uint8_t _btn1, uint8_t _btn2)
{
  // Assert button
  if( !IS_VALID_BUTTON(_btn1) || !IS_VALID_BUTTON(_btn2) ) return;
}

void app_button_event_handler(uint8_t _btn, button_event_t button_event)
{
  uint8_t sec_btn = BTN_NUM;
  
  switch (button_event)
  {
  case BUTTON_INVALID:
    break;
    
  case BUTTON_SHORT_PRESS:
    btn_short_button_press(_btn);
    break;
    
  case BUTTON_DOUBLE_PRESS:
    btn_double_button_press(_btn);
    break;
    
  case BUTTON_LONG_HOLD:
    btn_long_hold_button_press(_btn);
    break;
    
  case BUTTON_LONG_PRESS:
    btn_long_button_press(_btn);
    break;
    
  case BUTTON_VERY_LONG_HOLD:
    btn_very_long_hold_button_press(_btn);
    break;
    
  case BUTTON_VERY_LONG_PRESS:
    btn_very_long_button_press(_btn);
    break;
    
  case DOUBLE_BTN_TRACK:
    /*if( btn_is_pushed[keylstFn1] ) sec_btn = keylstFn1;
    else if( btn_is_pushed[keylstFn2] ) sec_btn = keylstFn2;
    else if( btn_is_pushed[keylstFn3] ) sec_btn = keylstFn3;
    else if( btn_is_pushed[keylstFn4] ) sec_btn = keylstFn4;
    if( sec_btn < keylstDummy )
      btn_double_long_hold_press(_btn, sec_btn);*/
    break;
    
  default:
    break;
  }
}

// Only use button1_timer to track double button long hold.
void check_track_double_button(void)
{
  if( btn_is_pushed[keylst3] == TRUE && btn_is_pushed[keylst2] == TRUE )
  {
    printlog("3 & 2");   
    timer_stop(m_timer_id_btn_detet[keylst2]);  // Disable btn2_timer when tracking double hold.
    m_btn_timer_status[keylst3] = BUTTON_STATUS_DOUBLE_TRACK;
    m_btn_timer_status[keylst2] = BUTTON_STATUS_INIT;
    double_button_track = TRUE;
    timer_start(m_timer_id_btn_detet[keylst2], BUTTON_DOUBLE_BTN_TRACK_DURATION);  //3 s
  } else if( btn_is_pushed[keylst3] == TRUE && btn_is_pushed[keylst4] == TRUE ) {
    printlog("3 & 4");
    timer_stop(m_timer_id_btn_detet[keylst4]);  // Disable btn2_timer when tracking double hold.
    m_btn_timer_status[keylst3] = BUTTON_STATUS_DOUBLE_TRACK;
    m_btn_timer_status[keylst4] = BUTTON_STATUS_INIT;
    double_button_track = TRUE;
    timer_start(m_timer_id_btn_detet[keylst4], BUTTON_DOUBLE_BTN_TRACK_DURATION);  //3 s
  }  else {
    if (double_button_track == TRUE)
    {
      m_btn_timer_status[keylst3] = BUTTON_STATUS_INIT;
      m_btn_timer_status[keylst2] = BUTTON_STATUS_INIT;
      m_btn_timer_status[keylst4] = BUTTON_STATUS_INIT;
      double_button_track = FALSE;
      timer_stop(m_timer_id_btn_detet[keylst3]);
      timer_stop(m_timer_id_btn_detet[keylst2]);
      timer_stop(m_timer_id_btn_detet[keylst4]);
    }
  }
}
void led_on(uint8_t num)
{
  if(num == 5) GPIO_WriteBit(LEDS_PORT1,led_btn[num],RESET);
  else GPIO_WriteBit(LEDS_PORT,led_btn[num],RESET);
}
void led_off(uint8_t num)
{
  if(num == 5) GPIO_WriteBit(LEDS_PORT1,led_btn[num],SET);
  else GPIO_WriteBit(LEDS_PORT,led_btn[num],SET);
}

void button_push(uint8_t _btn)
{
  // Assert button
  if( !IS_VALID_BUTTON(_btn) ) return;
  led_on(_btn);
  btn_is_pushed[_btn] = TRUE;
  check_track_double_button();
  
  if (double_button_track == FALSE)
  {
    m_btn_timer_status[_btn] = BUTTON_STATUS_LESS_2S;
    timer_start(m_timer_id_btn_detet[_btn], BUTTON_WAIT_2S);
  }
}

void button_release(uint8_t _btn)
{
  // Assert button
  if( !IS_VALID_BUTTON(_btn) ) return;
  led_off(_btn);
  btn_is_pushed[_btn] = FALSE;
  button_event_t button_event = BUTTON_INVALID;
  
  check_track_double_button();
  
  switch (m_btn_timer_status[_btn])
  {
  case BUTTON_STATUS_INIT:
    break;
    
  case BUTTON_STATUS_LESS_2S:
    if (detect_double_btn_press[_btn] == FALSE)
    {
      detect_double_btn_press[_btn] = TRUE;
      timer_start(m_timer_id_double_btn_detet[_btn], BUTTON_DOUBLE_BTN_DURATION);  // 500ms
    }
    else
    {
      button_event = BUTTON_DOUBLE_PRESS;
      detect_double_btn_press[_btn] = FALSE;
      timer_stop(m_timer_id_double_btn_detet[_btn]);
    }
    break;
    
  case BUTTON_STATUS_MORE_2S:
    button_event = BUTTON_LONG_PRESS;
    break;
    
  case BUTTON_STATUS_MORE_5S:
    button_event = BUTTON_VERY_LONG_PRESS;
    break;
    
  default:
    break;
  }
  
  m_btn_timer_status[_btn] = BUTTON_STATUS_INIT;
  if (button_event != BUTTON_INVALID) {
    app_button_event_handler(_btn, button_event);
  }
}

void button_event_handler(uint8_t _pin)
{
  tmrIdleDuration = 0;
  button_first_detect_status = GPIO_ReadInputData(BUTTONS_PORT1);
  button_first_detect_status <<= 8;
  button_first_detect_status |= GPIO_ReadInputData(BUTTONS_PORT2);
  timer_start(m_timer_id_debonce_detet, BUTTON_DEBONCE_DURATION);
}

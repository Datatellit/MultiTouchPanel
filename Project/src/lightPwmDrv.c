#include "lightPwmDrv.h"
#include "stm8l15x.h"
#define TIM1_PERIOD             65535
#define TIM1_REPTETION_COUNTER      0
static void TIMLightPwm_ClkGpioConfig(void)
{
  /* Enable TIM1 clock */
  CLK_PeripheralClockConfig(CLK_Peripheral_TIM1, ENABLE);
  CLK_PeripheralClockConfig(CLK_Peripheral_TIM3, ENABLE);
  GPIO_Init(GPIOD, GPIO_Pin_4 | GPIO_Pin_2  , GPIO_Mode_Out_PP_High_Fast);
  GPIO_Init(GPIOD, GPIO_Pin_0 , GPIO_Mode_Out_PP_High_Fast);
}

static void TIMPWMFunction_Config(void)
{
  TIM1_DeInit();
  TIM3_DeInit();
  /* Time base configuration */
  TIM1_TimeBaseInit(7, TIM1_CounterMode_Up, 1999, TIM1_REPTETION_COUNTER);
  //Õ¼¿Õ±È tim_plus/(peirod+1)
  TIM1_OC1Init(TIM1_OCMode_PWM1, TIM1_OutputState_Enable, TIM1_OutputNState_Disable,
               20, TIM1_OCPolarity_Low, TIM1_OCNPolarity_Low, TIM1_OCIdleState_Set,
               TIM1_OCNIdleState_Set);
  TIM1_OC1PreloadConfig(ENABLE);

  TIM1_OC2Init(TIM1_OCMode_PWM1, TIM1_OutputState_Enable, TIM1_OutputNState_Disable,
               200, TIM1_OCPolarity_Low, TIM1_OCNPolarity_Low, TIM1_OCIdleState_Set,
               TIM1_OCNIdleState_Set);
  TIM1_OC2PreloadConfig(ENABLE);

  TIM1_ARRPreloadConfig(ENABLE);
  
  TIM3_TimeBaseInit(TIM3_Prescaler_8, TIM3_CounterMode_Up,1999);
  TIM3_OC2Init(TIM3_OCMode_PWM1, TIM3_OutputState_Enable, 200, TIM3_OCPolarity_Low,TIM3_OCIdleState_Set);
  TIM3_OC2PreloadConfig(ENABLE);
  TIM3_ARRPreloadConfig(ENABLE);

  /* TIM1 Interrupt enable */ // must delete this code
  //TIM1_ITConfig(TIM1_IT_CC1, ENABLE);
  //TIM1_ITConfig(TIM1_IT_CC2, ENABLE);

  /* Enable TIM1 outputs */
  TIM1_CtrlPWMOutputs(ENABLE);
  TIM3_CtrlPWMOutputs(ENABLE);
  /* TIM1 enable counter */
  TIM1_Cmd(ENABLE);
  TIM3_Cmd(ENABLE);
}

void initTimPWMFunction (void)
{
  TIMLightPwm_ClkGpioConfig ();
  TIMPWMFunction_Config ();
}

#include "stm8l15x.h"
#include "stm8l15x_tim2.h"
#include "timer_2.h"

TM2_CallBack_t TIM2_10us_handler = 0;
void TIM2_Init(void)
{
  TIM2_DeInit();
  CLK_PeripheralClockConfig(CLK_Peripheral_TIM2, ENABLE);  //enable timer 2 clk
  TIM2_TimeBaseInit(TIM2_Prescaler_1,TIM2_CounterMode_Up, 16000);     // 1000us
  TIM2_SetCounter(0);
  TIM2_ITConfig(TIM2_IT_Update, ENABLE); //Enable TIM2 IT UPDATE
  TIM2_ARRPreloadConfig(ENABLE);
  TIM2_Cmd(ENABLE);
}


#ifndef __TIMER_TWO_H
#define __TIMER_TWO_H

typedef void (*TM2_CallBack_t)();
// 10us interupt
void TIM2_Init(void);
extern TM2_CallBack_t TIM2_10us_handler;
#endif // __TIMER_TWO_H
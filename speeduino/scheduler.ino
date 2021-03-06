/*
Speeduino - Simple engine management for the Arduino Mega 2560 platform
Copyright (C) Josh Stewart
A full copy of the license may be found in the projects root directory
*/

#include "scheduler.h"
#include "globals.h"

void initialiseSchedulers()
  {
    nullSchedule.Status = OFF;

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__) //AVR chips use the ISR for this
   // Much help in this from http://arduinomega.blogspot.com.au/2011/05/timer2-and-overflow-interrupt-lets-get.html
    //Fuel Schedules, which uses timer 3
    TCCR3B = 0x00;          //Disable Timer3 while we set it up
    TCNT3  = 0;             //Reset Timer Count
    TIFR3  = 0x00;          //Timer3 INT Flag Reg: Clear Timer Overflow Flag
    TCCR3A = 0x00;          //Timer3 Control Reg A: Wave Gen Mode normal
    TCCR3B = (1 << CS12);   //Timer3 Control Reg B: Timer Prescaler set to 256. Refer to http://www.instructables.com/files/orig/F3T/TIKL/H3WSA4V7/F3TTIKLH3WSA4V7.jpg
    //TCCR3B = 0x03;   //Timer3 Control Reg B: Timer Prescaler set to 64. Refer to http://www.instructables.com/files/orig/F3T/TIKL/H3WSA4V7/F3TTIKLH3WSA4V7.jpg

    //Ignition Schedules, which uses timer 5
    TCCR5B = 0x00;          //Disable Timer5 while we set it up
    TCNT5  = 0;             //Reset Timer Count
    TIFR5  = 0x00;          //Timer5 INT Flag Reg: Clear Timer Overflow Flag
    TCCR5A = 0x00;          //Timer5 Control Reg A: Wave Gen Mode normal
    //TCCR5B = (1 << CS12);   //Timer5 Control Reg B: Timer Prescaler set to 256. Refer to http://www.instructables.com/files/orig/F3T/TIKL/H3WSA4V7/F3TTIKLH3WSA4V7.jpg
    TCCR5B = 0x03;         //aka Divisor = 64 = 490.1Hz

    //The remaining Schedules (Schedules 4 for fuel and ignition) use Timer4
    TCCR4B = 0x00;          //Disable Timer4 while we set it up
    TCNT4  = 0;             //Reset Timer Count
    TIFR4  = 0x00;          //Timer4 INT Flag Reg: Clear Timer Overflow Flag
    TCCR4A = 0x00;          //Timer4 Control Reg A: Wave Gen Mode normal
    TCCR4B = (1 << CS12);   //Timer4 Control Reg B: aka Divisor = 256 = 122.5HzTimer Prescaler set to 256. Refer to http://www.instructables.com/files/orig/F3T/TIKL/H3WSA4V7/F3TTIKLH3WSA4V7.jpg

#elif defined (CORE_TEENSY)

  //FlexTimer 0 is used for 4 ignition and 4 injection schedules. There are 8 channels on this module, so no other timers are needed
  FTM0_MODE |= FTM_MODE_WPDIS; // Write Protection Disable
  FTM0_MODE |= FTM_MODE_FTMEN; //Flex Timer module enable
  FTM0_MODE |= FTM_MODE_INIT;

  FTM0_SC = 0x00; // Set this to zero before changing the modulus
  FTM0_CNTIN = 0x0000; //Shouldn't be needed, but just in case
  FTM0_CNT = 0x0000; // Reset the count to zero
  FTM0_MOD = 0xFFFF; // max modulus = 65535

  //FlexTimer 1 is used for schedules on channel 5+. Currently only channel 5 is used, but will likely be expanded later
  FTM1_MODE |= FTM_MODE_WPDIS; // Write Protection Disable
  FTM1_MODE |= FTM_MODE_FTMEN; //Flex Timer module enable
  FTM1_MODE |= FTM_MODE_INIT;

  FTM1_SC = 0x00; // Set this to zero before changing the modulus
  FTM1_CNTIN = 0x0000; //Shouldn't be needed, but just in case
  FTM1_CNT = 0x0000; // Reset the count to zero
  FTM1_MOD = 0xFFFF; // max modulus = 65535

  /*
   * Enable the clock for FTM0/1
   * 00 No clock selected. Disables the FTM counter.
   * 01 System clock
   * 10 Fixed frequency clock
   * 11 External clock
   */
  FTM0_SC |= FTM_SC_CLKS(0b1);
  FTM1_SC |= FTM_SC_CLKS(0b1);

  /*
   * Set Prescaler
   * This is the slowest that the timer can be clocked (Without used the slow timer, which is too slow). It results in ticks of 2.13333uS on the teensy 3.5:
   * 60000000 Hz = F_BUS
   * 128 * 1000000uS / F_BUS = 2.133uS
   *
   * 000 = Divide by 1
   * 001 Divide by 2
   * 010 Divide by 4
   * 011 Divide by 8
   * 100 Divide by 16
   * 101 Divide by 32
   * 110 Divide by 64
   * 111 Divide by 128
   */
  FTM0_SC |= FTM_SC_PS(0b111);
  FTM1_SC |= FTM_SC_PS(0b111);

  //Setup the channels (See Pg 1014 of K64 DS).
  //The are probably not needed as power on state should be 0
  //FTM0_C0SC &= ~FTM_CSC_ELSB;
  //FTM0_C0SC &= ~FTM_CSC_ELSA;
  //FTM0_C0SC &= ~FTM_CSC_DMA;
  FTM0_C0SC &= ~FTM_CSC_MSB; //According to Pg 965 of the K64 datasheet, this should not be needed as MSB is reset to 0 upon reset, but the channel interrupt fails to fire without it
  FTM0_C0SC |= FTM_CSC_MSA; //Enable Compare mode
  FTM0_C0SC |= FTM_CSC_CHIE; //Enable channel compare interrupt

  FTM0_C1SC &= ~FTM_CSC_MSB; //According to Pg 965 of the datasheet, this should not be needed as MSB is reset to 0 upon reset, but the channel interrupt fails to fire without it
  FTM0_C1SC |= FTM_CSC_MSA; //Enable Compare mode
  FTM0_C1SC |= FTM_CSC_CHIE; //Enable channel compare interrupt

  FTM0_C2SC &= ~FTM_CSC_MSB; //According to Pg 965 of the datasheet, this should not be needed as MSB is reset to 0 upon reset, but the channel interrupt fails to fire without it
  FTM0_C2SC |= FTM_CSC_MSA; //Enable Compare mode
  FTM0_C2SC |= FTM_CSC_CHIE; //Enable channel compare interrupt

  FTM0_C3SC &= ~FTM_CSC_MSB; //According to Pg 965 of the datasheet, this should not be needed as MSB is reset to 0 upon reset, but the channel interrupt fails to fire without it
  FTM0_C3SC |= FTM_CSC_MSA; //Enable Compare mode
  FTM0_C3SC |= FTM_CSC_CHIE; //Enable channel compare interrupt

  FTM0_C4SC &= ~FTM_CSC_MSB; //According to Pg 965 of the datasheet, this should not be needed as MSB is reset to 0 upon reset, but the channel interrupt fails to fire without it
  FTM0_C4SC |= FTM_CSC_MSA; //Enable Compare mode
  FTM0_C4SC |= FTM_CSC_CHIE; //Enable channel compare interrupt

  FTM0_C5SC &= ~FTM_CSC_MSB; //According to Pg 965 of the datasheet, this should not be needed as MSB is reset to 0 upon reset, but the channel interrupt fails to fire without it
  FTM0_C5SC |= FTM_CSC_MSA; //Enable Compare mode
  FTM0_C5SC |= FTM_CSC_CHIE; //Enable channel compare interrupt

  FTM0_C6SC &= ~FTM_CSC_MSB; //According to Pg 965 of the datasheet, this should not be needed as MSB is reset to 0 upon reset, but the channel interrupt fails to fire without it
  FTM0_C6SC |= FTM_CSC_MSA; //Enable Compare mode
  FTM0_C6SC |= FTM_CSC_CHIE; //Enable channel compare interrupt

  FTM0_C7SC &= ~FTM_CSC_MSB; //According to Pg 965 of the datasheet, this should not be needed as MSB is reset to 0 upon reset, but the channel interrupt fails to fire without it
  FTM0_C7SC |= FTM_CSC_MSA; //Enable Compare mode
  FTM0_C7SC |= FTM_CSC_CHIE; //Enable channel compare interrupt

  //Do the same, but on flex timer 3 (Used for channels 5-8)
  FTM3_C0SC &= ~FTM_CSC_MSB; //According to Pg 965 of the K64 datasheet, this should not be needed as MSB is reset to 0 upon reset, but the channel interrupt fails to fire without it
  FTM3_C0SC |= FTM_CSC_MSA; //Enable Compare mode
  FTM3_C0SC |= FTM_CSC_CHIE; //Enable channel compare interrupt

  FTM3_C1SC &= ~FTM_CSC_MSB; //According to Pg 965 of the K64 datasheet, this should not be needed as MSB is reset to 0 upon reset, but the channel interrupt fails to fire without it
  FTM3_C1SC |= FTM_CSC_MSA; //Enable Compare mode
  FTM3_C1SC |= FTM_CSC_CHIE; //Enable channel compare interrupt

  FTM3_C2SC &= ~FTM_CSC_MSB; //According to Pg 965 of the K64 datasheet, this should not be needed as MSB is reset to 0 upon reset, but the channel interrupt fails to fire without it
  FTM3_C2SC |= FTM_CSC_MSA; //Enable Compare mode
  FTM3_C2SC |= FTM_CSC_CHIE; //Enable channel compare interrupt

  FTM3_C3SC &= ~FTM_CSC_MSB; //According to Pg 965 of the K64 datasheet, this should not be needed as MSB is reset to 0 upon reset, but the channel interrupt fails to fire without it
  FTM3_C3SC |= FTM_CSC_MSA; //Enable Compare mode
  FTM3_C3SC |= FTM_CSC_CHIE; //Enable channel compare interrupt

  FTM3_C4SC &= ~FTM_CSC_MSB; //According to Pg 965 of the K64 datasheet, this should not be needed as MSB is reset to 0 upon reset, but the channel interrupt fails to fire without it
  FTM3_C4SC |= FTM_CSC_MSA; //Enable Compare mode
  FTM3_C4SC |= FTM_CSC_CHIE; //Enable channel compare interrupt

  FTM3_C5SC &= ~FTM_CSC_MSB; //According to Pg 965 of the K64 datasheet, this should not be needed as MSB is reset to 0 upon reset, but the channel interrupt fails to fire without it
  FTM3_C5SC |= FTM_CSC_MSA; //Enable Compare mode
  FTM3_C5SC |= FTM_CSC_CHIE; //Enable channel compare interrupt

  FTM3_C6SC &= ~FTM_CSC_MSB; //According to Pg 965 of the K64 datasheet, this should not be needed as MSB is reset to 0 upon reset, but the channel interrupt fails to fire without it
  FTM3_C6SC |= FTM_CSC_MSA; //Enable Compare mode
  FTM3_C6SC |= FTM_CSC_CHIE; //Enable channel compare interrupt

  FTM3_C7SC &= ~FTM_CSC_MSB; //According to Pg 965 of the K64 datasheet, this should not be needed as MSB is reset to 0 upon reset, but the channel interrupt fails to fire without it
  FTM3_C7SC |= FTM_CSC_MSA; //Enable Compare mode
  FTM3_C7SC |= FTM_CSC_CHIE; //Enable channel compare interrupt

  // enable IRQ Interrupt
  NVIC_ENABLE_IRQ(IRQ_FTM0);
  NVIC_ENABLE_IRQ(IRQ_FTM1);

#elif defined(CORE_STM32)
  #if defined(ARDUINO_ARCH_STM32) // STM32GENERIC core
    //see https://github.com/rogerclarkmelbourne/Arduino_STM32/blob/754bc2969921f1ef262bd69e7faca80b19db7524/STM32F1/system/libmaple/include/libmaple/timer.h#L444
    Timer1.setPrescaleFactor((HAL_RCC_GetHCLKFreq() * 2U)-1);  //2us resolution
    Timer2.setPrescaleFactor((HAL_RCC_GetHCLKFreq() * 2U)-1);  //2us resolution
    Timer3.setPrescaleFactor((HAL_RCC_GetHCLKFreq() * 2U)-1);  //2us resolution
    Timer2.setMode(1, TIMER_OUTPUT_COMPARE);
    Timer2.setMode(2, TIMER_OUTPUT_COMPARE);
    Timer2.setMode(3, TIMER_OUTPUT_COMPARE);
    Timer2.setMode(4, TIMER_OUTPUT_COMPARE);

    Timer3.setMode(1, TIMER_OUTPUT_COMPARE);
    Timer3.setMode(2, TIMER_OUTPUT_COMPARE);
    Timer3.setMode(3, TIMER_OUTPUT_COMPARE);
    Timer3.setMode(4, TIMER_OUTPUT_COMPARE);
    Timer1.setMode(1, TIMER_OUTPUT_COMPARE);

  #else //libmaple core aka STM32DUINO
    //see https://github.com/rogerclarkmelbourne/Arduino_STM32/blob/754bc2969921f1ef262bd69e7faca80b19db7524/STM32F1/system/libmaple/include/libmaple/timer.h#L444
    //(CYCLES_PER_MICROSECOND == 72, APB2 at 72MHz, APB1 at 36MHz).
    //Timer2 to 4 is on APB1, Timer1 on APB2.   http://www.st.com/resource/en/datasheet/stm32f103cb.pdf sheet 12
    Timer1.setPrescaleFactor((72 * 2U)-1); //2us resolution
    Timer2.setPrescaleFactor((36 * 2U)-1); //2us resolution
    Timer3.setPrescaleFactor((36 * 2U)-1); //2us resolution
    Timer2.setMode(TIMER_CH1, TIMER_OUTPUT_COMPARE);
    Timer2.setMode(TIMER_CH2, TIMER_OUTPUT_COMPARE);
    Timer2.setMode(TIMER_CH3, TIMER_OUTPUT_COMPARE);
    Timer2.setMode(TIMER_CH4, TIMER_OUTPUT_COMPARE);

    Timer3.setMode(TIMER_CH1, TIMER_OUTPUT_COMPARE);
    Timer3.setMode(TIMER_CH2, TIMER_OUTPUT_COMPARE);
    Timer3.setMode(TIMER_CH3, TIMER_OUTPUT_COMPARE);
    Timer3.setMode(TIMER_CH4, TIMER_OUTPUT_COMPARE);

  #endif
  Timer2.attachInterrupt(1, fuelSchedule1Interrupt);
  Timer2.attachInterrupt(2, fuelSchedule2Interrupt);
  Timer2.attachInterrupt(3, fuelSchedule3Interrupt);
  Timer2.attachInterrupt(4, fuelSchedule4Interrupt);

#if (IGN_CHANNELS >= 1)
  Timer3.attachInterrupt(1, ignitionSchedule1Interrupt);
#endif
#if (IGN_CHANNELS >= 2)
  Timer3.attachInterrupt(2, ignitionSchedule2Interrupt);
#endif
#if (IGN_CHANNELS >= 3)
  Timer3.attachInterrupt(3, ignitionSchedule3Interrupt);
#endif
#if (IGN_CHANNELS >= 4)
  Timer3.attachInterrupt(4, ignitionSchedule4Interrupt);
#endif
#if (IGN_CHANNELS >= 5)
  Timer1.attachInterrupt(1, ignitionSchedule5Interrupt);
#endif

  Timer1.resume();
  Timer2.resume();
  Timer3.resume();
#endif

    fuelSchedule1.Status = OFF;
    fuelSchedule2.Status = OFF;
    fuelSchedule3.Status = OFF;
    fuelSchedule4.Status = OFF;
    fuelSchedule5.Status = OFF;
    fuelSchedule6.Status = OFF;
    fuelSchedule7.Status = OFF;
    fuelSchedule8.Status = OFF;

    fuelSchedule1.schedulesSet = 0;
    fuelSchedule2.schedulesSet = 0;
    fuelSchedule3.schedulesSet = 0;
    fuelSchedule4.schedulesSet = 0;
    fuelSchedule5.schedulesSet = 0;
    fuelSchedule6.schedulesSet = 0;
    fuelSchedule7.schedulesSet = 0;
    fuelSchedule8.schedulesSet = 0;

    fuelSchedule1.counter = &FUEL1_COUNTER;
    fuelSchedule2.counter = &FUEL2_COUNTER;
    fuelSchedule3.counter = &FUEL3_COUNTER;
    fuelSchedule4.counter = &FUEL4_COUNTER;
    fuelSchedule5.counter = &FUEL5_COUNTER;
    fuelSchedule6.counter = &FUEL6_COUNTER;
    fuelSchedule7.counter = &FUEL7_COUNTER;
    fuelSchedule8.counter = &FUEL8_COUNTER;

    ignitionSchedule1.Status = OFF;
    ignitionSchedule2.Status = OFF;
    ignitionSchedule3.Status = OFF;
    ignitionSchedule4.Status = OFF;
    ignitionSchedule5.Status = OFF;
    ignitionSchedule6.Status = OFF;
    ignitionSchedule7.Status = OFF;
    ignitionSchedule8.Status = OFF;

    ignitionSchedule1.schedulesSet = 0;
    ignitionSchedule2.schedulesSet = 0;
    ignitionSchedule3.schedulesSet = 0;
    ignitionSchedule4.schedulesSet = 0;
    ignitionSchedule5.schedulesSet = 0;
    ignitionSchedule6.schedulesSet = 0;
    ignitionSchedule7.schedulesSet = 0;
    ignitionSchedule8.schedulesSet = 0;

  }

/*
These 8 function turn a schedule on, provides the time to start and the duration and gives it callback functions.
All 8 functions operate the same, just on different schedules
Args:
startCallback: The function to be called once the timeout is reached
timeout: The number of uS in the future that the startCallback should be triggered
duration: The number of uS after startCallback is called before endCallback is called
endCallback: This function is called once the duration time has been reached
*/

//Experimental new generic function
void setFuelSchedule(struct Schedule *targetSchedule, unsigned long timeout, unsigned long duration)
{
  if(targetSchedule->Status != RUNNING) //Check that we're not already part way through a schedule
  {
    //Callbacks no longer used, but retained for now:
    //fuelSchedule1.StartCallback = startCallback;
    //fuelSchedule1.EndCallback = endCallback;
    targetSchedule->duration = duration;

    //Need to check that the timeout doesn't exceed the overflow
    uint16_t timeout_timer_compare;
    if (timeout > MAX_TIMER_PERIOD) { timeout_timer_compare = uS_TO_TIMER_COMPARE( (MAX_TIMER_PERIOD - 1) ); } // If the timeout is >16x (Each tick represents 16uS) the maximum allowed value of unsigned int (65535), the timer compare value will overflow when appliedcausing erratic behaviour such as erroneous sparking.
    else { timeout_timer_compare = uS_TO_TIMER_COMPARE(timeout); } //Normal case

    //The following must be enclosed in the noInterupts block to avoid contention caused if the relevant interrupt fires before the state is fully set
    noInterrupts();
    targetSchedule->startCompare = *targetSchedule->counter + timeout_timer_compare;
    targetSchedule->endCompare = targetSchedule->startCompare + uS_TO_TIMER_COMPARE(duration);
    targetSchedule->Status = PENDING; //Turn this schedule on
    targetSchedule->schedulesSet++; //Increment the number of times this schedule has been set

    *targetSchedule->compare = targetSchedule->startCompare;
    interrupts();
    FUEL1_TIMER_ENABLE();
  }
  else
  {
    //If the schedule is already running, we can set the next schedule so it is ready to go
    //This is required in cases of high rpm and high DC where there otherwise would not be enough time to set the schedule
    targetSchedule->nextStartCompare = *targetSchedule->counter + uS_TO_TIMER_COMPARE(timeout);
    targetSchedule->nextEndCompare = targetSchedule->nextStartCompare + uS_TO_TIMER_COMPARE(duration);
    targetSchedule->hasNextSchedule = true;
  }
}


//void setFuelSchedule1(void (*startCallback)(), unsigned long timeout, unsigned long duration, void(*endCallback)())
void setFuelSchedule1(unsigned long timeout, unsigned long duration)
{
  //Check whether timeout exceeds the maximum future time. This can potentially occur on sequential setups when below ~115rpm
  if(timeout < MAX_TIMER_PERIOD_SLOW)
  {
    if(fuelSchedule1.Status != RUNNING) //Check that we're not already part way through a schedule
    {
      //Callbacks no longer used, but retained for now:
      //fuelSchedule1.StartCallback = startCallback;
      //fuelSchedule1.EndCallback = endCallback;
      fuelSchedule1.duration = duration;

      //Need to check that the timeout doesn't exceed the overflow
      uint16_t timeout_timer_compare;
      if ((timeout+duration) > MAX_TIMER_PERIOD_SLOW) { timeout_timer_compare = uS_TO_TIMER_COMPARE_SLOW( (MAX_TIMER_PERIOD_SLOW - 1 - duration) ); } // If the timeout is >16x (Each tick represents 16uS) the maximum allowed value of unsigned int (65535), the timer compare value will overflow when appliedcausing erratic behaviour such as erroneous sparking.
      else { timeout_timer_compare = uS_TO_TIMER_COMPARE_SLOW(timeout); } //Normal case

      //The following must be enclosed in the noInterupts block to avoid contention caused if the relevant interrupt fires before the state is fully set
      noInterrupts();
      fuelSchedule1.startCompare = FUEL1_COUNTER + timeout_timer_compare;
      fuelSchedule1.endCompare = fuelSchedule1.startCompare + uS_TO_TIMER_COMPARE_SLOW(duration);
      fuelSchedule1.Status = PENDING; //Turn this schedule on
      fuelSchedule1.schedulesSet++; //Increment the number of times this schedule has been set
      //Schedule 1 shares a timer with schedule 5
      //if(channel5InjEnabled) { FUEL1_COMPARE = setQueue(timer3Aqueue, &fuelSchedule1, &fuelSchedule5, FUEL1_COUNTER); }
      //else { timer3Aqueue[0] = &fuelSchedule1; timer3Aqueue[1] = &fuelSchedule1; timer3Aqueue[2] = &fuelSchedule1; timer3Aqueue[3] = &fuelSchedule1; FUEL1_COMPARE = fuelSchedule1.startCompare; }
      //timer3Aqueue[0] = &fuelSchedule1; timer3Aqueue[1] = &fuelSchedule1; timer3Aqueue[2] = &fuelSchedule1; timer3Aqueue[3] = &fuelSchedule1;
      FUEL1_COMPARE = fuelSchedule1.startCompare;
      interrupts();
      FUEL1_TIMER_ENABLE();
    }
    else
    {
      //If the schedule is already running, we can set the next schedule so it is ready to go
      //This is required in cases of high rpm and high DC where there otherwise would not be enough time to set the schedule
      noInterrupts();
      fuelSchedule1.nextStartCompare = FUEL1_COUNTER + uS_TO_TIMER_COMPARE_SLOW(timeout);
      fuelSchedule1.nextEndCompare = fuelSchedule1.nextStartCompare + uS_TO_TIMER_COMPARE_SLOW(duration);
      fuelSchedule1.duration = duration;
      fuelSchedule1.hasNextSchedule = true;
      interrupts();
    } //Schedule is RUNNING
  } //Timeout less than threshold
}

void setFuelSchedule2(unsigned long timeout, unsigned long duration)
{
  //Check whether timeout exceeds the maximum future time. This can potentially occur on sequential setups when below ~115rpm
  if(timeout < MAX_TIMER_PERIOD_SLOW)
  {
    if(fuelSchedule2.Status != RUNNING) //Check that we're not already part way through a schedule
    {
      //Callbacks no longer used, but retained for now:
      //fuelSchedule2.StartCallback = startCallback;
      //fuelSchedule2.EndCallback = endCallback;
      fuelSchedule2.duration = duration;

      //Need to check that the timeout doesn't exceed the overflow
      uint16_t timeout_timer_compare;
      if (timeout > MAX_TIMER_PERIOD_SLOW) { timeout_timer_compare = uS_TO_TIMER_COMPARE_SLOW( (MAX_TIMER_PERIOD - 1) ); } // If the timeout is >4x (Each tick represents 4uS) the maximum allowed value of unsigned int (65535), the timer compare value will overflow when appliedcausing erratic behaviour such as erroneous sparking.
      else { timeout_timer_compare = uS_TO_TIMER_COMPARE_SLOW(timeout); } //Normal case

      //The following must be enclosed in the noInterupts block to avoid contention caused if the relevant interrupt fires before the state is fully set
      noInterrupts();
      fuelSchedule2.startCompare = FUEL2_COUNTER + timeout_timer_compare;
      fuelSchedule2.endCompare = fuelSchedule2.startCompare + uS_TO_TIMER_COMPARE_SLOW(duration);
      FUEL2_COMPARE = fuelSchedule2.startCompare; //Use the B compare unit of timer 3
      fuelSchedule2.Status = PENDING; //Turn this schedule on
      fuelSchedule2.schedulesSet++; //Increment the number of times this schedule has been set
      interrupts();
      FUEL2_TIMER_ENABLE();
    }
    else
    {
      //If the schedule is already running, we can set the next schedule so it is ready to go
      //This is required in cases of high rpm and high DC where there otherwise would not be enough time to set the schedule
      fuelSchedule2.nextStartCompare = FUEL2_COUNTER + uS_TO_TIMER_COMPARE_SLOW(timeout);
      fuelSchedule2.nextEndCompare = fuelSchedule2.nextStartCompare + uS_TO_TIMER_COMPARE_SLOW(duration);
      fuelSchedule2.hasNextSchedule = true;
    }
  }
}
//void setFuelSchedule3(void (*startCallback)(), unsigned long timeout, unsigned long duration, void(*endCallback)())
void setFuelSchedule3(unsigned long timeout, unsigned long duration)
{
  //Check whether timeout exceeds the maximum future time. This can potentially occur on sequential setups when below ~115rpm
  if(timeout < MAX_TIMER_PERIOD_SLOW)
  {
    if(fuelSchedule3.Status != RUNNING)//Check that we're not already part way through a schedule
    {
      //Callbacks no longer used, but retained for now:
      //fuelSchedule3.StartCallback = startCallback;
      //fuelSchedule3.EndCallback = endCallback;
      fuelSchedule3.duration = duration;

      //Need to check that the timeout doesn't exceed the overflow
      uint16_t timeout_timer_compare;
      if (timeout > MAX_TIMER_PERIOD_SLOW) { timeout_timer_compare = uS_TO_TIMER_COMPARE_SLOW( (MAX_TIMER_PERIOD - 1) ); } // If the timeout is >4x (Each tick represents 4uS) the maximum allowed value of unsigned int (65535), the timer compare value will overflow when appliedcausing erratic behaviour such as erroneous sparking.
      else { timeout_timer_compare = uS_TO_TIMER_COMPARE_SLOW(timeout); } //Normal case

      //The following must be enclosed in the noInterupts block to avoid contention caused if the relevant interrupt fires before the state is fully set
      noInterrupts();
      fuelSchedule3.startCompare = FUEL3_COUNTER + timeout_timer_compare;
      fuelSchedule3.endCompare = fuelSchedule3.startCompare + uS_TO_TIMER_COMPARE_SLOW(duration);
      FUEL3_COMPARE = fuelSchedule3.startCompare; //Use the C copmare unit of timer 3
      fuelSchedule3.Status = PENDING; //Turn this schedule on
      fuelSchedule3.schedulesSet++; //Increment the number of times this schedule has been set
      interrupts();
      FUEL3_TIMER_ENABLE();
    }
    else
    {
      //If the schedule is already running, we can set the next schedule so it is ready to go
      //This is required in cases of high rpm and high DC where there otherwise would not be enough time to set the schedule
      fuelSchedule3.nextStartCompare = FUEL3_COUNTER + uS_TO_TIMER_COMPARE_SLOW(timeout);
      fuelSchedule3.nextEndCompare = fuelSchedule3.nextStartCompare + uS_TO_TIMER_COMPARE_SLOW(duration);
      fuelSchedule3.hasNextSchedule = true;
    }
  }
}
//void setFuelSchedule4(void (*startCallback)(), unsigned long timeout, unsigned long duration, void(*endCallback)())
void setFuelSchedule4(unsigned long timeout, unsigned long duration) //Uses timer 4 compare B
{
  //Check whether timeout exceeds the maximum future time. This can potentially occur on sequential setups when below ~115rpm
  if(timeout < MAX_TIMER_PERIOD_SLOW)
  {
    if(fuelSchedule4.Status != RUNNING) //Check that we're not already part way through a schedule
    {
      //Callbacks no longer used, but retained for now:
      //fuelSchedule4.StartCallback = startCallback;
      //fuelSchedule4.EndCallback = endCallback;
      fuelSchedule4.duration = duration;

      //Need to check that the timeout doesn't exceed the overflow
      uint16_t timeout_timer_compare;
      if (timeout > MAX_TIMER_PERIOD_SLOW) { timeout_timer_compare = uS_TO_TIMER_COMPARE_SLOW( (MAX_TIMER_PERIOD - 1) ); } // If the timeout is >4x (Each tick represents 4uS) the maximum allowed value of unsigned int (65535), the timer compare value will overflow when appliedcausing erratic behaviour such as erroneous sparking.
      else { timeout_timer_compare = uS_TO_TIMER_COMPARE_SLOW(timeout); } //Normal case

      //The following must be enclosed in the noInterupts block to avoid contention caused if the relevant interrupt fires before the state is fully set
      noInterrupts();
      fuelSchedule4.startCompare = FUEL4_COUNTER + timeout_timer_compare;
      fuelSchedule4.endCompare = fuelSchedule4.startCompare + uS_TO_TIMER_COMPARE_SLOW(duration);
      FUEL4_COMPARE = fuelSchedule4.startCompare; //Use the C copmare unit of timer 3
      fuelSchedule4.Status = PENDING; //Turn this schedule on
      fuelSchedule4.schedulesSet++; //Increment the number of times this schedule has been set
      interrupts();
      FUEL4_TIMER_ENABLE();
    }
    else
    {
      //If the schedule is already running, we can set the next schedule so it is ready to go
      //This is required in cases of high rpm and high DC where there otherwise would not be enough time to set the schedule
      fuelSchedule4.nextStartCompare = FUEL4_COUNTER + uS_TO_TIMER_COMPARE_SLOW(timeout);
      fuelSchedule4.nextEndCompare = fuelSchedule4.nextStartCompare + uS_TO_TIMER_COMPARE_SLOW(duration);
      fuelSchedule4.hasNextSchedule = true;
    }
  }
}

void setFuelSchedule5(void (*startCallback)(), unsigned long timeout, unsigned long duration, void(*endCallback)())
{
  if(fuelSchedule5.Status != RUNNING) //Check that we're not already part way through a schedule
  {
    fuelSchedule5.StartCallback = startCallback; //Name the start callback function
    fuelSchedule5.EndCallback = endCallback; //Name the end callback function
    fuelSchedule5.duration = duration;

    /*
     * The following must be enclosed in the noIntterupts block to avoid contention caused if the relevant interrupts fires before the state is fully set
     */
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__)
    noInterrupts();
    fuelSchedule5.startCompare = TCNT3 + (timeout >> 4); //As above, but with bit shift instead of / 16
    fuelSchedule5.endCompare = fuelSchedule5.startCompare + (duration >> 4);
    fuelSchedule5.Status = PENDING; //Turn this schedule on
    fuelSchedule5.schedulesSet++; //Increment the number of times this schedule has been set
    OCR3A = setQueue(timer3Aqueue, &fuelSchedule1, &fuelSchedule5, TCNT3); //Schedule 1 shares a timer with schedule 5
    interrupts();
    TIMSK3 |= (1 << OCIE3A); //Turn on the A compare unit (ie turn on the interrupt)
#endif
  }
  else
  {
    //If the schedule is already running, we can set the next schedule so it is ready to go
    //This is required in cases of high rpm and high DC where there otherwise would not be enough time to set the schedule
    fuelSchedule5.nextStartCompare = FUEL5_COUNTER + uS_TO_TIMER_COMPARE_SLOW(timeout);
    fuelSchedule5.nextEndCompare = fuelSchedule5.nextStartCompare + uS_TO_TIMER_COMPARE_SLOW(duration);
    fuelSchedule5.hasNextSchedule = true;
  }
}

#if INJ_CHANNELS >= 6
//This uses timer
void setFuelSchedule6(unsigned long timeout, unsigned long duration)
{
  if(fuelSchedule6.Status != RUNNING) //Check that we're not already part way through a schedule
  {
    //Callbacks no longer used, but retained for now:
    //fuelSchedule4.StartCallback = startCallback;
    //fuelSchedule4.EndCallback = endCallback;
    fuelSchedule6.duration = duration;

    //Need to check that the timeout doesn't exceed the overflow
    uint16_t timeout_timer_compare;
    if (timeout > MAX_TIMER_PERIOD_SLOW) { timeout_timer_compare = uS_TO_TIMER_COMPARE_SLOW( (MAX_TIMER_PERIOD - 1) ); } // If the timeout is >4x (Each tick represents 4uS) the maximum allowed value of unsigned int (65535), the timer compare value will overflow when appliedcausing erratic behaviour such as erroneous sparking.
    else { timeout_timer_compare = uS_TO_TIMER_COMPARE_SLOW(timeout); } //Normal case

    //The following must be enclosed in the noInterupts block to avoid contention caused if the relevant interrupt fires before the state is fully set
    noInterrupts();
    fuelSchedule6.startCompare = FUEL6_COUNTER + timeout_timer_compare;
    fuelSchedule6.endCompare = fuelSchedule6.startCompare + uS_TO_TIMER_COMPARE_SLOW(duration);
    FUEL6_COMPARE = fuelSchedule6.startCompare; //Use the C copmare unit of timer 3
    fuelSchedule6.Status = PENDING; //Turn this schedule on
    fuelSchedule6.schedulesSet++; //Increment the number of times this schedule has been set
    interrupts();
    FUEL6_TIMER_ENABLE();
  }
  else
  {
    //If the schedule is already running, we can set the next schedule so it is ready to go
    //This is required in cases of high rpm and high DC where there otherwise would not be enough time to set the schedule
    fuelSchedule6.nextStartCompare = FUEL6_COUNTER + uS_TO_TIMER_COMPARE_SLOW(timeout);
    fuelSchedule6.nextEndCompare = fuelSchedule6.nextStartCompare + uS_TO_TIMER_COMPARE_SLOW(duration);
    fuelSchedule6.hasNextSchedule = true;
  }
}
#endif

#if INJ_CHANNELS >= 7
//This uses timer
void setFuelSchedule7(unsigned long timeout, unsigned long duration)
{
  if(fuelSchedule7.Status != RUNNING) //Check that we're not already part way through a schedule
  {
    //Callbacks no longer used, but retained for now:
    //fuelSchedule4.StartCallback = startCallback;
    //fuelSchedule4.EndCallback = endCallback;
    fuelSchedule7.duration = duration;

    //Need to check that the timeout doesn't exceed the overflow
    uint16_t timeout_timer_compare;
    if (timeout > MAX_TIMER_PERIOD) { timeout_timer_compare = uS_TO_TIMER_COMPARE( (MAX_TIMER_PERIOD - 1) ); } // If the timeout is >4x (Each tick represents 4uS) the maximum allowed value of unsigned int (65535), the timer compare value will overflow when appliedcausing erratic behaviour such as erroneous sparking.
    else { timeout_timer_compare = uS_TO_TIMER_COMPARE(timeout); } //Normal case

    //The following must be enclosed in the noInterupts block to avoid contention caused if the relevant interrupt fires before the state is fully set
    noInterrupts();
    fuelSchedule7.startCompare = FUEL7_COUNTER + timeout_timer_compare;
    fuelSchedule7.endCompare = fuelSchedule6.startCompare + uS_TO_TIMER_COMPARE(duration);
    FUEL7_COMPARE = fuelSchedule7.startCompare; //Use the C copmare unit of timer 3
    fuelSchedule7.Status = PENDING; //Turn this schedule on
    fuelSchedule7.schedulesSet++; //Increment the number of times this schedule has been set
    interrupts();
    FUEL7_TIMER_ENABLE();
  }
  else
  {
    //If the schedule is already running, we can set the next schedule so it is ready to go
    //This is required in cases of high rpm and high DC where there otherwise would not be enough time to set the schedule
    fuelSchedule7.nextStartCompare = FUEL7_COUNTER + uS_TO_TIMER_COMPARE(timeout);
    fuelSchedule7.nextEndCompare = fuelSchedule7.nextStartCompare + uS_TO_TIMER_COMPARE(duration);
    fuelSchedule7.hasNextSchedule = true;
  }
}
#endif

#if INJ_CHANNELS >= 8
//This uses timer
void setFuelSchedule8(unsigned long timeout, unsigned long duration)
{
  if(fuelSchedule8.Status != RUNNING) //Check that we're not already part way through a schedule
  {
    //Callbacks no longer used, but retained for now:
    //fuelSchedule4.StartCallback = startCallback;
    //fuelSchedule4.EndCallback = endCallback;
    fuelSchedule8.duration = duration;

    //Need to check that the timeout doesn't exceed the overflow
    uint16_t timeout_timer_compare;
    if (timeout > MAX_TIMER_PERIOD) { timeout_timer_compare = uS_TO_TIMER_COMPARE( (MAX_TIMER_PERIOD - 1) ); } // If the timeout is >4x (Each tick represents 4uS) the maximum allowed value of unsigned int (65535), the timer compare value will overflow when appliedcausing erratic behaviour such as erroneous sparking.
    else { timeout_timer_compare = uS_TO_TIMER_COMPARE(timeout); } //Normal case

    //The following must be enclosed in the noInterupts block to avoid contention caused if the relevant interrupt fires before the state is fully set
    noInterrupts();
    fuelSchedule8.startCompare = FUEL8_COUNTER + timeout_timer_compare;
    fuelSchedule8.endCompare = fuelSchedule8.startCompare + uS_TO_TIMER_COMPARE(duration);
    FUEL8_COMPARE = fuelSchedule8.startCompare; //Use the C copmare unit of timer 3
    fuelSchedule8.Status = PENDING; //Turn this schedule on
    fuelSchedule8.schedulesSet++; //Increment the number of times this schedule has been set
    interrupts();
    FUEL8_TIMER_ENABLE();
  }
  else
  {
    //If the schedule is already running, we can set the next schedule so it is ready to go
    //This is required in cases of high rpm and high DC where there otherwise would not be enough time to set the schedule
    fuelSchedule8.nextStartCompare = FUEL8_COUNTER + uS_TO_TIMER_COMPARE(timeout);
    fuelSchedule8.nextEndCompare = fuelSchedule8.nextStartCompare + uS_TO_TIMER_COMPARE(duration);
    fuelSchedule8.hasNextSchedule = true;
  }
}
#endif

//Ignition schedulers use Timer 5
void setIgnitionSchedule1(void (*startCallback)(), unsigned long timeout, unsigned long duration, void(*endCallback)())
{
  if(ignitionSchedule1.Status != RUNNING) //Check that we're not already part way through a schedule
  {
    ignitionSchedule1.StartCallback = startCallback; //Name the start callback function
    ignitionSchedule1.EndCallback = endCallback; //Name the start callback function
    ignitionSchedule1.duration = duration;

    //Need to check that the timeout doesn't exceed the overflow
    uint16_t timeout_timer_compare;
    if (timeout > MAX_TIMER_PERIOD) { timeout_timer_compare = uS_TO_TIMER_COMPARE( (MAX_TIMER_PERIOD - 1) ); } // If the timeout is >4x (Each tick represents 4uS) the maximum allowed value of unsigned int (65535), the timer compare value will overflow when appliedcausing erratic behaviour such as erroneous sparking.
    else { timeout_timer_compare = uS_TO_TIMER_COMPARE(timeout); } //Normal case

    noInterrupts();
    ignitionSchedule1.startCompare = IGN1_COUNTER + timeout_timer_compare; //As there is a tick every 4uS, there are timeout/4 ticks until the interrupt should be triggered ( >>2 divides by 4)
    ignitionSchedule1.endCompare = ignitionSchedule1.startCompare + uS_TO_TIMER_COMPARE(duration);
    IGN1_COMPARE = ignitionSchedule1.startCompare;
    ignitionSchedule1.Status = PENDING; //Turn this schedule on
    ignitionSchedule1.schedulesSet++;
    interrupts();
    IGN1_TIMER_ENABLE();
  }
}

static inline void refreshIgnitionSchedule1(unsigned long timeToEnd)
{
  if( (ignitionSchedule1.Status == RUNNING) && (timeToEnd < ignitionSchedule1.duration) )
  //Must have the threshold check here otherwise it can cause a condition where the compare fires twice, once after the other, both for the end
  //if( (timeToEnd < ignitionSchedule1.duration) && (timeToEnd > IGNITION_REFRESH_THRESHOLD) )
  {
    noInterrupts();
    ignitionSchedule1.endCompare = IGN1_COUNTER + uS_TO_TIMER_COMPARE(timeToEnd);
    IGN1_COMPARE = ignitionSchedule1.endCompare;
    interrupts();
  }
}

void setIgnitionSchedule2(void (*startCallback)(), unsigned long timeout, unsigned long duration, void(*endCallback)())
{
  if(ignitionSchedule2.Status != RUNNING) //Check that we're not already part way through a schedule
  {
    ignitionSchedule2.StartCallback = startCallback; //Name the start callback function
    ignitionSchedule2.EndCallback = endCallback; //Name the start callback function
    ignitionSchedule2.duration = duration;

    //Need to check that the timeout doesn't exceed the overflow
    uint16_t timeout_timer_compare;
    if (timeout > MAX_TIMER_PERIOD) { timeout_timer_compare = uS_TO_TIMER_COMPARE( (MAX_TIMER_PERIOD - 1) ); } // If the timeout is >4x (Each tick represents 4uS) the maximum allowed value of unsigned int (65535), the timer compare value will overflow when appliedcausing erratic behaviour such as erroneous sparking.
    else { timeout_timer_compare = uS_TO_TIMER_COMPARE(timeout); } //Normal case

    noInterrupts();
    ignitionSchedule2.startCompare = IGN2_COUNTER + timeout_timer_compare; //As there is a tick every 4uS, there are timeout/4 ticks until the interrupt should be triggered ( >>2 divides by 4)
    ignitionSchedule2.endCompare = ignitionSchedule2.startCompare + uS_TO_TIMER_COMPARE(duration);
    IGN2_COMPARE = ignitionSchedule2.startCompare;
    ignitionSchedule2.Status = PENDING; //Turn this schedule on
    ignitionSchedule2.schedulesSet++;
    interrupts();
    IGN2_TIMER_ENABLE();
  }
}
void setIgnitionSchedule3(void (*startCallback)(), unsigned long timeout, unsigned long duration, void(*endCallback)())
{
  if(ignitionSchedule3.Status != RUNNING) //Check that we're not already part way through a schedule
  {

    ignitionSchedule3.StartCallback = startCallback; //Name the start callback function
    ignitionSchedule3.EndCallback = endCallback; //Name the start callback function
    ignitionSchedule3.duration = duration;

    //Need to check that the timeout doesn't exceed the overflow
    uint16_t timeout_timer_compare;
    if (timeout > MAX_TIMER_PERIOD) { timeout_timer_compare = uS_TO_TIMER_COMPARE( (MAX_TIMER_PERIOD - 1) ); } // If the timeout is >4x (Each tick represents 4uS) the maximum allowed value of unsigned int (65535), the timer compare value will overflow when appliedcausing erratic behaviour such as erroneous sparking.
    else { timeout_timer_compare = uS_TO_TIMER_COMPARE(timeout); } //Normal case

    noInterrupts();
    ignitionSchedule3.startCompare = IGN3_COUNTER + timeout_timer_compare; //As there is a tick every 4uS, there are timeout/4 ticks until the interrupt should be triggered ( >>2 divides by 4)
    ignitionSchedule3.endCompare = ignitionSchedule3.startCompare + uS_TO_TIMER_COMPARE(duration);
    IGN3_COMPARE = ignitionSchedule3.startCompare;
    ignitionSchedule3.Status = PENDING; //Turn this schedule on
    ignitionSchedule3.schedulesSet++;
    interrupts();
    IGN3_TIMER_ENABLE();
  }
  else
  {
    //If the schedule is already running, we can set the next schedule so it is ready to go
    //This is required in cases of high rpm and high DC where there otherwise would not be enough time to set the schedule
    ignitionSchedule3.nextStartCompare = IGN3_COUNTER + uS_TO_TIMER_COMPARE(timeout);
    ignitionSchedule3.nextEndCompare = ignitionSchedule3.nextStartCompare + uS_TO_TIMER_COMPARE(duration);
    ignitionSchedule3.hasNextSchedule = true;
  }
}
void setIgnitionSchedule4(void (*startCallback)(), unsigned long timeout, unsigned long duration, void(*endCallback)())
{
  if(ignitionSchedule4.Status != RUNNING) //Check that we're not already part way through a schedule
  {

    ignitionSchedule4.StartCallback = startCallback; //Name the start callback function
    ignitionSchedule4.EndCallback = endCallback; //Name the start callback function
    ignitionSchedule4.duration = duration;

    //Need to check that the timeout doesn't exceed the overflow
    uint16_t timeout_timer_compare;
    if (timeout > MAX_TIMER_PERIOD) { timeout_timer_compare = uS_TO_TIMER_COMPARE_SLOW( (MAX_TIMER_PERIOD - 1) ); } // If the timeout is >4x (Each tick represents 4uS) the maximum allowed value of unsigned int (65535), the timer compare value will overflow when appliedcausing erratic behaviour such as erroneous sparking.
    else { timeout_timer_compare = uS_TO_TIMER_COMPARE_SLOW(timeout); } //Normal case

    noInterrupts();
    ignitionSchedule4.startCompare = IGN4_COUNTER + timeout_timer_compare;
    ignitionSchedule4.endCompare = ignitionSchedule4.startCompare + uS_TO_TIMER_COMPARE_SLOW(duration);
    IGN4_COMPARE = ignitionSchedule4.startCompare;
    ignitionSchedule4.Status = PENDING; //Turn this schedule on
    ignitionSchedule4.schedulesSet++;
    interrupts();
    IGN4_TIMER_ENABLE();
  }
  else
  {
    //If the schedule is already running, we can set the next schedule so it is ready to go
    //This is required in cases of high rpm and high DC where there otherwise would not be enough time to set the schedule
    ignitionSchedule4.nextStartCompare = IGN4_COUNTER + uS_TO_TIMER_COMPARE_SLOW(timeout);
    ignitionSchedule4.nextEndCompare = ignitionSchedule4.nextStartCompare + uS_TO_TIMER_COMPARE_SLOW(duration);
    ignitionSchedule4.hasNextSchedule = true;
  }
}
void setIgnitionSchedule5(void (*startCallback)(), unsigned long timeout, unsigned long duration, void(*endCallback)())
{
  if(ignitionSchedule5.Status != RUNNING)//Check that we're not already part way through a schedule
  {

    ignitionSchedule5.StartCallback = startCallback; //Name the start callback function
    ignitionSchedule5.EndCallback = endCallback; //Name the start callback function
    ignitionSchedule5.duration = duration;

    //Need to check that the timeout doesn't exceed the overflow
    uint16_t timeout_timer_compare;
    if (timeout > MAX_TIMER_PERIOD) { timeout_timer_compare = uS_TO_TIMER_COMPARE_SLOW( (MAX_TIMER_PERIOD - 1) ); } // If the timeout is >4x (Each tick represents 4uS) the maximum allowed value of unsigned int (65535), the timer compare value will overflow when appliedcausing erratic behaviour such as erroneous sparking.
    else { timeout_timer_compare = uS_TO_TIMER_COMPARE_SLOW(timeout); } //Normal case

    noInterrupts();
    ignitionSchedule5.startCompare = IGN5_COUNTER + timeout_timer_compare;
    ignitionSchedule5.endCompare = ignitionSchedule5.startCompare + uS_TO_TIMER_COMPARE_SLOW(duration);
    IGN5_COMPARE = ignitionSchedule5.startCompare;
    ignitionSchedule5.Status = PENDING; //Turn this schedule on
    ignitionSchedule5.schedulesSet++;
    interrupts();
    IGN5_TIMER_ENABLE();
  }
}

/*******************************************************************************************************************************************************************************************************/
//This function (All 8 ISR functions that are below) gets called when either the start time or the duration time are reached
//This calls the relevant callback function (startCallback or endCallback) depending on the status of the schedule.
//If the startCallback function is called, we put the scheduler into RUNNING state
//Timer3A (fuel schedule 1) Compare Vector
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__) //AVR chips use the ISR for this
ISR(TIMER3_COMPA_vect) //fuelSchedules 1 and 5
#elif defined (CORE_TEENSY) || defined(CORE_STM32)
static inline void fuelSchedule1Interrupt() //Most ARM chips can simply call a function
#endif
  {
    if (fuelSchedule1.Status == PENDING) //Check to see if this schedule is turn on
    {
      //To use timer queue, change fuelShedule1 to timer3Aqueue[0];
      if (configPage2.injLayout == INJ_SEMISEQUENTIAL) { openInjector1and4(); }
      else { openInjector1(); }
      fuelSchedule1.Status = RUNNING; //Set the status to be in progress (ie The start callback has been called, but not the end callback)
      FUEL1_COMPARE = FUEL1_COUNTER + uS_TO_TIMER_COMPARE_SLOW(fuelSchedule1.duration); //Doing this here prevents a potential overflow on restarts
    }
    else if (fuelSchedule1.Status == RUNNING)
    {
       //timer3Aqueue[0]->EndCallback();
       if (configPage2.injLayout == INJ_SEMISEQUENTIAL) { closeInjector1and4(); }
       else { closeInjector1(); }
       fuelSchedule1.Status = OFF; //Turn off the schedule
       fuelSchedule1.schedulesSet = 0;
       //FUEL1_COMPARE = fuelSchedule1.endCompare;

       //If there is a next schedule queued up, activate it
       if(fuelSchedule1.hasNextSchedule == true)
       {
         FUEL1_COMPARE = fuelSchedule1.nextStartCompare;
         fuelSchedule1.endCompare = fuelSchedule1.nextEndCompare;
         fuelSchedule1.Status = PENDING;
         fuelSchedule1.schedulesSet = 1;
         fuelSchedule1.hasNextSchedule = false;
       }
       else { FUEL1_TIMER_DISABLE(); }
    }
    else if (fuelSchedule1.Status == OFF) { FUEL1_TIMER_DISABLE(); } //Safety check. Turn off this output compare unit and return without performing any action
  }

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__) //AVR chips use the ISR for this
ISR(TIMER3_COMPB_vect) //fuelSchedule2
#elif defined (CORE_TEENSY) || defined(CORE_STM32)
static inline void fuelSchedule2Interrupt() //Most ARM chips can simply call a function
#endif
  {
    if (fuelSchedule2.Status == PENDING) //Check to see if this schedule is turn on
    {
      //fuelSchedule2.StartCallback();
      if (configPage2.injLayout == INJ_SEMISEQUENTIAL) { openInjector2and3(); }
      else { openInjector2(); }
      fuelSchedule2.Status = RUNNING; //Set the status to be in progress (ie The start callback has been called, but not the end callback)
      FUEL2_COMPARE = FUEL2_COUNTER + uS_TO_TIMER_COMPARE_SLOW(fuelSchedule2.duration); //Doing this here prevents a potential overflow on restarts
    }
    else if (fuelSchedule2.Status == RUNNING)
    {
       //fuelSchedule2.EndCallback();
       if (configPage2.injLayout == INJ_SEMISEQUENTIAL) { closeInjector2and3(); }
       else { closeInjector2(); }
       fuelSchedule2.Status = OFF; //Turn off the schedule
       fuelSchedule2.schedulesSet = 0;

       //If there is a next schedule queued up, activate it
       if(fuelSchedule2.hasNextSchedule == true)
       {
         FUEL2_COMPARE = fuelSchedule2.nextStartCompare;
         fuelSchedule2.endCompare = fuelSchedule2.nextEndCompare;
         fuelSchedule2.Status = PENDING;
         fuelSchedule2.schedulesSet = 1;
         fuelSchedule2.hasNextSchedule = false;
       }
       else { FUEL2_TIMER_DISABLE(); }
    }
  }

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__) //AVR chips use the ISR for this
ISR(TIMER3_COMPC_vect) //fuelSchedule3
#elif defined (CORE_TEENSY) || defined(CORE_STM32)
static inline void fuelSchedule3Interrupt() //Most ARM chips can simply call a function
#endif
  {
    if (fuelSchedule3.Status == PENDING) //Check to see if this schedule is turn on
    {
      //fuelSchedule3.StartCallback();
      //Hack for 5 cylinder
      if(channel5InjEnabled) { openInjector3and5(); }
      else { openInjector3(); }
      fuelSchedule3.Status = RUNNING; //Set the status to be in progress (ie The start callback has been called, but not the end callback)
      FUEL3_COMPARE = FUEL3_COUNTER + uS_TO_TIMER_COMPARE_SLOW(fuelSchedule3.duration); //Doing this here prevents a potential overflow on restarts
    }
    else if (fuelSchedule3.Status == RUNNING)
    {
       //fuelSchedule3.EndCallback();
       //Hack for 5 cylinder
       if(channel5InjEnabled) { closeInjector3and5(); }
       else { closeInjector3and5(); }
       fuelSchedule3.Status = OFF; //Turn off the schedule
       fuelSchedule3.schedulesSet = 0;

       //If there is a next schedule queued up, activate it
       if(fuelSchedule3.hasNextSchedule == true)
       {
         FUEL3_COMPARE = fuelSchedule3.nextStartCompare;
         fuelSchedule3.endCompare = fuelSchedule3.nextEndCompare;
         fuelSchedule3.Status = PENDING;
         fuelSchedule3.schedulesSet = 1;
         fuelSchedule3.hasNextSchedule = false;
       }
       else { FUEL3_TIMER_DISABLE(); }
    }
  }

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__) //AVR chips use the ISR for this
ISR(TIMER4_COMPB_vect) //fuelSchedule4
#elif defined (CORE_TEENSY) || defined(CORE_STM32)
static inline void fuelSchedule4Interrupt() //Most ARM chips can simply call a function
#endif
  {
    if (fuelSchedule4.Status == PENDING) //Check to see if this schedule is turn on
    {
      //fuelSchedule4.StartCallback();
      openInjector4();
      fuelSchedule4.Status = RUNNING; //Set the status to be in progress (ie The start callback has been called, but not the end callback)
      FUEL4_COMPARE = FUEL4_COUNTER + uS_TO_TIMER_COMPARE_SLOW(fuelSchedule4.duration); //Doing this here prevents a potential overflow on restarts
    }
    else if (fuelSchedule4.Status == RUNNING)
    {
       //fuelSchedule4.EndCallback();
       closeInjector4();
       fuelSchedule4.Status = OFF; //Turn off the schedule
       fuelSchedule4.schedulesSet = 0;

       //If there is a next schedule queued up, activate it
       if(fuelSchedule4.hasNextSchedule == true)
       {
         FUEL4_COMPARE = fuelSchedule4.nextStartCompare;
         fuelSchedule4.endCompare = fuelSchedule4.nextEndCompare;
         fuelSchedule4.Status = PENDING;
         fuelSchedule4.schedulesSet = 1;
         fuelSchedule4.hasNextSchedule = false;
       }
       else { FUEL4_TIMER_DISABLE(); }
    }
  }

#if (INJ_CHANNELS >= 6)
#if defined(CORE_AVR) //AVR chips use the ISR for this
ISR(TIMER4_COMPA_vect) //fuelSchedule6
#elif defined (CORE_TEENSY) || defined(CORE_STM32)
static inline void fuelSchedule6Interrupt() //Most ARM chips can simply call a function
#endif
{
  if (fuelSchedule6.Status == PENDING) //Check to see if this schedule is turn on
  {
    //fuelSchedule4.StartCallback();
    openInjector6();
    fuelSchedule6.Status = RUNNING; //Set the status to be in progress (ie The start callback has been called, but not the end callback)
    FUEL6_COMPARE = fuelSchedule6.endCompare;
  }
  else if (fuelSchedule6.Status == RUNNING)
  {
     //fuelSchedule4.EndCallback();
     closeInjector6();
     fuelSchedule6.Status = OFF; //Turn off the schedule
     fuelSchedule6.schedulesSet = 0;

     //If there is a next schedule queued up, activate it
     if(fuelSchedule6.hasNextSchedule == true)
     {
       FUEL6_COMPARE = fuelSchedule6.nextStartCompare;
       fuelSchedule6.endCompare = fuelSchedule6.nextEndCompare;
       fuelSchedule6.Status = PENDING;
       fuelSchedule6.schedulesSet = 1;
       fuelSchedule6.hasNextSchedule = false;
     }
     else { FUEL6_TIMER_DISABLE(); }
  }
}
#endif

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__) //AVR chips use the ISR for this
ISR(TIMER5_COMPA_vect) //ignitionSchedule1
#elif defined (CORE_TEENSY) || defined(CORE_STM32)
static inline void ignitionSchedule1Interrupt() //Most ARM chips can simply call a function
#endif
  {
    if (ignitionSchedule1.Status == PENDING) //Check to see if this schedule is turn on
    {
      ignitionSchedule1.StartCallback();
      ignitionSchedule1.Status = RUNNING; //Set the status to be in progress (ie The start callback has been called, but not the end callback)
      ignitionSchedule1.startTime = micros();
      //IGN1_COMPARE = ignitionSchedule1.endCompare;
      IGN1_COMPARE = IGN1_COUNTER + uS_TO_TIMER_COMPARE(ignitionSchedule1.duration); //Doing this here prevents a potential overflow on restarts
      //This code is all to do with the staged ignition timing testing. That is, calling this interrupt slightly before the true ignition point and recalculating the end time for more accuracy
      //IGN1_COMPARE = ignitionSchedule1.endCompare - 50;
      //ignitionSchedule1.Status = STAGED;
    }
    else if (ignitionSchedule1.Status == STAGED)
    {
      int16_t crankAngle = getCrankAngle(timePerDegree);
      if(crankAngle > CRANK_ANGLE_MAX_IGN) { crankAngle -= CRANK_ANGLE_MAX_IGN; }
      if(ignition1EndAngle > crankAngle)
      {
        IGN1_COMPARE = IGN1_COUNTER + uS_TO_TIMER_COMPARE( fastDegreesToUS((ignition1EndAngle - crankAngle)) );
      }
      else { IGN1_COMPARE = ignitionSchedule1.endCompare; }

      ignitionSchedule1.Status = RUNNING;
    }
    else if (ignitionSchedule1.Status == RUNNING)
    {
      ignitionSchedule1.EndCallback();
      ignitionSchedule1.Status = OFF; //Turn off the schedule
      ignitionSchedule1.schedulesSet = 0;
      ignitionCount += 1; //Increment the igintion counter
      IGN1_TIMER_DISABLE();
    }
  }

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__) //AVR chips use the ISR for this
ISR(TIMER5_COMPB_vect) //ignitionSchedule2
#elif defined (CORE_TEENSY) || defined(CORE_STM32)
static inline void ignitionSchedule2Interrupt() //Most ARM chips can simply call a function
#endif
  {
    if (ignitionSchedule2.Status == PENDING) //Check to see if this schedule is turn on
    {
      ignitionSchedule2.StartCallback();
      ignitionSchedule2.Status = RUNNING; //Set the status to be in progress (ie The start callback has been called, but not the end callback)
      ignitionSchedule2.startTime = micros();
      IGN2_COMPARE = IGN2_COUNTER + uS_TO_TIMER_COMPARE(ignitionSchedule2.duration); //Doing this here prevents a potential overflow on restarts
    }
    else if (ignitionSchedule2.Status == RUNNING)
    {
      ignitionSchedule2.Status = OFF; //Turn off the schedule
      ignitionSchedule2.EndCallback();
      ignitionSchedule2.schedulesSet = 0;
      ignitionCount += 1; //Increment the igintion counter
      IGN2_TIMER_DISABLE();
    }
  }

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__) //AVR chips use the ISR for this
ISR(TIMER5_COMPC_vect) //ignitionSchedule3
#elif defined (CORE_TEENSY) || defined(CORE_STM32)
static inline void ignitionSchedule3Interrupt() //Most ARM chips can simply call a function
#endif
  {
    if (ignitionSchedule3.Status == PENDING) //Check to see if this schedule is turn on
    {
      ignitionSchedule3.StartCallback();
      ignitionSchedule3.Status = RUNNING; //Set the status to be in progress (ie The start callback has been called, but not the end callback)
      ignitionSchedule3.startTime = micros();
      IGN3_COMPARE = IGN3_COUNTER + uS_TO_TIMER_COMPARE(ignitionSchedule3.duration); //Doing this here prevents a potential overflow on restarts
    }
    else if (ignitionSchedule3.Status == RUNNING)
    {
       ignitionSchedule3.Status = OFF; //Turn off the schedule
       ignitionSchedule3.EndCallback();
       ignitionSchedule3.schedulesSet = 0;
       ignitionCount += 1; //Increment the igintion counter

       //If there is a next schedule queued up, activate it
       if(ignitionSchedule3.hasNextSchedule == true)
       {
         IGN3_COMPARE = ignitionSchedule3.nextStartCompare;
         ignitionSchedule3.endCompare = ignitionSchedule3.nextEndCompare;
         ignitionSchedule3.Status = PENDING;
         ignitionSchedule3.schedulesSet = 1;
         ignitionSchedule3.hasNextSchedule = false;
       }
       else { IGN3_TIMER_DISABLE(); }
    }
  }

#if defined(CORE_AVR) //AVR chips use the ISR for this
ISR(TIMER4_COMPA_vect) //ignitionSchedule4
#elif defined (CORE_TEENSY) || defined(CORE_STM32)
static inline void ignitionSchedule4Interrupt() //Most ARM chips can simply call a function
#endif
  {
    if (ignitionSchedule4.Status == PENDING) //Check to see if this schedule is turn on
    {
      ignitionSchedule4.StartCallback();
      ignitionSchedule4.Status = RUNNING; //Set the status to be in progress (ie The start callback has been called, but not the end callback)
      ignitionSchedule4.startTime = micros();
      IGN4_COMPARE = IGN4_COUNTER + uS_TO_TIMER_COMPARE_SLOW(ignitionSchedule4.duration); //Doing this here prevents a potential overflow on restarts
    }
    else if (ignitionSchedule4.Status == RUNNING)
    {
       ignitionSchedule4.Status = OFF; //Turn off the schedule
       ignitionSchedule4.EndCallback();
       ignitionSchedule4.schedulesSet = 0;
       ignitionCount += 1; //Increment the igintion counter

       //If there is a next schedule queued up, activate it
       if(ignitionSchedule4.hasNextSchedule == true)
       {
         IGN4_COMPARE = ignitionSchedule4.nextStartCompare;
         ignitionSchedule4.endCompare = ignitionSchedule4.nextEndCompare;
         ignitionSchedule4.Status = PENDING;
         ignitionSchedule4.schedulesSet = 1;
         ignitionSchedule4.hasNextSchedule = false;
       }
       else { IGN4_TIMER_DISABLE(); }
    }
  }

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__) //AVR chips use the ISR for this
ISR(TIMER1_COMPC_vect) //ignitionSchedule5
#elif defined (CORE_TEENSY) || defined(CORE_STM32)
static inline void ignitionSchedule5Interrupt() //Most ARM chips can simply call a function
#endif
  {
    if (ignitionSchedule5.Status == PENDING) //Check to see if this schedule is turn on
    {
      ignitionSchedule5.StartCallback();
      ignitionSchedule5.Status = RUNNING; //Set the status to be in progress (ie The start callback has been called, but not the end callback)
      ignitionSchedule5.startTime = micros();
      IGN5_COMPARE = IGN5_COUNTER + uS_TO_TIMER_COMPARE_SLOW(ignitionSchedule5.duration); //Doing this here prevents a potential overflow on restarts
    }
    else if (ignitionSchedule5.Status == RUNNING)
    {
       ignitionSchedule5.Status = OFF; //Turn off the schedule
       ignitionSchedule5.EndCallback();
       ignitionSchedule5.schedulesSet = 0;
       ignitionCount += 1; //Increment the igintion counter
       IGN5_TIMER_DISABLE();
    }
  }



#if defined(CORE_TEENSY)
void ftm0_isr(void)
{
  //Use separate variables for each test to ensure conversion to bool
  bool interrupt1 = (FTM0_C0SC & FTM_CSC_CHF);
  bool interrupt2 = (FTM0_C1SC & FTM_CSC_CHF);
  bool interrupt3 = (FTM0_C2SC & FTM_CSC_CHF);
  bool interrupt4 = (FTM0_C3SC & FTM_CSC_CHF);
  bool interrupt5 = (FTM0_C4SC & FTM_CSC_CHF);
  bool interrupt6 = (FTM0_C5SC & FTM_CSC_CHF);
  bool interrupt7 = (FTM0_C6SC & FTM_CSC_CHF);
  bool interrupt8 = (FTM0_C7SC & FTM_CSC_CHF);

  if(interrupt1) { FTM0_C0SC &= ~FTM_CSC_CHF; fuelSchedule1Interrupt(); }
  else if(interrupt2) { FTM0_C1SC &= ~FTM_CSC_CHF; fuelSchedule2Interrupt(); }
  else if(interrupt3) { FTM0_C2SC &= ~FTM_CSC_CHF; fuelSchedule3Interrupt(); }
  else if(interrupt4) { FTM0_C3SC &= ~FTM_CSC_CHF; fuelSchedule4Interrupt(); }
  else if(interrupt5) { FTM0_C4SC &= ~FTM_CSC_CHF; ignitionSchedule1Interrupt(); }
  else if(interrupt6) { FTM0_C5SC &= ~FTM_CSC_CHF; ignitionSchedule2Interrupt(); }
  else if(interrupt7) { FTM0_C6SC &= ~FTM_CSC_CHF; ignitionSchedule3Interrupt(); }
  else if(interrupt8) { FTM0_C7SC &= ~FTM_CSC_CHF; ignitionSchedule4Interrupt(); }

}
#endif

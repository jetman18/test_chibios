
#include "ch.h"
#include "hal.h"


int main(void) {

  halInit();
  chSysInit();
  palSetPadMode( GPIOB, 0U, PAL_MODE_OUTPUT_PUSHPULL );
  palSetPadMode( GPIOB, 14U, PAL_MODE_OUTPUT_PUSHPULL );
  palSetPadMode( GPIOB, 7U, PAL_MODE_OUTPUT_PUSHPULL );
  while (true) {

     chThdSleepMilliseconds(100);
     palClearPad(GPIOB, 0U);
     palClearPad(GPIOB, 14U);
     palClearPad(GPIOB, 7U);
     chThdSleepMilliseconds(100);
     palSetPad(GPIOB, 0U);
     palSetPad(GPIOB, 14U);
     palSetPad(GPIOB, 7U);
  }
}

/*
Based on ChibiOS/testhal/STM32F0xx/SPI,

PA2(TX) and PA3(RX) are routed to USART2 38400-8-N-1.
*/

#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "nrf24l01.h"

/*
 * Low speed SPI configuration (140.625kHz, CPHA=0, CPOL=0, MSb first).
 */
static const SPIConfig ls_spicfg = {
  NULL,
  GPIOB,
  12,
  SPI_CR1_BR_2 | SPI_CR1_BR_1,
  SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0
};

/*
 * This is a periodic thread that does absolutely nothing except flashing
 * a LED.
 */
static WORKING_AREA(blinker_wa, 128);
static msg_t blinker(void *arg) {

  (void)arg;
  chRegSetThreadName("blinker");
  while (TRUE) {
    palSetPad(GPIOC, GPIOC_LED3);
    chThdSleepMilliseconds(500);
    palClearPad(GPIOC, GPIOC_LED3);
    chThdSleepMilliseconds(500);
  }
}

void nrf_dump_regs(nrf_regs *r) {

	int i;
	int j;

//	cio_print("\n\r** START nRF2401 Register DUMP **\n\r");
	chprintf((BaseSequentialStream *) &SD2, "\n\r** START nRF2401 Register DUMP **\n\r");

	nrf_reg_buf buf;

	for(i = 0; i < r->count; i++) {

		nrf_read_reg(i, &buf);

		if(r->data[i].size == 0) continue;

//		cio_printf("%s: ", r->data[i].name);
		chprintf((BaseSequentialStream *) &SD2, "%s: ", r->data[i].name);

		for(j = 0; j < buf.size; j++) {
//			cio_printb(buf.data[j], 8);
//			cio_printf(" (%u) ", buf.data[j]);
			chprintf((BaseSequentialStream *) &SD2, " (%u) ", buf.data[j]);
		}

//		cio_print("\n\r - ");
		chprintf((BaseSequentialStream *) &SD2,"\n\r - ");

/*		for(j = 0; j < r->data[i].fields->count; j++) {
			cio_printf("%u[%u]:%s=%u ", j,
				r->data[i].fields->data[j].size,
				r->data[i].fields->data[j].name,
				nrf_get_reg_field(i, j, &buf));
		}*/

		chprintf((BaseSequentialStream *) &SD2,"\n\r - ");
	}
	chprintf((BaseSequentialStream *) &SD2,"** END **\n\r");
//	cio_print("** END nRF2401 Register DUMP **\n\r");
}

void nrf_configure_sb_rx(void) {

	// Set address for TX and receive on P0
 	nrf_reg_buf addr;

	addr.data[0] = 1;
	addr.data[1] = 2;
	addr.data[2] = 3;
	addr.data[3] = 4;
	addr.data[4] = 5;

	nrf_preset_sb(NRF_MODE_PRX, 40, 1, &addr);

	// Wait for radio to power up 
	chThdSleepMilliseconds(500);
}


/*
 * Application entry point.
 */
int main(void) {
  unsigned i;

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  /*
   * SPI2 I/O pins setup.
   */
  palSetPadMode(GPIOB, 13, PAL_MODE_ALTERNATE(0) |
                           PAL_STM32_OSPEED_HIGHEST);       /* New SCK.     */
  palSetPadMode(GPIOB, 14, PAL_MODE_ALTERNATE(0) |
                           PAL_STM32_OSPEED_HIGHEST);       /* New MISO.    */
  palSetPadMode(GPIOB, 15, PAL_MODE_ALTERNATE(0) |
                           PAL_STM32_OSPEED_HIGHEST);       /* New MOSI.    */
  palSetPad(GPIOB, 12);
  palSetPadMode(GPIOB, 12, PAL_MODE_OUTPUT_PUSHPULL |
                           PAL_STM32_OSPEED_HIGHEST);       /* New CS.      */

  sdStart( &SD2 , NULL );
  palSetPadMode(GPIOA, 2, PAL_MODE_ALTERNATE(1));      /* USART1 TX.       */
  palSetPadMode(GPIOA, 3, PAL_MODE_ALTERNATE(1));      /* USART1 RX.       */

  spiAcquireBus(&SPID2);              /* Acquire ownership of the bus.    */
//  palSetPad(GPIOC, GPIOC_LED4);       /* LED OFF.                         */
  spiStart(&SPID2, &ls_spicfg);       /* Setup transfer parameters.       */

//  nrf_init();
  nrf_configure_sb_rx();
  nrf_dump_regs(&nrf_reg_def);

  chThdCreateStatic(blinker_wa, sizeof(blinker_wa),
                    NORMALPRIO-1, blinker, NULL);
  static nrf_payload   p;

  int s;

  // set payload size according to the payload size configured for the RX channel
  p.size = 1;

  while (TRUE) {
    s = nrf_receive_blocking(&p);
//    cio_printf("Received payload: %x; bytes received: %u\n\r", p.data[0], s);
    chprintf((BaseSequentialStream *) &SD2, "Received payload: %x; bytes received: %u\n\r", p.data[0], s );
    chThdSleepMilliseconds(100);
 }

  return 0;
}

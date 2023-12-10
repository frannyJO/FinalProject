/*****************************************************************//**
 * @file main_sampler_test.cpp
 *
 * @brief Basic test of nexys4 ddr mmio cores
 *
 * @author p chu
 * @version v1.0: initial release
 *********************************************************************/

// #define _DEBUG
#include <ctime>
#include "chu_init.h"
#include "gpio_cores.h"
#include "xadc_core.h"
#include "sseg_core.h"
#include "spi_core.h"
#include "i2c_core.h"
#include "ps2_core.h"
#include "ddfs_core.h"
#include "adsr_core.h"

/**
 * blink once per second for 5 times.
 * provide a sanity check for timer (based on SYS_CLK_FREQ)
 * @param led_p pointer to led instance
 */
void timer_check(GpoCore *led_p) {
   int i;

   for (i = 0; i < 5; i++) {
      led_p->write(0xffff);
      sleep_ms(500);
      led_p->write(0x0000);
      sleep_ms(500);
      debug("timer check - (loop #)/now: ", i, now_ms());
   }
}

/**
 * check individual led
 * @param led_p pointer to led instance
 * @param n number of led
 */
void led_check(GpoCore *led_p, int n) {
   int i;

   for (i = 0; i < n; i++) {
      led_p->write(1, i);
      sleep_ms(100);
      led_p->write(0, i);
      sleep_ms(100);
   }
}

/**
 * leds flash according to switch positions.
 * @param led_p pointer to led instance
 * @param sw_p pointer to switch instance
 */
void sw_check(GpoCore *led_p, GpiCore *sw_p) {
   int i, s;

   s = sw_p->read();
   for (i = 0; i < 30; i++) {
      led_p->write(s);
      sleep_ms(50);
      led_p->write(0);
      sleep_ms(50);
      uart.disp(s);
      uart.disp("\n");
   }
}

/**
 * uart transmits test line.
 * @note uart instance is declared as global variable in chu_io_basic.h
 */
void uart_check() {
   static int loop = 0;

   uart.disp("uart test #");
   uart.disp(loop);
   uart.disp("\n\r");
   loop++;
}

/**
 * read FPGA internal voltage temperature
 * @param adc_p pointer to xadc instance
 */

void adc_check(XadcCore *adc_p, GpoCore *led_p) {
   double reading;
   int n, i;
   uint16_t raw;

   for (i = 0; i < 5; i++) {
      // display 12-bit channel 0 reading in LED
      raw = adc_p->read_raw(0);
      raw = raw >> 4;
      led_p->write(raw);
      // display on-chip sensor and 4 channels in console
      uart.disp("FPGA vcc/temp: ");
      reading = adc_p->read_fpga_vcc();
      uart.disp(reading, 3);
      uart.disp(" / ");
      reading = adc_p->read_fpga_temp();
      uart.disp(reading, 3);
      uart.disp("\n\r");
      for (n = 0; n < 4; n++) {
         uart.disp("analog channel/voltage: ");
         uart.disp(n);
         uart.disp(" / ");
         reading = adc_p->read_adc_in(n);
         uart.disp(reading, 3);
         uart.disp("\n\r");
      } // end for
      sleep_ms(200);
   }
}

/**
 * tri-color led dims gradually
 * @param led_p pointer to led instance
 * @param sw_p pointer to switch instance
 */

void pwm_3color_led_check(PwmCore *pwm_p) {
   int i, n;
   double bright, duty;
   const double P20 = 1.2589;  // P20=100^(1/20); i.e., P20^20=100

   pwm_p->set_freq(50);
   for (n = 0; n < 3; n++) {
      bright = 1.0;
      for (i = 0; i < 20; i++) {
         bright = bright * P20;
         duty = bright / 100.0;
         pwm_p->set_duty(duty, n);
         pwm_p->set_duty(duty, n + 3);
         sleep_ms(100);
      }
      sleep_ms(300);
      pwm_p->set_duty(0.0, n);
      pwm_p->set_duty(0.0, n + 3);
   }
}

/**
 * Test debounced buttons
 *   - count transitions of normal and debounced button
 * @param db_p pointer to debouceCore instance
 */

void debounce_check(DebounceCore *db_p, GpoCore *led_p) {
   long start_time;
   int btn_old, db_old, btn_new, db_new;
   int b = 0;
   int d = 0;
   uint32_t ptn;

   start_time = now_ms();
   btn_old = db_p->read();
   db_old = db_p->read_db();
   do {
      btn_new = db_p->read();
      db_new = db_p->read_db();
      if (btn_old != btn_new) {
         b = b + 1;
         btn_old = btn_new;
      }
      if (db_old != db_new) {
         d = d + 1;
         db_old = db_new;
      }
      ptn = d & 0x0000000f;
      ptn = ptn | (b & 0x0000000f) << 4;
      led_p->write(ptn);
   } while ((now_ms() - start_time) < 5000);
}

/*
 * Test pattern in 7-segment LEDs
 * @param sseg_p pointer to 7-seg LED instance
 */

void sseg_check(SsegCore *sseg_p) {
   int i, n;
   uint8_t dp;

   //turn off led
   for (i = 0; i < 8; i++) {
      sseg_p->write_1ptn(0xff, i);
   }
   //turn off all decimal points
   sseg_p->set_dp(0x00);

   // display 0x0 to 0xf in 4 epochs
   // upper 4  digits mirror the lower 4
   for (n = 0; n < 4; n++) {
      for (i = 0; i < 4; i++) {
         sseg_p->write_1ptn(sseg_p->h2s(i + n * 4), 3 - i);
         sseg_p->write_1ptn(sseg_p->h2s(i + n * 4), 7 - i);
         sleep_ms(300);
      } // for i
   }  // for n
      // shift a decimal point 4 times
   for (i = 0; i < 4; i++) {
      bit_set(dp, 3 - i);
      sseg_p->set_dp(1 << (3 - i));
      sleep_ms(300);
   }
   //turn off led
   for (i = 0; i < 8; i++) {
      sseg_p->write_1ptn(0xff, i);
   }
   //turn off all decimal points
   sseg_p->set_dp(0x00);


}


/**
 * Test adxl362 accelerometer using SPI
*/

void gsensor_check(SpiCore *spi_p, GpoCore *led_p) {
   const uint8_t RD_CMD = 0x0b;
   const uint8_t PART_ID_REG = 0x02;
   const uint8_t DATA_REG = 0x08;
   const float raw_max = 127.0 / 2.0;  //128 max 8-bit reading for +/-2g

   int8_t xraw, yraw, zraw;
   float x, y, z;
   int id;

   spi_p->set_freq(400000);
   spi_p->set_mode(0, 0);
   // check part id
   spi_p->assert_ss(0);    // activate
   spi_p->transfer(RD_CMD);  // for read operation
   spi_p->transfer(PART_ID_REG);  // part id address
   id = (int) spi_p->transfer(0x00);
   spi_p->deassert_ss(0);
   uart.disp("read ADXL362 id (should be 0xf2): ");
   uart.disp(id, 16);
   uart.disp("\n\r");
   // read 8-bit x/y/z g values once
   spi_p->assert_ss(0);    // activate
   spi_p->transfer(RD_CMD);  // for read operation
   spi_p->transfer(DATA_REG);  //
   xraw = spi_p->transfer(0x00);
   yraw = spi_p->transfer(0x00);
   zraw = spi_p->transfer(0x00);
   spi_p->deassert_ss(0);
   x = (float) xraw / raw_max;
   y = (float) yraw / raw_max;
   z = (float) zraw / raw_max;
   uart.disp("x/y/z axis g values: ");
   uart.disp(x, 3);
   uart.disp(" / ");
   uart.disp(y, 3);
   uart.disp(" / ");
   uart.disp(z, 3);
   uart.disp("\n\r");
   //0 deg
   if(x<1.1 && x>1){
  led_p->write(1,6);
  led_p->write(0,7);
  led_p->write(0,8);
  led_p->write(0,9);
   }
   if(x<0.1 && x>0){
  led_p->write(0,6);
  led_p->write(1,7);
  led_p->write(0,8);
  led_p->write(0,9);
   }
   if(x<-1 && x>-1.1){
  led_p->write(0,6);
  led_p->write(0,7);
  led_p->write(1,8);
  led_p->write(0,9);
   }
   if(x<0 && x>-0.1){
  led_p->write(0,6);
  led_p->write(0,7);
  led_p->write(0,8);
  led_p->write(1,9);
   }

}

/*
 * read temperature from adt7420
 * @param adt7420_p pointer to adt7420 instance
 */
//float tempC;

void sseg_temp(float temp, SsegCore *sseg_p){
uint8_t point = 0x10;

int temp_val = static_cast<int>(temp * 1000.0);

int num_array[5];

num_array[0] = (temp_val / 10000) % 10;
num_array[1] = (temp_val / 1000) % 10;
num_array[2] = (temp_val / 100) % 10;
num_array[3] = (temp_val / 10) % 10;
num_array[4] = (temp_val / 1) % 10;

sseg_p->set_dp(0x00);
sseg_p->write_1ptn(sseg_p->h2s(12), 0);
sseg_p->set_dp(point);

for(int i = 0; i < 5; i++){
sseg_p->write_1ptn(sseg_p->h2s(num_array[i]), 5 - i);
/*if(i == 4){
sseg_p->write_1ptn(sseg_p->h2s(12), 0);
}
else {
sseg_p->write_1ptn(sseg_p->h2s(num_array[i]), 5 - i);
}*/
}
}


void adt7420_check(I2cCore *adt7420_p, GpoCore *led_p, SsegCore *sseg_p) {
   const uint8_t DEV_ADDR = 0x4b;
   uint8_t wbytes[2], bytes[2];
   //int ack;
   uint16_t tmp;
   float tmpC;

   // read adt7420 id register to verify device existence
   // ack = adt7420_p->read_dev_reg_byte(DEV_ADDR, 0x0b, &id);

   wbytes[0] = 0x0b;
   adt7420_p->write_transaction(DEV_ADDR, wbytes, 1, 1);
   adt7420_p->read_transaction(DEV_ADDR, bytes, 1, 0);
   uart.disp("read ADT7420 id (should be 0xcb): ");
   uart.disp(bytes[0], 16);
   uart.disp("\n\r");
   //debug("ADT check ack/id: ", ack, bytes[0]);
   // read 2 bytes
   //ack = adt7420_p->read_dev_reg_bytes(DEV_ADDR, 0x0, bytes, 2);
   wbytes[0] = 0x00;
   adt7420_p->write_transaction(DEV_ADDR, wbytes, 1, 1);
   adt7420_p->read_transaction(DEV_ADDR, bytes, 2, 0);

   // conversion
   tmp = (uint16_t) bytes[0];
   tmp = (tmp << 8) + (uint16_t) bytes[1];
   if (tmp & 0x8000) {
      tmp = tmp >> 3;
      tmpC = (float) ((int) tmp - 8192) / 16;
   } else {
      tmp = tmp >> 3;
      tmpC = (float) tmp / 16;
   }

   sseg_temp(tmpC, sseg_p);

   uart.disp("temperature (C): ");
   //tempC = tmpC;
   uart.disp(tmpC);
   uart.disp("\n\r");
   led_p->write(tmp);
   sleep_ms(1000);
   led_p->write(0);

}

void testingnumbers(SsegCore *sseg_p, int num_array[8]){
/*uint8_t point = 0x10;

int temp_val = static_cast<int>(temp * 1000.0);

int num_array[8];

num_array[0] = 0;
num_array[1] = 0;
num_array[2] = (temp_val / 100) % 10;
num_array[3] = (temp_val / 10000) % 10;
num_array[4] = 0;
num_array[5] = 0;
num_array[6] = (temp_val / 10) % 10;
num_array[7] = 0;

*/

sseg_p->set_dp(0x00);
/*
for (int i = 0; i < 3; i++){
sseg_p->write_1ptn(sseg_p->h2s(0), i);
}

for (int i = 4; i < 7; i++){
  sseg_p->write_1ptn(sseg_p->h2s(0), i);
}

//sseg_p->set_dp(point);

int points = 0;

for(int i = 0; i < 8; i++){
*/

sseg_p->write_1ptn(sseg_p->h2s(num_array[0]), 0);
sseg_p->write_1ptn(sseg_p->h2s(num_array[1]), 1);
sseg_p->write_1ptn(sseg_p->h2s(num_array[2]), 2);
sseg_p->write_1ptn(sseg_p->h2s(num_array[3]), 3);
sseg_p->write_1ptn(sseg_p->h2s(num_array[4]), 4);
sseg_p->write_1ptn(sseg_p->h2s(num_array[5]), 5);
sseg_p->write_1ptn(sseg_p->h2s(num_array[6]), 6);
sseg_p->write_1ptn(sseg_p->h2s(num_array[7]), 7);

//points += 10;
//sleep_ms(1000);

/*if(i == 4){++++
sseg_p->write_1ptn(sseg_p->h2s(12), 0);
}
else {
sseg_p->write_1ptn(sseg_p->h2s(num_array[i]), 5 - i);
}*/
//}
}


void ps2_check(Ps2Core *ps2_p, GpoCore *led_p, SsegCore *sseg_p, GpiCore *sw_p) {
   int id;
   char ch;
   int player1point=0; //keyboard
   int player2point=0; //nexys4ddr
   int player1point_prev=0;
   int player2point_prev=0;
   //unsigned long last;

   int flipTimeSet = 850;
   int currTime;
   int win = 0;
   int checked = 0;

   sseg_p->set_dp(0x00);

   int num_array[8];

    num_array[0] = 0;
    num_array[1] = 0;
    num_array[2] = 0;
    num_array[3] = 0;
    num_array[4] = 0;
    num_array[5] = 0;
    num_array[6] = 0;
    num_array[7] = 0;

    uart.disp("Ready to begin game!\n\r");
    led_check(led_p, 16);

    testingnumbers(sseg_p, num_array);

   //uart.disp("\n\rPS2 device (1-keyboard / 2-mouse): ");
   id = ps2_p->init();
   //uart.disp(id);
   //uart.disp("\n\r");

   do {
         if (ps2_p->get_kb_ch(&ch)) {
            player1point_prev = player1point;
            player2point_prev = player2point;

            if((ch == 'a') && win == 0){ //A keyboard
               led_p->write(1,0);

               currTime=now_ms();
               while ( ((now_ms() - currTime) < flipTimeSet)){
                  int s=sw_p->read();
                  if ((s==1) && (checked == 0)){
                     player2point=player2point+100;
                     checked = 1;
                     break;
                  }

               }

               led_p->write(0,0);

               if (checked == 0 ){
                  player1point=player1point+100;
               }

               checked = 0;

               if((player1point > 900) || (player2point > 900)){
                  win = 1;
               }

            }
            if(ch == 'b' && win == 0){ //B keyboard
               led_p->write(1,1);

               currTime=now_ms();
               while (((now_ms() - currTime) < flipTimeSet) ){
                  int s=sw_p->read();
                  if ((s==2) && (checked == 0)){
                     player2point=player2point+100;
                     checked = 1;
                     break;
                  }
               }

               led_p->write(0,1);

               if (checked == 0 ){
                  player1point=player1point+100;
               }

               checked = 0;

               if((player1point > 900) || (player2point > 900)){
                  win = 1;
               }

            }
            if(ch == 'c' && win == 0){ //C keyboard
               led_p->write(1,2);

               currTime=now_ms();
               while (((now_ms() - currTime) < flipTimeSet)){
                  int s=sw_p->read();
                  if ((s==4) && (checked == 0)){
                     player2point=player2point+100;
                     checked = 1;
                     break;
                  }
               }

               led_p->write(0,2);

               if (checked == 0 ){
                  player1point=player1point+100;
               }

               checked = 0;

               if((player1point > 900) || (player2point > 900)){
                  win = 1;
               }

            }
            if(ch == 'd' && win == 0){ //D keyboard
               led_p->write(1,3);

               currTime=now_ms();
               while (((now_ms() - currTime) < flipTimeSet)){
                  int s=sw_p->read();
                  if ((s==8) && (checked == 0)){
                     player2point=player2point+100;
                     checked = 1;
                     break;
                  }
               }

               led_p->write(0,3);

               if (checked == 0 ){
                  player1point=player1point+100;
               }

               checked = 0;

               if((player1point > 900) || (player2point > 900)){
                  win = 1;
               }

            }
            if(ch == 'e' && win == 0){ //E keyboard
               led_p->write(1,4);

               currTime=now_ms();
               while (((now_ms() - currTime) < flipTimeSet)){
                  int s=sw_p->read();
                  if ((s==16) && (checked == 0)){
                     player2point=player2point+100;
                     checked = 1;
                     break;
                  }
               }

               led_p->write(0,4);

               if (checked == 0 ){
                  player1point=player1point+100;
               }

               checked = 0;

               if((player1point > 900) || (player2point > 900)){
                  win = 1;
               }

            }
            if(ch == 'f' && win == 0){ //f keyboard
               led_p->write(1,5);

               currTime=now_ms();
               while (((now_ms() - currTime) < flipTimeSet)){
                  int s=sw_p->read();
                  if ((s==32) && (checked == 0)){
                     player2point=player2point+100;
                     checked = 1;
                     break;
                  }
               }

               led_p->write(0,5);

               if (checked == 0 ){
                  player1point=player1point+100;
               }

               checked = 0;

               if((player1point > 900) || (player2point > 900)){
                  win = 1;
               }

            }
            if(ch == 'g' && win == 0){ //g keyboard
               led_p->write(1,6);

               currTime=now_ms();
               while (((now_ms() - currTime) < flipTimeSet)){
                  int s=sw_p->read();
                  if ((s==64) && (checked == 0)){
                     player2point=player2point+100;
                     checked = 1;
                     break;
                  }
               }

               led_p->write(0,6);

               if (checked == 0 ){
                  player1point=player1point+100;
               }

               checked = 0;

               if((player1point > 900) || (player2point > 900)){
                  win = 1;
               }

            }
            if(ch == 'h' && win == 0){ //h keyboard
               led_p->write(1,7);

               currTime=now_ms();
               while (((now_ms() - currTime) < flipTimeSet)){
                  int s=sw_p->read();
                  if ((s==128) && (checked == 0)){
                     player2point=player2point+100;
                     checked = 1;
                  break;
                  }
               }

               led_p->write(0,7);

               if (checked == 0 ){
                  player1point=player1point+100;
               }

               checked = 0;

               if((player1point > 900) || (player2point > 900)){
                  win = 1;
               }

            }
            if(ch == 'i' && win == 0){ //h keyboard
               led_p->write(1,8);

               currTime=now_ms();
               while (((now_ms() - currTime) < flipTimeSet)){
                  int s=sw_p->read();
                  if ((s==256) && (checked == 0)){
                     player2point=player2point+100;
                     checked = 1;
                     break;
                  }
               }

               led_p->write(0,8);

               if (checked == 0 ){
                  player1point=player1point+100;
               }

               checked = 0;

               if((player1point > 900) || (player2point > 900)){
                  win = 1;
               }

            }
            if(ch == 'j' && win == 0){ //i keyboard
               led_p->write(1,9);

               currTime=now_ms();
               while (((now_ms() - currTime) < flipTimeSet)){
                  int s=sw_p->read();
                  if ((s==512) && (checked == 0)){
                     player2point=player2point+100;
                     checked = 1;
                     break;
                  }
               }

               led_p->write(0,9);

               if (checked == 0 ){
                  player1point=player1point+100;
               }

               checked = 0;

               if((player1point > 900) || (player2point > 900)){
                  win = 1;
               }

            }
            if(ch == 'k' && win == 0){ //j keyboard
               led_p->write(1,10);

               currTime=now_ms();
               while (((now_ms() - currTime) < flipTimeSet)){
                  int s=sw_p->read();
                  if ((s==1024) && (checked == 0)){
                     player2point=player2point+100;
                     checked = 1;
                     break;
                  }
               }

               led_p->write(0,10);

               if (checked == 0 ){
                  player1point=player1point+100;
               }

               checked = 0;

               if((player1point > 900) || (player2point > 900)){
                  win = 1;
               }

            }
            if(ch == 'l' && win == 0){ //k keyboard
               led_p->write(1,11);

               currTime=now_ms();
               while (((now_ms() - currTime) < flipTimeSet)){
                  int s=sw_p->read();
                  if ((s==2048) && (checked == 0)){
                     player2point=player2point+100;
                     checked = 1;
                     break;
                  }
               }

               led_p->write(0,11);

               if (checked == 0 ){
                  player1point=player1point+100;
               }

               checked = 0;

               if((player1point > 900) || (player2point > 900)){
                  win = 1;
               }

            }
            if(ch == 'm' && win == 0){ //l keyboard
               led_p->write(1,12);

               currTime=now_ms();
               while (((now_ms() - currTime) < flipTimeSet)){
                  int s=sw_p->read();
                  if ((s==4096) && (checked == 0)){
                     player2point=player2point+100;
                     checked = 1;
                     break;
                  }
               }

               led_p->write(0,12);

               if (checked == 0 ){
                  player1point=player1point+100;
               }

               checked = 0;

               if((player1point > 900) || (player2point > 900)){
                  win = 1;
               }

            }
            if(ch == 'n' && win == 0){ //m keyboard
               led_p->write(1,13);

               currTime=now_ms();
               while (((now_ms() - currTime) < flipTimeSet)){
                  int s=sw_p->read();
                  if ((s==8192) && (checked == 0)){
                     player2point=player2point+100;
                     checked = 1;
                     break;
                  }
               }

               led_p->write(0,13);

               if (checked == 0 ){
                  player1point=player1point+100;
               }

               checked = 0;

               if((player1point > 900) || (player2point > 900)){
                  win = 1;
               }

            }
            if(ch == 'o' && win == 0){ //n keyboard
               led_p->write(1,14);

               currTime=now_ms();
               while (((now_ms() - currTime) < flipTimeSet)){
                  int s=sw_p->read();
                  if ((s==16384) && (checked == 0)){
                     player2point=player2point+100;
                     checked = 1;
                     break;
                  }
               }

               led_p->write(0,14);

               if (checked == 0 ){
                  player1point=player1point+100;
               }

               checked = 0;

               if((player1point > 900) || (player2point > 900)){
                  win = 1;
               }

            }
            if(ch == 'p' && win == 0){ //n keyboard
               led_p->write(1,15);

               currTime=now_ms();
               while (((now_ms() - currTime) < flipTimeSet)){
                  int s=sw_p->read();
                  if ((s==32768) && (checked == 0)){
                     player2point=player2point+100;
                     checked = 1;
                     break;
                  }
               }

               led_p->write(0,15);

               if (checked == 0 ){
                  player1point=player1point+100;
               }

               checked = 0;

               if((player1point > 900) || (player2point > 900)){
                  win = 1;
               }

            }

            if(player1point_prev != player1point){
               num_array[6] = (player1point / 100) % 10;
               num_array[7] = player1point / 1000;
            }

            if(player2point_prev != player2point){
               num_array[2] = (player2point / 100) % 10;
               num_array[3] = player2point / 1000;
            }

            testingnumbers(sseg_p, num_array);

            uart.disp(" ");
            //last = now_ms();
         } // end get_kb_ch()
        // end id==2
   } while (win == 0);

   if (player1point > player2point){
      num_array[7] = 1;
   }
   else {
      num_array[3] = 1;
   }
   testingnumbers(sseg_p, num_array);
   sleep_ms(2000);

   uart.disp("Game over\n\r");
   led_check(led_p, 16);
   uart.disp(" ");
   //uart.disp("\n\rExit PS2 test \n\r");

}


/**
 * play primary notes with ddfs
 * @param ddfs_p pointer to ddfs core
 * @note: music tempo is defined as beats of quarter-note per minute.
 *        60 bpm is 1 sec per quarter note
 * @note "click" sound due to abrupt stop of a note
 *
 */
void ddfs_check(DdfsCore *ddfs_p, GpoCore *led_p) {
   int i, j;
   float env;

   //vol = (float)sw.read_pin()/(float)(1<<16),
   ddfs_p->set_env_source(0);  // select envelop source
   ddfs_p->set_env(0.0);   // set volume
   sleep_ms(500);
   ddfs_p->set_env(1.0);   // set volume
   ddfs_p->set_carrier_freq(262);
   sleep_ms(2000);
   ddfs_p->set_env(0.0);   // set volume
   sleep_ms(2000);
   // volume control (attenuation)
   ddfs_p->set_env(0.0);   // set volume
   env = 1.0;
   for (i = 0; i < 1000; i++) {
      ddfs_p->set_env(env);
      sleep_ms(10);
      env = env / 1.0109; //1.0109**1024=2**16
   }
   // frequency modulation 635-912 800 - 2000 siren sound
   ddfs_p->set_env(1.0);   // set volume
   ddfs_p->set_carrier_freq(635);
   for (i = 0; i < 5; i++) {               // 10 cycles
      for (j = 0; j < 30; j++) {           // sweep 30 steps
         ddfs_p->set_offset_freq(j * 10);  // 10 Hz increment
         sleep_ms(25);
      } // end j loop
   } // end i loop
   ddfs_p->set_offset_freq(0);
   ddfs_p->set_env(0.0);   // set volume
   sleep_ms(1000);
}

/**
 * play primary notes with ddfs
 * @param adsr_p pointer to adsr core
 * @param ddfs_p pointer to ddfs core
 * @note: music tempo is defined as beats of quarter-note per minute.
 *        60 bpm is 1 sec per quarter note
 *
 */
void adsr_check(AdsrCore *adsr_p, GpoCore *led_p, GpiCore *sw_p) {
   const int melody[] = { 0, 2, 4, 5, 7, 9, 11 };
   int i, oct;

   adsr_p->init();
   // no adsr envelop and  play one octave
   adsr_p->bypass();
   for (i = 0; i < 7; i++) {
      led_p->write(bit(i));
      adsr_p->play_note(melody[i], 3, 500);
      sleep_ms(500);
   }
   adsr_p->abort();
   sleep_ms(1000);
   // set and enable adsr envelop
   // play 4 octaves
   adsr_p->select_env(sw_p->read());
   for (oct = 3; oct < 6; oct++) {
      for (i = 0; i < 7; i++) {
         led_p->write(bit(i));
         adsr_p->play_note(melody[i], oct, 500);
         sleep_ms(500);
      }
   }
   led_p->write(0);
   // test duration
   sleep_ms(1000);
   for (i = 0; i < 4; i++) {
      adsr_p->play_note(0, 4, 500 * i);
      sleep_ms(500 * i + 1000);
   }
}

GpoCore led(get_slot_addr(BRIDGE_BASE, S2_LED));
GpiCore sw(get_slot_addr(BRIDGE_BASE, S3_SW));
XadcCore adc(get_slot_addr(BRIDGE_BASE, S5_XDAC));
PwmCore pwm(get_slot_addr(BRIDGE_BASE, S6_PWM));
DebounceCore btn(get_slot_addr(BRIDGE_BASE, S7_BTN));
SsegCore sseg(get_slot_addr(BRIDGE_BASE, S8_SSEG));
SpiCore spi(get_slot_addr(BRIDGE_BASE, S9_SPI));
I2cCore adt7420(get_slot_addr(BRIDGE_BASE, S10_I2C));
Ps2Core ps2(get_slot_addr(BRIDGE_BASE, S11_PS2));
DdfsCore ddfs(get_slot_addr(BRIDGE_BASE, S12_DDFS));
AdsrCore adsr(get_slot_addr(BRIDGE_BASE, S13_ADSR), &ddfs);


int main() {

   while (1) {
      
      ps2_check(&ps2, &led, &sseg, &sw);

   }

} //main

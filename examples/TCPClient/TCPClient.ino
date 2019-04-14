/*
 *  SPRESENSE_WiFi.ino - GainSpan WiFi Module Control Program
 *  Copyright 2019 Norikazu Goto
 *
 *  This work is free software; you can redistribute it and/or modify it under the terms 
 *  of the GNU Lesser General Public License as published by the Free Software Foundation; 
 *  either version 2.1 of the License, or (at your option) any later version.
 *
 *  This work is distributed in the hope that it will be useful, but without any warranty; 
 *  without even the implied warranty of merchantability or fitness for a particular 
 *  purpose. See the GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License along with 
 *  this work; if not, write to the Free Software Foundation, 
 *  Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include <GS2200Hal.h>
#include "AppFunc.h"


#define  CONSOLE_BAUDRATE  115200


// the setup function runs once when you press reset or power the board
void setup() {

	/* initialize digital pin of LEDs as an output. */
	pinMode(LED0, OUTPUT);
	pinMode(LED1, OUTPUT);
	pinMode(LED2, OUTPUT);
	pinMode(LED3, OUTPUT);
	
	digitalWrite( LED0, LOW );   // turn the LED off (LOW is the voltage level)
	Serial.begin( CONSOLE_BAUDRATE ); // talk to PC

	/* Initialize SPI access of GS2200 */
	Init_GS2200_SPI();

	digitalWrite( LED0, HIGH ); // turn on LED

}

// the loop function runs over and over again forever
void loop() {

	App_TCPClient_Test(); 	/* Never return */

}

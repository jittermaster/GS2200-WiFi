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


#define  CONSOLE_BAUDRATE  115200

char rdata;
SPI_RESP_STATUS_E s;
int spi;
uint8_t spidata[SPI_MAX_RECEIVED_DATA];
uint16_t rxDataLen;

void setup() {

	/* initialize digital pin LED0 as an output. */
	pinMode(LED0, OUTPUT);
	digitalWrite( LED0, LOW );   // turn the LED off (LOW is the voltage level)
	Serial.begin( CONSOLE_BAUDRATE ); // talk to PC

	ConsoleLog("***********************************");
	ConsoleLog("*** UART to/from SPI Conversion ***");
	ConsoleLog("***********************************");
	ConsoleLog("");

	/* Initialize SPI access of GS2200 */
	Init_GS2200_SPI_type(iS110B_TypeC);

	digitalWrite( LED0, HIGH ); // turn on LED
}


void loop() {

	/* UART -> SPI */
	if( Serial.available() ){
		rdata = (char)Serial.read();
		/* Send 1 byte to GS2200 */
		s = WiFi_Write( &rdata, 1 );
		if( s != SPI_RESP_STATUS_OK )
			ConsoleLog("SPI ERROR: ESC command is not accepted");
	}

	/* SPI -> UART */
	if( Get_GPIO37Status() ){
		s = WiFi_Read( spidata, &rxDataLen );

		if( s == SPI_RESP_STATUS_TIMEOUT ){
			ConsoleLog("SPI: Timeout");
		}
		else if( s == SPI_RESP_STATUS_ERROR ){
			ConsoleLog("GS2200 responds Zero data length");
		}
		else{
			spi = 0;
			while( rxDataLen-- ){
				Serial.write( spidata[spi++] );
			}
		}
		
	}

}

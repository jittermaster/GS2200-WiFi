/*
 *  GS2200Hal.h - Hardware Adaptation Layer for GS2200 SPI Interface
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

#ifndef _GS_HAL_H_
#define _GS_HAL_H_

#define MAX_RECEIVED_DATA      1500
#define SPI_MAX_RECEIVED_DATA  1500

#define SPI_PORT           SPI5      /* SPRESENSE Main Board SPI */ 
#define SPI_FREQ           4000000   /* SPI Clock Frequency */
#define SPI_MODE           SPI_MODE1 /* SPI_MODE0, SPI_MODE1, SPI_MODE3 */
#define SPI_DATA_TRANSFER  SPI_PORT.transfer

#define SPI_TIMEOUT        20000     /* wait for GPIO37 for this period */ 


typedef enum {
	SPI_RESP_STATUS_OK = 0,
	SPI_RESP_STATUS_ERROR,
	SPI_RESP_STATUS_TIMEOUT
} SPI_RESP_STATUS_E;

typedef enum {
	iS110B_TypeA = 0,
	iS110B_TypeB,
	iS110B_TypeC,
} ModuleType;


/*-------------------------------------------------------------------------*
 * Function ProtoTypes:
 *-------------------------------------------------------------------------*/
uint32_t msDelta(uint32_t start);
void Init_GS2200_SPI(void);
void Init_GS2200_SPI_type(ModuleType type);

int Get_GPIO37Status(void);

SPI_RESP_STATUS_E WiFi_Write(const void *txData, uint16_t dataLength);
SPI_RESP_STATUS_E WiFi_Read(uint8_t *rxData, uint16_t *rxDataLen);

void WiFi_InitESCBuffer(void);
void WiFi_StoreESCBuffer(uint8_t rxData);
bool Check_CID(uint8_t cid);


void ConsoleLog(const char *pStr);
void ConsoleByteSend(uint8_t data);
void ConsolePrintf( const char *fmt, ...);


#endif	/* _GS_HAL_H_ */


/*-------------------------------------------------------------------------*
 * End of File:  GS2200Hal.h
 *-------------------------------------------------------------------------*/


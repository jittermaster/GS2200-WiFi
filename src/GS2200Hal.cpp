/*
 *  GS2200Hal.cpp - Hardware Adaptation Layer for GS2200 SPI Interface
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

#include <Arduino.h>
#include <SPI.h>
#include <stdarg.h>
#include "GS2200Hal.h"


/*-------------------------------------------------------------------------*
 * Constants:
 *-------------------------------------------------------------------------*/
#define WRITE_REQUEST          0x01       
#define READ_REQUEST           0x02
#define DATA_FROM_MCU          0x03
#define READ_RESPONSE_OK       0x12
#define READ_RESPONSE_NOK      0x14
#define WRITE_RESPONSE_OK      0x11
#define WRITE_RESPONSE_NOK     0x13 
#define DATA_TO_MCU            0x15 
#define HEADER_LENGTH          8
#define HALF_HEADER_LENGTH     4
#define PENDING_DATA_TO_MCU    0x01

#define SPI_IDLE_CHAR          0xF5

//#define GS_DEBUG

static int GPIO37 = 27;

/*-------------------------------------------------------------------------*
 * Globals:
 *-------------------------------------------------------------------------*/
uint8_t ESCBuffer[MAX_RECEIVED_DATA + 1];
uint32_t ESCBufferCnt = 0;

uint8_t pendingDataFlag = 0;


/*---------------------------------------------------------------------------*
 * msDelta
 *---------------------------------------------------------------------------*
 * Function: Calculate the difference between the current milliseconds and a given start timer value.
 * Inputs  : uint32_t start -- Timer value at start of delta.
 * Outputs : uint32_t -- Millisecond of Delta.
 *---------------------------------------------------------------------------*/
uint32_t msDelta(uint32_t start)
{
	uint32_t now;

	now = millis();
	if( now < start )
		return start - now;
	else
		return now - start;
}


/*---------------------------------------------------------------------------*
 * Init_GS2200_SPI with type
 *---------------------------------------------------------------------------*
 * Function: Initialize GS2200 SPI
 *---------------------------------------------------------------------------*/
void Init_GS2200_SPI_type(ModuleType type)
{
	switch(type) {
	case iS110B_TypeC:
		puts("Is Your module iS110B_TypeC ?");
		GPIO37 = 20;
		break;
	default:
		puts("Is Your module iS110B_TypeA or iS110B_TypeB ?");
		GPIO37 = 27;
		break;
	}

	/* Start the SPI library for GS2200 control*/
	SPI_PORT.begin();
	/* Set GPIO37 monitor pin */
	pinMode( GPIO37, INPUT ); 
	/* Configure the SPI port */
	SPI_PORT.beginTransaction( SPISettings( SPI_FREQ, MSBFIRST, SPI_MODE ) );

	ConsoleLog( "GS2200 is ready to go." );
}

/*---------------------------------------------------------------------------*
 * Init_GS2200_SPI
 *---------------------------------------------------------------------------*
 * Function: Initialize GS2200 SPI for compatibility
 *---------------------------------------------------------------------------*/
void Init_GS2200_SPI(void)
{
	/* Start the SPI library for GS2200 control*/
	Init_GS2200_SPI_type(iS110B_TypeA);

}

/*-----------------------------------------------------------------------------*
 * Get_GPIO37Status
 *-----------------------------------------------------------------------------*
 * Function : Check the status of GPIO37
 *          : If GS2200 has data to transmit, it will set the GPIO37 high.
 * Outputs  : High(1) or Low(0)
 *-----------------------------------------------------------------------------*/
int Get_GPIO37Status(void)
{
	return digitalRead( GPIO37 );
}



/*-------------------------------------------------------------------------*
 *                                                                         *
 * GS2200 SPI Command and Response                                         *
 *                                                                         *
 *-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*
 * Common Functions
 *-------------------------------------------------------------------------*/

static uint8_t SpiChecksum(uint8_t* headerBuff, uint8_t length)
{
	uint32_t cpt;
	uint8_t* pt;
	uint32_t ckecksum;
	
	/* Compute the checksum */
	pt=(uint8_t*)headerBuff;
	ckecksum = 0x00000000;
	for (cpt=length; cpt != 0; cpt--)
	{
		ckecksum += *pt; /* Compute the checksum byte by byte */
		pt++;
	}
	
	/* Complement the checksum */
	ckecksum ^= 0xFFFFFFFF;
	
	return ckecksum;
}


static void SpiMakeHeader(uint8_t *buff, uint16_t dataLength, uint8_t request )
{
	buff[0] =  0xA5;
	
	buff[1] =  request;
	buff[2] =  0x00;
	buff[3] =  0x00;
	buff[4] =  0x00;
	buff[5] = (uint8_t)dataLength ;
	buff[6] = (uint8_t)((dataLength >>8) & 0x00FF);
	
	buff[7] = SpiChecksum(buff+1,6);
}


static void Read_HeaderResponse(uint8_t* buff)
{
	for(int i=0; i<HEADER_LENGTH; i++)
		*buff++ = SPI_DATA_TRANSFER(SPI_IDLE_CHAR);
}


static void Write_Header(uint8_t* TxBuffer)
{
	for(int i=0; i<HEADER_LENGTH; i++)
		SPI_DATA_TRANSFER(*TxBuffer++);
}


static void Write_Header_Half(uint8_t* TxBuffer)
{
	for(int i=0; i<HALF_HEADER_LENGTH; i++)
		SPI_DATA_TRANSFER(*TxBuffer++);
}


static void Write_Data(uint8_t* TxBuffer, uint16_t dataLen)
{
	for(int i=0; i<dataLen; i++)
		SPI_DATA_TRANSFER(*TxBuffer++);
}
   


/*---------------------------------------------------------------------------*
 * WiFi_Write
 *---------------------------------------------------------------------------*
 * Description: Write a string of characters to GS2200
 * Inputs     : const uint8_t *txData -- string of bytes
 *              uint32_t dataLength -- Number of bytes to transfer
 * Outputs    : SPI_RESP_STATUS_E
 *---------------------------------------------------------------------------*/
SPI_RESP_STATUS_E WiFi_Write(const void *txData, uint16_t dataLength)
{
	const uint8_t *tx = (uint8_t *)txData;
	uint8_t spiHeaderBuff[8] = {0}, hiResponse[8]={0};
	uint16_t recvLen;
	uint32_t start = millis();

	
	// Make HI Header
	SpiMakeHeader(spiHeaderBuff, dataLength, WRITE_REQUEST);
	// send first half of WRITE_REQUEST to GS2200	
	Write_Header_Half(spiHeaderBuff);
	// wait for at least 3.2usec
	delayMicroseconds( 4 );
	// send last half of WRITE_REQUEST to GS2200
	Write_Header_Half(spiHeaderBuff+HALF_HEADER_LENGTH); 
	// Wait for the response from GS2200
	while(!Get_GPIO37Status()){
		if( msDelta(start) > SPI_TIMEOUT )
			return SPI_RESP_STATUS_TIMEOUT;
	}

	// Read the response from GS2200
	Read_HeaderResponse(hiResponse);
	// Get the data length GS2200 can receive. This should be the same as requested
	recvLen = hiResponse[6]<<8 | hiResponse[5];     
	//check response for write_request and also the check the size of data GS2000 can receive
	if((WRITE_RESPONSE_OK == hiResponse[1]) && (dataLength == recvLen) )
	{	 
		SpiMakeHeader(spiHeaderBuff, dataLength, DATA_FROM_MCU);  // make the data class(0x03) hearder		
		Write_Header(spiHeaderBuff);                              // send the header
		Write_Data((uint8_t*)tx, dataLength);                     // send the data to GS

		return SPI_RESP_STATUS_OK;
	}
	else
	{
#ifdef GS_DEBUG
		ConsoleLog( "SPI WRITE: Incorrect Response" );
		ConsolePrintf( "hiResponse[1]:0x%x\r\n", hiResponse[1] );
		ConsolePrintf( "hiResponse[5]:0x%x\r\n", hiResponse[5] );
		ConsolePrintf( "hiResponse[6]:0x%x\r\n", hiResponse[6] );
		ConsolePrintf( "recvLen:%d\r\n", recvLen );
#endif
		return SPI_RESP_STATUS_ERROR;
	}
}


/*-------------------------------------------------------------------------*
 * Read Functions
 *-------------------------------------------------------------------------*/

static uint16_t Read_DataLen(void)
{
	uint8_t spiHeaderBuff[8] = {0}, hiResponse[8] = {0};
	uint16_t respLength = 0, tempData = 0;
	uint32_t start = millis();
	
	// Make HI Header
	SpiMakeHeader(spiHeaderBuff, SPI_MAX_RECEIVED_DATA , READ_REQUEST);
	// Send HI Header
	Write_Header(spiHeaderBuff);
	// Wait for GPIO37=HIGH
	while(!Get_GPIO37Status()){
		if( msDelta(start) > SPI_TIMEOUT )
			return 0; // 0 should not happen, so this indocates ERROR
	}

	// Read header response from GS2200
	Read_HeaderResponse(hiResponse);
	
	if(hiResponse[1] == READ_RESPONSE_OK)
	{
		respLength |= hiResponse[5];
		tempData = hiResponse[6];									
		respLength |= (tempData << 8);
	}

	if(hiResponse[4] == PENDING_DATA_TO_MCU)
	{
		pendingDataFlag = 1;
	}
	else
	{
		pendingDataFlag = 0;
	}

	return respLength;
}


static void Read_Data(uint8_t* RxBuffer, uint16_t dataLen)
{
	for(int i=0; i<dataLen; i++)
		*RxBuffer++ = SPI_DATA_TRANSFER(SPI_IDLE_CHAR);
}


/*---------------------------------------------------------------------------*
 * WiFi_Read
 *---------------------------------------------------------------------------*
 * Description: Read a string of characters from GS2200.
 * Inputs     : uint8_t *rxData -- Pointer to a place to store a string of bytes
 *            : uint16_t *rxDataLen -- Pointer to a place to store the data length
 * Outputs    : SPI_RESP_STATUS_E
 *---------------------------------------------------------------------------*/
SPI_RESP_STATUS_E WiFi_Read(uint8_t *rxData, uint16_t *rxDataLen)
{
	uint32_t start = millis();
	uint8_t spiHeader[8]= {0};

	
	// Wait for GPIO37 = HIGH
	while(!Get_GPIO37Status()){
		if( msDelta(start) > SPI_TIMEOUT )
			return SPI_RESP_STATUS_TIMEOUT;
	}

	// Get how many bytes should be read
	*rxDataLen = Read_DataLen();
	if( *rxDataLen==0 ){ 	// data length must not be 0
#ifdef GS_DEBUG
		ConsoleLog( "SPI READ: Zero data Error" );
#endif    
		return SPI_RESP_STATUS_ERROR;
	}
	// Read Data Header
	Read_HeaderResponse(spiHeader); 
	
	// Read data from GS2200
	Read_Data( rxData, *rxDataLen );

	return SPI_RESP_STATUS_OK;

}

SPI_RESP_STATUS_E WiFi_Read_Timeout(uint8_t *rxData, uint16_t *rxDataLen, uint32_t timeout)
{
	uint32_t start = millis();
	uint8_t spiHeader[8]= {0};

	
	// Wait for GPIO37 = HIGH
	while(!Get_GPIO37Status()){
		if( msDelta(start) > timeout )
			return SPI_RESP_STATUS_TIMEOUT;
	}

	// Get how many bytes should be read
	*rxDataLen = Read_DataLen();
	if( *rxDataLen==0 ) 	// data length must not be 0
		return SPI_RESP_STATUS_ERROR;
		
	// Read Data Header
	Read_HeaderResponse(spiHeader);       
	
	// Read data from GS2200
	Read_Data( rxData, *rxDataLen );

	return SPI_RESP_STATUS_OK;

}



/*---------------------------------------------------------------------------*
 * WiFi_InitESCBuffer
 *---------------------------------------------------------------------------*
 * Description: Initialize ESCBuffer before receiving ESC sequence data
 *---------------------------------------------------------------------------*/
void WiFi_InitESCBuffer(void)
{
	ESCBufferCnt = 0;
	ESCBuffer[0] = '\0';
}

/*---------------------------------------------------------------------------*
 * WiFi_StoreESCBuffer
 *---------------------------------------------------------------------------*
 * Description: Store received data by ESC sequence
 *              First byte should be CID
 * Inputs: uint8_t rxData -- Byte received
 *---------------------------------------------------------------------------*/
void WiFi_StoreESCBuffer(uint8_t rxData)
{
	if( ESCBufferCnt < MAX_RECEIVED_DATA) {
		ESCBuffer[ESCBufferCnt++] = rxData;
		ESCBuffer[ESCBufferCnt] = '\0';
	}
}


/*---------------------------------------------------------------------------*
 * Check_CID
 *---------------------------------------------------------------------------*
 * Description: First byte of ESCBuffer should be CID
 *              Check if expected CID
 * Inputs: uint8_t cid -- Byte received
 *---------------------------------------------------------------------------*/
bool Check_CID(uint8_t cid)
{
	if( ESCBuffer[0] == cid )
		return true;

	return false;
}

	
/*------------------------------------------------------------------*
 *
 * Console Functions
 *
 *------------------------------------------------------------------*/
void ConsoleLog(const char *pStr)
{
	Serial.println( pStr );
}


void ConsoleByteSend(uint8_t data)
{
	Serial.write( &data, 1 );
}


#define PRINTFBUFFER 2048
void ConsolePrintf( const char *fmt, ...)
{
        char buf[PRINTFBUFFER]; // resulting string limited to 128 chars
        va_list args;

        va_start( args, fmt );
        vsnprintf( buf, PRINTFBUFFER, fmt, args);
        va_end( args );
        Serial.print( buf );
}

/*-------------------------------------------------------------------------*
 * End of File:  GS2200Hal.cpp
 *-------------------------------------------------------------------------*/


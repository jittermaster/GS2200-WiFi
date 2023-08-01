/*
 *  HTTP Library for GS2200
 *  Copyright 2023 Spresense Users
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "HttpGs2200.h"

//#define ENABLE_HTTP_DEBUG

#ifdef ENABLE_HTTP_DEBUG
#define HTTP_DEBUG(...)    \
	{\
	printf("DEBUG:   %s L#%d ", __func__, __LINE__);  \
	printf(__VA_ARGS__); \
	printf("\n"); \
	}
#else
#define HTTP_DEBUG(...)
#endif /* HTTP_DEBUG */
extern uint8_t ESCBuffer[];

bool HttpGs2200::begin(HTTPGS2200_HostParams* params)
{
  HTTP_DEBUG("Initialize HTTP");

  mCid = ATCMD_INVALID_CID;

  mData.host = params->host;
  mData.port = params->port;
  
  HTTP_DEBUG("Server: %s, Port: %s", mData.host, mData.port);

  return true;
}

#ifndef SUBCORE
bool HttpGs2200::set_cert(char* name, char* time_string, int format, int location, File *fp)
{
	bool result = true;

	AtCmd_TCERTADD(name, format, location, *fp); 

    AtCmd_SETTIME(time_string);
	AtCmd_SSLCONF(100);
	AtCmd_LOGLVL(2);

  return result;
}
#endif

bool HttpGs2200::set_cert(char* name, char* time_string, int format, int location, uint8_t* ptr, int size )
{
	bool result = true;

	AtCmd_TCERTADD(name, format, location, ptr, size); 

    AtCmd_SETTIME(time_string);
	AtCmd_SSLCONF(100);
	AtCmd_LOGLVL(2);

  return result;
}

void HttpGs2200::config(ATCMD_HTTP_HEADER_E param, const char *val)
{
	ATCMD_RESP_E resp = ATCMD_RESP_UNMATCH;

	while (1) {
		resp = AtCmd_HTTPCONF(param, val);

		if (ATCMD_RESP_OK == resp) {
			break;
		} else {
			printf("config error! err resp = %d, retry...\n", resp);
			continue;
		}
	}

  return;
}

bool HttpGs2200::connect()
{
	ATCMD_RESP_E resp;
	ATCMD_NetworkStatus networkStatus;

	resp = ATCMD_RESP_UNMATCH;
	
	do {
		WiFi_InitESCBuffer();
		if (0 == strncmp("443", mData.port, 3)) {
			resp = AtCmd_HTTPSOPEN(&mCid, mData.host, mData.port, "TLS_CA");
		} else {
			resp = AtCmd_HTTPOPEN(&mCid, mData.host, mData.port);
		}
		
		if (resp != ATCMD_RESP_OK) {
			ConsoleLog( "No Connect!" );
			delay(2000);
			continue;
		}
		
		if (mCid == ATCMD_INVALID_CID) {
			ConsoleLog( "No CID!" );
			delay(2000);
			continue;
		}
	} while (ATCMD_RESP_OK != resp);

	do {
		resp = AtCmd_NSTAT(&networkStatus);
	} while (ATCMD_RESP_OK != resp);

	HTTP_DEBUG( "Connected" );
	ConsolePrintf("IP: %d.%d.%d.%d\r\n", 
	              networkStatus.addr.ipv4[0], networkStatus.addr.ipv4[1], networkStatus.addr.ipv4[2], networkStatus.addr.ipv4[3]);
	return true;
}

bool HttpGs2200::send(ATCMD_HTTP_METHOD_E type, uint8_t timeout, const char *page, const char *msg, uint32_t size)
{
	ATCMD_RESP_E resp = ATCMD_RESP_UNMATCH;
	bool result = false;
	int retry = 10;
	while (1) {
		resp = AtCmd_HTTPSEND(mCid, type, timeout, page, msg, size);
		if (ATCMD_RESP_OK == resp || ATCMD_RESP_BULK_DATA_RX == resp) {
			result = true;
			break;
		} else {
			if(retry > 0){
				retry--;
				sleep(1);
				continue;
			}else{
				result = false;
				break;
			}
		}
	}
	return result;
}

int HttpGs2200::receive(uint8_t* data, int length)
{
	int receive_size = -1;
	
	memset(data, 0, length);
	WiFi_InitESCBuffer();

	while (1) {
		if (mWifi->available()) {
			receive_size = mWifi->read(mCid, data, length);
			if (0 < receive_size) {
				break;
			} else {
				continue;
				printf("theHttpGs2200.receive err.\n");
			}
		}
	}

	return receive_size;
}

bool HttpGs2200::receive(uint64_t timeout)
{
	ATCMD_RESP_E resp = ATCMD_RESP_UNMATCH;
	bool result = false;
	WiFi_InitESCBuffer();
	uint64_t start = millis();
	while (1) {
		if (mWifi->available()) {
			resp = AtCmd_RecvResponse();
			if (ATCMD_RESP_BULK_DATA_RX == resp) {
				result = true;
				break;
			} else {
				result = false;
				break;
			}
		}
		if (msDelta(start)> timeout ) {// Timeout
			result = false;
			printf("msDelta(start)> %lld Timeout.\n", timeout);
			break;
		}
	}
	
	return result;
}

void HttpGs2200::read_data(uint8_t* data, int length)
{
	memset(data, 0, length);
	memcpy(data, (ESCBuffer + 1), length);

	return;
}

bool HttpGs2200::post(const char* url_path, const char* body) {
	bool result = false;

	HTTP_DEBUG("POST Start");
	result = connect();

	WiFi_InitESCBuffer();

	HTTP_DEBUG("Socket Open");
	result = send(HTTP_METHOD_POST, 10, url_path, body, strlen(body));

	return result;
}

bool HttpGs2200::get(const char* url_path) {
	bool result = false;

	HTTP_DEBUG("GET Start");
	result = connect();
	
	WiFi_InitESCBuffer();

	HTTP_DEBUG("Open Socket");
	result = send(HTTP_METHOD_GET, 10, url_path, "", 0);

	return result;
}

bool HttpGs2200::end()
{
	ATCMD_RESP_E resp = ATCMD_RESP_UNMATCH;
	bool result = false;
	while (1) {
		resp = AtCmd_HTTPCLOSE(mCid);

		if (ATCMD_RESP_OK == resp || ATCMD_RESP_INVALID_CID == resp ) {
			result = true;
			break;
		} else {
			result = false;
			continue;
		}
	}
	HTTP_DEBUG("Socket Closed");
	return result;
}

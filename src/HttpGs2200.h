/*
 *  MQTT Library for GS2200
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

#ifndef HTTP_GS2200_h
#define HTTP_GS2200_h

#include <Arduino.h>
#include <GS2200Hal.h>
#include <GS2200AtCmd.h>
#include <TelitWiFi.h>


typedef struct {
  char *host;
  char *port;
} HTTPGS2200_HostParams;

class HttpGs2200
{
public:

  HttpGs2200(TelitWiFi* wifi) : mWifi(wifi) {}
  ~HttpGs2200(){}

  bool begin(HTTPGS2200_HostParams* params);
#ifndef SUBCORE
  bool set_cert(char* name, char* time_string, int format, int location, File *fp);
#endif
  bool set_cert(char* name, char* time_string, int format, int location, uint8_t* ptr, int size );
  bool connect();
  void config(ATCMD_HTTP_HEADER_E param, const char *val);
  bool send(ATCMD_HTTP_METHOD_E type, uint8_t timeout, const char *page, const char *msg, uint32_t size);
  int receive(uint8_t* data, int length);
  bool receive(uint64_t timeout = 10000);
  void read_data(uint8_t* data, int length);
  bool end();
  bool post(const char* url_path, const char* body);
  bool get(const char* url_path);

private:

  TelitWiFi* mWifi;
  char mCid;

  HTTPGS2200_HostParams mData;

};

#endif // HTTP_GS2200_h

/*
 *  Ambient Library for GS2200
 *  Copyright 2020 Spresense Users
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

#ifndef AMBIENT_GS2200_h
#define AMBIENT_GS2200_h

#include <Arduino.h>
#include <GS2200Hal.h>
#include <GS2200AtCmd.h>
#include <TelitWiFi.h>

#define AMBIENT_WRITEKEY_SIZE 18
#define AMBIENT_MAX_RETRY 5
#define AMBIENT_DATA_SIZE 24
#define AMBIENT_NUM_PARAMS 11
#define AMBIENT_TIMEOUT 3000 // milliseconds

class AmbientGs2200
{
public:

  AmbientGs2200(){}
  ~AmbientGs2200(){}

  bool begin(TelitWiFi* wifi, uint16_t channelId, const String& writeKey);
  bool connect(char* host, int port);
  bool set(int field, const char * data);
  bool set(int field, double data);
  bool set(int field, int data);
  bool clear(int field);
  bool send();
  void end(){ }

private:

  TelitWiFi* mWifi;
  char mCid;

  uint16_t mChannelId;
  String mWriteKey;

  struct {
    int set;
    String item;
  } mData[AMBIENT_NUM_PARAMS];
};

#endif // AMBIENT_GS2200_h

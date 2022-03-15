/*
 *  LTE-M Ambient Library for GS2200
 *  Copyright 2020 
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

#include "AmbientGs2200.h"

#define AMBIENT_DEBUG

#ifdef AMBIENT_DEBUG
#define DBG(...) { Serial.print(__VA_ARGS__); }
#define ERR(...) { Serial.print(__VA_ARGS__); }
#else
#define DBG(...)
#define ERR(...)
#endif /* AMBIENT_DBG */

const char  path[] = "/api/v2/channels/";
const char* ambient_keys[] = {"\"d1\":\"", "\"d2\":\"", "\"d3\":\"", "\"d4\":\"", "\"d5\":\"", "\"d6\":\"", "\"d7\":\"", "\"d8\":\"", "\"lat\":\"", "\"lng\":\"", "\"created\":\""};

const char  server[] = "54.65.206.59";
const char  port[] = "80";

uint8_t* ESCBuffer_p;
extern uint8_t ESCBuffer[];
extern uint32_t ESCBufferCnt;

bool AmbientGs2200::begin(TelitWiFi* wifi, uint16_t id, const String& writeKey)
{
#ifdef AMBIENT_DEBUG
    Serial.println("Initialize Ambient");
#endif

  mWifi = wifi;
  mCid = ATCMD_INVALID_CID;

  if (sizeof(writeKey) > AMBIENT_WRITEKEY_SIZE) {
      ERR("writeKey length > AMBIENT_WRITEKEY_SIZE");
      return false;
  }

  this->mChannelId = id;
  this->mWriteKey = writeKey;

  for (int i = 0; i < AMBIENT_NUM_PARAMS; i++) {
      this->mData[i].set = false;
  }

#ifdef AMBIENT_DEBUG
  Serial.println("Server: " + (String)server + ", Port: " + port);
  Serial.println("mCannelID: " + (String)this->mChannelId + ", Write Key: " + (String)this->mWriteKey);
#endif

  return true;
}

bool AmbientGs2200::set(int field, const char* data)
{
  --field;
  if (field < 0 || field >= AMBIENT_NUM_PARAMS) {
      return false;
  }

  if (strlen(data) > AMBIENT_DATA_SIZE) {
      return false;
  }
  this->mData[field].set = true;
  this->mData[field].item = data;

  Serial.println(String(field) + ", " + String(this->mData[field].item));

  return true;
}

bool AmbientGs2200::set(int field, double data)
{
  return set(field,String(data).c_str());
}

bool AmbientGs2200::set(int field, int data)
{
  return set(field, String(data).c_str());
}

bool AmbientGs2200::clear(int field)
{
  --field;
  if (field < 0 || field >= AMBIENT_NUM_PARAMS) {
      return false;
  }
  this->mData[field].set = false;

  return true;
}

bool AmbientGs2200::send()
{
  // Prepare for the next chunck of incoming data

  mCid = mWifi->connect( server, port );

  /* Retrieve the file with the specified URL. */
  String post = "{\"writeKey\":\"" + mWriteKey + "\",";

  for (int i = 0; i < AMBIENT_NUM_PARAMS; ++i) {
    if (mData[i].set) {
      post += String(ambient_keys[i]);
      post += mData[i].item;
      post += "\",";
    }
  }
  post.setCharAt(post.length()-1,'}'); // to overwrite the last conmma 

  String ctype = String("");

  String fullpath = path + String(this->mChannelId) + "/data";


  String data = "POST " + fullpath + " HTTP/1.1\r\nHOST: " + server + "\r\n";
  data = data + "Content-Length: " + String( post.length()) + "\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n";
//  data = data + "Content-Length: " + String( post.length()) + "\r\nContent-Type: application/json\r\n\r\n";
  
  Serial.println(data);

  uint16_t length = data.length();
  printf("cid=%c\tlengh=%d\n%s\n",mCid,length,data.c_str());
  
  if( ATCMD_RESP_OK != AtCmd_SendBulkData( mCid, data.c_str(), length )){
    // Data is not sent, we need to re-send the data
    ConsoleLog( "Send Error.");
    delay(1);
  }

  delay(100);

  data = post;
  length = data.length();

  if( ATCMD_RESP_OK != AtCmd_SendBulkData( mCid, data.c_str(), length )){
    // Data is not sent, we need to re-send the data
    ConsoleLog( "Send Error.");
    delay(1);
  }

  sleep(2);

  mWifi->stop(mCid);
  mCid = ATCMD_INVALID_CID;

  return true;
}

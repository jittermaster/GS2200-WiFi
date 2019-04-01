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
#include <AtCmd.h>
#include "AppFunc.h"
#include "config.h"
#include <Audio.h>
#include <LowPower.h>

AudioClass *theAudio;

#define  CONSOLE_BAUDRATE  115200

char server_cid = 0;

char TCP_Data[]=RADIO_SITE;

uint8_t* ESCBuffer_p;
extern uint8_t ESCBuffer[];
extern uint32_t ESCBufferCnt;

/**
 * @brief Audio attention callback
 *
 * When audio internal error occurc, this function will be called back.
 */

static void audio_attention_cb(const ErrorAttentionParam *atprm)
{
  puts("Attention!");
  
//  if (atprm->error_code > AS_ATTENTION_CODE_WARNING)
  if (atprm->error_att_sub_code == AS_ATTENTION_SUB_CODE_SIMPLE_FIFO_UNDERFLOW)
    {
      LowPower.reboot();
   }
}

// the setup function runs once when you press reset or power the board
void setup() {

      LowPower.begin();

	/* initialize digital pin LED_BUILTIN as an output. */
	pinMode(LED0, OUTPUT);
	digitalWrite( LED0, LOW );   // turn the LED off (LOW is the voltage level)
	Serial.begin( CONSOLE_BAUDRATE ); // talk to PC

	/* Initialize SPI access of GS2200 */
	Init_GS2200_SPI();

	digitalWrite( LED0, HIGH ); // turn on LED

  puts(RADIO_SITE);

  // start audio system
  theAudio = AudioClass::getInstance();
  theAudio->begin(audio_attention_cb);
  puts("initialization Audio Library");
  theAudio->setPlayerMode(AS_SETPLAYER_OUTPUTDEVICE_SPHP, AS_SP_DRV_MODE_4DRIVER);
  err_t err = theAudio->initPlayer(AudioClass::Player0, AS_CODECTYPE_MP3, "/mnt/sd0/BIN", AS_SAMPLINGRATE_AUTO, AS_CHANNEL_STEREO);
  puts("initialization Player Library");

  theAudio->setVolume(-160);
  
	/* WiFi Module Initialize */
	App_InitModule();
	App_ConnectAP();

  do{
    server_cid = App_ConnectWeb();
  }while(server_cid == ATCMD_INVALID_CID);

}

static int es_reader(int argc, FAR char *argv[])
{

  static int state = 0; // 0 is "header search", 1 is "read payload"  
  static int fetch_size = 0; /*�ŏ��ɓǂݍ��މ񐔁B�{���o�C�g���ǂ��B*/
  ATCMD_RESP_E resp;

  (void)argc;
  (void)argv;

while(1){
  /*Wait for data.*/
//  puts("loop");
  while( !Get_GPIO37Status() ){
    usleep(10000);
  }

  while( Get_GPIO37Status() ){
    resp = AtCmd_RecvResponse();

    if( ATCMD_RESP_BULK_DATA_RX == resp ){
      if( Check_CID( server_cid ) ){
        
        ESCBuffer_p = ESCBuffer+1;
        ESCBufferCnt = ESCBufferCnt -1;
        
        if(state== 0){
          while(ESCBufferCnt > 3){
            if( ('\r' == *(ESCBuffer_p)) &&
              ('\n' == *(ESCBuffer_p+1)) &&
              ('\r' == *(ESCBuffer_p+2)) &&
              ('\n' == *(ESCBuffer_p+3)) ){
                state=1;
                ESCBuffer_p = ESCBuffer_p+3;
                ESCBufferCnt = ESCBufferCnt-3;
                ConsoleLog( "\nfind\n");
                break;
              }
            ESCBuffer_p++;
            ESCBufferCnt--;
          }
        }

        if(state== 1){
          theAudio->writeFrames(AudioClass::Player0, ESCBuffer_p, ESCBufferCnt);
        }
      }
      
      WiFi_InitESCBuffer();
    }
    
    static int cnt =0;
    if(cnt==50){
      puts("start");
      theAudio->startPlayer(AudioClass::Player0);
      cnt++;
    }else{
      cnt++;
    }
  }
}

}

// the loop function runs over and over again forever
void loop() {

//	ConsoleLog( "Start to send TCP Data");
//	Prepare for the next chunck of incoming data
	WiFi_InitESCBuffer();

//	Start the infinite loop to send the data

	if( ATCMD_RESP_OK != AtCmd_SendBulkData( server_cid, TCP_Data, strlen(TCP_Data) ) ){
		// Data is not sent, we need to re-send the data
		ConsoleLog( "Send Error.");
		delay(1);
	}

  puts(RADIO_NAME);
  task_create("es_reader", 155, 1024, es_reader, NULL);

  while(1){
    sleep(100);
  }
}

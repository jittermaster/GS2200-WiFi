#coding:utf-8

from time import sleep
import paho.mqtt.client as mqtt
import threading
import time

HOST = "test.mosquitto.org"
PORT = 1883
CLI_ID = "Telit_Device_sub"
TOPIC = "Telit/property"
MESSAGE = "IoT Data"

PUBLISH_NUMBER = 1
SLEEP_TIME = 1


def on_connect(client, userdata, flags, respons_code):
    print('Connected status = {0}'.format(respons_code))
    client.subscribe(TOPIC)
    
def on_message(client, userdata, msg):
    print(msg.topic+" "+str(msg.payload))

    
def publish_many_times(client, topic, message, number, time, print_flag=True):

    for i in range(number):
        client.publish(topic, message)
        if print_flag == True:
            print (topic + ' ' + message)
        sleep(time)


def timerCallBack(iTimeSec,isRepeated):
    print("timerCallBack!")
    publish_many_times(client,TOPIC, MESSAGE, PUBLISH_NUMBER, SLEEP_TIME)
    
    if isRepeated == True :
        threading.Timer(iTimeSec,timerCallBack,[iTimeSec,isRepeated]).start()

        
def startTimer(iTimeSec,isRepeated):
    threading.Timer(iTimeSec,timerCallBack,[iTimeSec,isRepeated]).start()


    
if __name__ == '__main__':
    client = mqtt.Client( CLI_ID )
    client.on_connect = on_connect
    client.on_message = on_message

    print "publish start " + str(type(client))

    client.connect( HOST )
    #startTimer(3,True)
    
    client.loop_forever()

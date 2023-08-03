/*
  RFSniffer

  Usage: ./RFSniffer [<pulseLength>]
  [] = optional

  Hacked from http://code.google.com/p/rc-switch/
  by @justy to provide a handy RF code sniffer
*/

#include "./rc-switch/RCSwitch.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <MQTTClient.h>     
     
RCSwitch mySwitch;
MQTTClient client;
 
#define ADDRESS     "tcp://192.168.1.150:1885"
#define CLIENTID    "ExampleClientPub"
#define TOPIC       "sensors/doorbell"
#define QOS         1
#define TIMEOUT     10000L
#define PAYLOAD     "hello"

void mqttConnect(){
 
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;

    MQTTClient_create(&client, ADDRESS, CLIENTID,
        MQTTCLIENT_PERSISTENCE_NONE, NULL);
/*    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;*/

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(-1);
    }
}

int main(int argc, char *argv[]) {
     int rc;
     mqttConnect();


     printf("MQTT connected \n");
  
     // This pin is not the first pin on the RPi GPIO header!
     // Consult https://projects.drogon.net/raspberry-pi/wiringpi/pins/
     // for more information.
     int PIN = 25;
     
     if(wiringPiSetup() == -1) {
       printf("wiringPiSetup failed, exiting...");
       return 0;
     }

     int pulseLength = 0;
     if (argv[1] != NULL) pulseLength = atoi(argv[1]);

     mySwitch = RCSwitch();
     mySwitch.setProtocol(11);
     if (pulseLength != 0) mySwitch.setPulseLength(pulseLength);
     mySwitch.enableReceive(PIN);  // Receiver on interrupt 0 => that is pin #2
     
    
     while(1) {
  
      if (mySwitch.available()) {
    
        int value = mySwitch.getReceivedValue();
    
        if (value == 0) {
          printf("Unknown encoding\n");
        } else if (value == 11291937 || value == 8641916) {
          MQTTClient_message pubmsg = MQTTClient_message_initializer;
          MQTTClient_deliveryToken token;
  	char *payload = "{\"doorbell\":\"front\"}";
 	   pubmsg.payload = (void*)payload;
	    pubmsg.payloadlen = strlen(payload);
	    pubmsg.qos = QOS;
	    pubmsg.retained = 0;
	    MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);
	    printf("Waiting for up to %d seconds for publication of %s\n"
	            "on topic %s for client with ClientID: %s\n",
	            (int)(TIMEOUT/1000), PAYLOAD, TOPIC, CLIENTID);
	    rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
	    printf("Message with delivery token %d delivered\n", token);

	}
	else {    
   
          printf("Received %i\n", mySwitch.getReceivedValue() );
        }
    
        fflush(stdout);
        mySwitch.resetAvailable();
      }
      usleep(100); 
  
  }

  exit(0);


}


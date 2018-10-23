#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(7, 8); // CE, CSN
const byte ADDRESSES[][6] = {"REPLY", "MAIN0"};
const long HOPPING_INTERVAL = 5000;
const long MESSAGE_INTERVAL = 5; 
const int BASE_CHANNEL = 76;
const int CHANNEL_TO_CHECK = 12;            //12,37,62
const char MESSAGE_HEADER[] = "MSH";
const char HANDSHAKE_HEADER[] = "Hsh";
const int ACK = 9999;
const int RETRY_MAX = 4;
const long RTO = 2000;
const int PACKAGE_NUM = 10000;
//long previous = 0;
boolean connection_state = 0;
boolean button_state = 0;
boolean reply_waiting = 0;
boolean finish = 0;
int hash = 0;
int channel = 0;
int hopping_step = 0;
int message_count = 0;
int message_count_old=0;
int message=0;
unsigned long present_time;
unsigned long start_hopping_time;
unsigned long previous_exchanging_time=0;

void setup() {
  Serial.begin(9600);
  radio.begin();
  radio.openWritingPipe(ADDRESSES[0]);
  radio.openReadingPipe(1, ADDRESSES[1]);
  radio.setPALevel(RF24_PA_MIN);
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  //-- state checking before every loop --
  present_time = millis();

  //-- answer when receiving seed --
  if (connection_state == 0) {
    radio.setChannel(BASE_CHANNEL);
    handShake();
  }
  //-- receiving messages --
  else {
    //Serial.println("start hopping time "+start_hopping_time);
    hoppingChannel();
    if (present_time < start_hopping_time + PACKAGE_NUM*MESSAGE_INTERVAL*1.2) {
      receiveMessage();
      if (message != 0) message_count++;
      if (message_count != message_count_old) {
        if (message_count%10 == 0) Serial.println(String(message_count) +"  messages received");
        message_count_old = message_count;
      }

      /*
      if (present_time - previous_exchanging_time > MESSAGE_INTERVAL) {
        receiveMessage();
        if (message != 0) {
          message_count++;
        }
        previous_exchanging_time = present_time;
      }
      */
    }
    else {
      if (finish == 0) {
        Serial.println("Received "+String(message_count)+" messages; Success rate = "+String(100*message_count/PACKAGE_NUM)+"%");
        finish = 1;
      }
    }
  }
}

void handShake(){
  if (present_time - previous_exchanging_time > MESSAGE_INTERVAL) {
    //-- need better handshake: receive then send simultaneously. leave like this for now --
    if (reply_waiting == 0) {
      receiveMessage();
      if (message != 0) {
        //message.remove(0,3);
        hash = message;
        Serial.println("Seed received = "+String(hash));
        reply_waiting =1;
        start_hopping_time = present_time+2*RTO;
      }
    }
    else if (present_time < start_hopping_time - RTO) {
      message = ACK;
      sendMessage();
    }
    else {
      reply_waiting = 0;
      connection_state = 1;
    }
    previous_exchanging_time = present_time;
  }
}

void sendMessage(){
  radio.stopListening();
  radio.write(&message, sizeof(message));
  //digitalWrite(LED_BUILTIN, HIGH);
  //delay(100);
  //digitalWrite(LED_BUILTIN, LOW);
  //delay(5);
}

void receiveMessage(){
  //String message = "";
  message = 0;
  radio.startListening();
  if (radio.available()) {
    radio.read(&message, sizeof(message));
    //if (message != 0) Serial.println("receive "+String(message));
  }
  //return message;
  //while(!radio.available());
  //radio.read(&Init, sizeof(Init));
  //if (init==1) {
    //radio.stopListening();
  //  digitalWrite(LED_BUILTIN, HIGH);
  //}
  //delay(5);
}

void hoppingChannel(){
    //radio.stopListening();
    if (present_time - hopping_step*HOPPING_INTERVAL > start_hopping_time) {
      hopping_step++;
      //-- changing channel using hash --
      channel = (channel + hash)%(125-BASE_CHANNEL);
      radio.setChannel(BASE_CHANNEL+channel);
      Serial.println("Hopping to channel "+String(radio.getChannel()));
    }
}

/*
  radio.startListening();
  if (radio.available()) {
    radio.read(&buttonState, sizeof(buttonState));
  }
      if (buttonState == 1) {
      if (digitalRead(LED_BUILTIN)==HIGH) digitalWrite(LED_BUILTIN, LOW);
      else digitalWrite(LED_BUILTIN, HIGH);
    }
    delay(5);
*/
/*
  radio.stopListening();
  hash += 1;
  if (hash % 2 == 0) digitalWrite(LED_BUILTIN, HIGH);
  else digitalWrite(LED_BUILTIN, LOW);
  radio.write(&hash, sizeof(hash));
  delay(1000);

      radio.startListening();
    //Serial.println(hash);
    unsigned long Current = millis();
    if (Current - Previous > Interval) {
      Previous = Current;
      //if (digitalRead(LED_BUILTIN)==HIGH) digitalWrite(LED_BUILTIN, LOW);
      //else digitalWrite(LED_BUILTIN, HIGH);
      //change channel using hash
      channel = (channel + hash)%49;
      radio.setChannel(76+channel);
      Serial.println(radio.getChannel());
    }
    //while(!radio.available());
    if (radio.available()) radio.read(&buttonState, sizeof(buttonState));
    //radio.read(&buttonState, sizeof(buttonState));
    if (buttonState == 1) {
      if (digitalRead(LED_BUILTIN)==HIGH) digitalWrite(LED_BUILTIN, LOW);
      else digitalWrite(LED_BUILTIN, HIGH);
    }
    delay(5);
  }
*/
/*
    if (Init == 0) {
    radio.startListening();
    if (radio.available()) {
      radio.read(&hash, sizeof(hash));
      if (hash != 0) {
        Init = 1;
        digitalWrite(LED_BUILTIN, HIGH);
      }
    }
    delay(5);

    radio.stopListening();
    radio.write(&Init, sizeof(Init));
    delay(5);

  */

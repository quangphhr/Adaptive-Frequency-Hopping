#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#define button 4

RF24 radio(7, 8); // CE, CSN
const byte ADDRESSES[][6] = {"REPLY", "MAIN0"};
const long HOPPING_INTERVAL = 5000;
const long MESSAGE_INTERVAL = 5; 
const int BASE_CHANNEL = 76;
const int CHANNEL_TO_CHECK = 12;            //12,37,62
const char MESSAGE_HEADER[] = "MSH";
const char HANDSHAKE_HEADER[] = "HSH";
const int ACK = 9999;
const int RETRY_MAX = 4;
const long RTO = 2000;
const int PACKAGE_NUM = 10000;
//long previous = 0;
boolean connection_state = 0;
boolean button_state = 0;
boolean reply_waiting = 0;
boolean finish = 0;
int hash = 5;
int channel = 0;
int hopping_step = 0;
int message_count = 0;
int message=0;
unsigned long present_time;
unsigned long start_hopping_time;
unsigned long previous_exchanging_time=0;

void setup() {
  Serial.begin(9600);
  pinMode(button, INPUT);
  radio.begin();
  radio.openWritingPipe(ADDRESSES[1]);
  radio.openReadingPipe(1, ADDRESSES[0]);
  radio.setPALevel(RF24_PA_MIN);
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  //-- state checking before every loop --
  present_time = millis();
  button_state = digitalRead(button);

  //-- try to connect if not currently connected --
  if (connection_state == 0) {
    radio.setChannel(BASE_CHANNEL);
    handShake();
  }
  //-- sending messages --
  
  else {
    //Serial.println("start hopping time ="+String(start_hopping_time));
    hoppingChannel();
    /*
    if (button_state == 1) {
      message_count = 0;
      finish = 0;
      if (digitalRead(LED_BUILTIN)==HIGH) digitalWrite(LED_BUILTIN, LOW);
      else digitalWrite(LED_BUILTIN, HIGH);
    }
    */
    if (message_count < PACKAGE_NUM) {
      if (present_time - previous_exchanging_time > MESSAGE_INTERVAL) {
        message = 8888;
        sendMessage();
        message_count++;
        previous_exchanging_time = present_time;
        if (message_count%500 == 0) Serial.println(String(message_count) +"  messages sent");
      }
    }
    else {
      if (finish == 0) {
        Serial.println("Finish sending "+String(PACKAGE_NUM)+" messages");
        finish = 1;
      }
    }
  }
  //delay(1000);
}

void handShake(){
  if (present_time - previous_exchanging_time > MESSAGE_INTERVAL) {
    if (reply_waiting == 0) {
      message = hash;
      sendMessage();
      start_hopping_time = present_time+2*RTO;
      reply_waiting = 1;
      Serial.println("present time = "+String(present_time));
      Serial.println("start time = "+String(start_hopping_time));
    }
    else if (present_time < start_hopping_time - RTO) {
      receiveMessage();
      if (message == ACK) {
        //reply_waiting = 0;
        connection_state = 1;
        Serial.println("Handshake complete");
      }
    }
    else {
      reply_waiting = 0;
      Serial.println("Fail to handshake, redo sequence");
      if (digitalRead(LED_BUILTIN)==HIGH) digitalWrite(LED_BUILTIN, LOW);
      else digitalWrite(LED_BUILTIN, HIGH);
      }
    previous_exchanging_time = present_time;
  }
}

void sendMessage(){
  radio.stopListening();
  radio.write(&message, sizeof(message));
  //Serial.println(message);
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
    if (message != 0) Serial.println("receive "+String(message));
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
      //Serial.println("Hopping step count = "+String(hopping_step));
      //-- changing channel using hash --
      channel = (channel + hash)%(125-BASE_CHANNEL);
      radio.setChannel(BASE_CHANNEL+channel);
      Serial.println("Hopping to channel "+String(radio.getChannel()));
    }
}

/*    
  }
  if (init == 0) {
    radio.stopListening();
    radio.write(&hash, sizeof(hash));
    //digitalWrite(LED_BUILTIN, HIGH);
    //delay(100);
    //digitalWrite(LED_BUILTIN, LOW);
    delay(5);
    
    radio.startListening();
    if (radio.available()) radio.read(&init, sizeof(init));
    //while(!radio.available());
    //radio.read(&Init, sizeof(Init));
    if (init==1) {
      //radio.stopListening();
      digitalWrite(LED_BUILTIN, HIGH);
    }
    delay(5);

  }
  else {
    radio.stopListening();
    unsigned long current = millis();
    if (current - previous > interval) {
      previous = current;
      //if (digitalRead(LED_BUILTIN)==HIGH) digitalWrite(LED_BUILTIN, LOW);
      //else digitalWrite(LED_BUILTIN, HIGH);
      //change channel using hash
      channel = (channel + hash)%49;
      radio.setChannel(76+channel);
      Serial.println(radio.getChannel());
    }
    button_state = digitalRead(button);
    radio.write(&button_state, sizeof(button_state));
    if (button_state == 1) {
      if (digitalRead(LED_BUILTIN)==HIGH) digitalWrite(LED_BUILTIN, LOW);
      else digitalWrite(LED_BUILTIN, HIGH);
    }
    delay(5);
  }



  buttonState = digitalRead(button);
  if (buttonState == 1) {
    if (digitalRead(LED_BUILTIN)==HIGH) digitalWrite(LED_BUILTIN, LOW);
    else digitalWrite(LED_BUILTIN, HIGH);
  }
  radio.stopListening();
  radio.write(&buttonState, sizeof(buttonState));
  delay(200);
*/
/*
  radio.startListening();
  if (radio.available()) radio.read(&hash, sizeof(hash));
  if (hash % 2 == 0) digitalWrite(LED_BUILTIN, HIGH);
  else digitalWrite(LED_BUILTIN, LOW);
  delay(5);
*/

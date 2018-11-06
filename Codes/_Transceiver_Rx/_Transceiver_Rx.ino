#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(7, 8); // CE, CSN
const byte ADDRESSES[][6] = {"REPLY", "MAIN0"};
const long HOPPING_INTERVAL = 5000;
const long MESSAGE_INTERVAL = 10;
const int MAX_CHANNEL = 125;  
const int BASE_CHANNEL = 0;
const int DEFAULT_CHANNEL = 76;
const int CHANNEL_TO_CHECK = 12;            //12,37,62
const String MESSAGE_HEADER = "MSH";
const String HANDSHAKE_HEADER = "HSH";
const String ACK = "ACK";
const int RETRY_MAX = 4;
const long RTO = 2000;
const int PACKAGE_NUM = 10000;
//long previous = 0;
boolean connection_state = 0;
//boolean button_state = 0;
boolean send_state = 1;
boolean reply_waiting = 0;
boolean finish = 0;
int hash = 0;
int channel = 0;
//int sent_channel;
//int hopping_step = 0;
//int time_slot_count = 0;
//int message_count = 0;
//int message_count_old=0;
String message="";
//int channel_state[2*MAX_CHANNEL];
unsigned long present_time;
unsigned long start_hopping_time;
unsigned long next_hopping_time = 0;
unsigned long next_exchanging_time = 0;

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
    radio.setChannel(DEFAULT_CHANNEL);
    handShake();
  }
  //-- receiving messages --
  else {
    //Serial.println("start hopping time "+String(start_hopping_time));
    hoppingChannel();
    slotTiming();

    if (present_time >= start_hopping_time) {
      if (send_state == 0) {
        receiveMessage();
        String header = getValue(message, ',', 0);
        if (header == MESSAGE_HEADER) {
          reply_waiting = 1;
        }
      }
      else {
        if (reply_waiting == 1) {
          message = ACK;
          sendMessage();
          reply_waiting = 0;
        }
      }
    }
  
    /*    
    if (present_time < start_hopping_time + PACKAGE_NUM*MESSAGE_INTERVAL*1.2) {
      receiveMessage();
      if (message != "") message_count++;
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
      
    }
    else {
      if (finish == 0) {
        Serial.println("Received "+String(message_count)+" messages; Success rate = "+String(100*message_count/PACKAGE_NUM)+"%");
        finish = 1;
      }
    }*/
  }
}

void handShake(){
  slotTiming();
  
  if (send_state == 0) {
    receiveMessage();
    String header = getValue(message, ',', 0);
    if (header == HANDSHAKE_HEADER) {
      hash = getValue(message, ',', 1).toInt();
      Serial.println("Seed received = "+String(hash));
      reply_waiting =1;
      start_hopping_time = present_time+2*RTO;
    }
  }
  else {
    if (reply_waiting == 1) {
      message = ACK;
      sendMessage();
      //reply_waiting = 0;
    }
  }
  if ((reply_waiting == 1) && (present_time > start_hopping_time - RTO)) {
    reply_waiting = 0;
    connection_state = 1;
    channel = radio.getChannel();
    next_exchanging_time = start_hopping_time;
    send_state = 1;
    Serial.println("Handshake complete, start hopping time = "+String(start_hopping_time));
  }
  /*
  if ((present_time - previous_exchanging_time) > MESSAGE_INTERVAL) {
    //-- need better handshake: receive then send simultaneously. leave like this for now --
    if (reply_waiting == 0) {
      receiveMessage();
      if (message != "") {
        //message.remove(0,3);
        hash = getValue(message, ',', 1).toInt();
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
      Serial.println("start hopping time "+String(start_hopping_time));
    }
    if (previous_exchanging_time == 0) previous_exchanging_time = present_time;
    else previous_exchanging_time += MESSAGE_INTERVAL;
  }*/
}

void hoppingChannel(){
  //radio.stopListening();
  if (next_hopping_time < start_hopping_time) next_hopping_time = start_hopping_time;
  if (present_time > next_hopping_time) {
    next_hopping_time += HOPPING_INTERVAL;
      
    //-- changing channel using hash --
    channel = (channel + hash)%(125-BASE_CHANNEL);
    radio.setChannel(BASE_CHANNEL+channel);
    Serial.println("Hopping to channel "+String(radio.getChannel()));
  }
}

void slotTiming(){
  //-- Alternate time slot --
  if (present_time > next_exchanging_time) {
    if (send_state == 0) {
      send_state = 1;
      //reply_waiting = 0;
    }
    else send_state = 0;
    if (next_exchanging_time == 0) next_exchanging_time = present_time+MESSAGE_INTERVAL;
    else next_exchanging_time += MESSAGE_INTERVAL;
  }
}

void sendMessage(){
  radio.stopListening();
  char temp_message[32];
  message.toCharArray(temp_message,32);
  radio.write(&temp_message, sizeof(temp_message));
}

void receiveMessage(){
  char temp_message[32] = "";
  message = "";
  radio.startListening();
  if (radio.available()) {
    radio.read(&temp_message, sizeof(temp_message));
    if (temp_message != "") {
      message = String(temp_message);
      Serial.println("receive "+message);
    }
  }
}

String getValue(String data, char separator, int index){
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
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

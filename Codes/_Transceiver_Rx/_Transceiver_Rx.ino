#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(7, 8); // CE, CSN
const byte ADDRESSES[][6] = {"REPLY", "MAIN0"};
const int HOPPING_INTERVAL = 1000;
const int MESSAGE_INTERVAL = 10;
const int DEFAULT_CHANNEL = 76;
const int MAX_CHANNEL = 70;  
const int BASE_CHANNEL = 20;
const int CHANNEL_TO_CHECK = -1;            //12,37,62
const String MESSAGE_HEADER = "MSH";
const String HANDSHAKE_HEADER = "HSH";
const String ACK_HEADER = "ACK";
const int RETRY_MAX = 4;
const int RTO = 2000;
boolean flag_sync_state = 0;
boolean flag_send_slot = 0;
boolean flag_reply_waiting = 0;
boolean flag_ending_notification = 0;
int hash = 0;
int current_channel = 0;
String message = "";
boolean blacklist_state[MAX_CHANNEL];
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

  //-- answer confirmable message; seed in case of start up --
  if (!flag_sync_state) {
    //radio.setChannel(DEFAULT_CHANNEL);
    handShake();
  }
  
  //-- normal function: receiving messages --
  else {
    //Serial.println("start hopping time "+String(start_hopping_time));
    hoppingChannel();
    slotTiming();

    if (present_time >= start_hopping_time) {
      if (!flag_send_slot) {
        receiveMessage();
        String header = getValue(message, ',', 0);
        if (header == MESSAGE_HEADER) {
          flag_reply_waiting = 1;
        }
        else if (header == HANDSHAKE_HEADER){
          flag_sync_state = 0;
          flag_reply_waiting = 0;
        }
      }
      else {
        if (flag_reply_waiting) {
          message = ACK_HEADER;
          sendMessage();
          flag_reply_waiting = 0;
        }
      }
    }
  }
}

void handShake(){
  slotTiming();
  
  if (!flag_send_slot) {
    receiveMessage();
    String header = getValue(message, ',', 0);
    if (header == HANDSHAKE_HEADER) {
      switch (getValue(message, ',', 1).toInt()) {
        case 0: {
          hash = getValue(message, ',', 2).toInt();
          Serial.println("Seed received = "+String(hash));
          break;        
        }
        case 1: {
          //-- update blacklist channels --
          //int j;
          //for (j = BASE_CHANNEL; j < MAX_CHANNEL; j++)
          //  blacklist_state[j] = 0;

          int i=2;
          while (getValue(message, ',', i) != "") {
            blacklist_state[getValue(message, ',', i).toInt()] = 1;
            //Serial.println("blacklist channel "+String(getValue(message, ',', i).toInt()));
            i++;
          }
          break;
        }
      }
      
      flag_reply_waiting =1;
      next_exchanging_time = present_time+MESSAGE_INTERVAL;
      start_hopping_time = present_time+2*RTO;
    }
  }
  else {
    if (flag_reply_waiting) {
      message = ACK_HEADER+"H";
      sendMessage();
      flag_reply_waiting = 0;
    }
  }
  if (/*(flag_reply_waiting) &&*/ (present_time > start_hopping_time - RTO)) {
    flag_reply_waiting = 0;
    flag_sync_state = 1;
    current_channel = radio.getChannel();
    next_exchanging_time = start_hopping_time;
    flag_send_slot = 0;
    Serial.println("Handshake complete, start hopping time = "+String(start_hopping_time));
  }
}

void hoppingChannel(){
  //radio.stopListening();
  if (next_hopping_time < start_hopping_time) next_hopping_time = start_hopping_time;
  if (present_time > next_hopping_time) {
    next_hopping_time += HOPPING_INTERVAL;
    
    if (CHANNEL_TO_CHECK > 124 || CHANNEL_TO_CHECK < 0) {
      //-- changing channel using hash --
      current_channel = (current_channel - BASE_CHANNEL + hash)%(MAX_CHANNEL-BASE_CHANNEL) + BASE_CHANNEL;
      while (blacklist_state[current_channel])
        current_channel = (current_channel - BASE_CHANNEL + hash)%(MAX_CHANNEL-BASE_CHANNEL) + BASE_CHANNEL;
      radio.setChannel(current_channel);
      Serial.println("Hopping to channel "+String(radio.getChannel()));
    }
    else if (current_channel != CHANNEL_TO_CHECK) {
      current_channel = CHANNEL_TO_CHECK;
      radio.setChannel(current_channel);
      Serial.println("Hopping to channel "+String(radio.getChannel())); 
    }
  }
}

void slotTiming(){
  //-- Alternate time slot --
  if (present_time > next_exchanging_time) {
    if (!flag_send_slot) {
      flag_send_slot = 1;
    }
    else flag_send_slot = 0;
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
      boolean signal_status = radio.testRPD();
      message = String(temp_message) + "," + (signal_status? "StrongSignal" : "WeakSignal");
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

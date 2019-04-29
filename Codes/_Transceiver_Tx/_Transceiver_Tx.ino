#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#define button 4

RF24 radio(7, 8); // CE, CSN
const byte ADDRESSES[][6] = {"REPLY", "MAIN0"};
const long HOPPING_INTERVAL = 5000;
const long MESSAGE_INTERVAL = 10;
const int MAX_CHANNEL = 125; 
const int BASE_CHANNEL = 0;
const int DEFAULT_CHANNEL = 76;
const int CHANNEL_TO_CHECK = -1;            //12,37,62
const String MESSAGE_HEADER = "MSH";
const String HANDSHAKE_HEADER = "HSH";
const String ACK = "ACK";
const int RETRY_MAX = 4;
const long RTO = 2000;
const int PACKAGE_NUM = 10000;
//long previous = 0;
boolean connection_state = 0;
boolean button_state = 0;
boolean send_state = 0;
boolean reply_waiting = 0;
boolean finish = 0;
int hash = 5;
int channel = 0;
int sent_channel;
//int hopping_step = 0;
//int time_slot_count = 0;
int message_count = 0;
String message="";
int channel_state[2*MAX_CHANNEL];
unsigned long present_time;
unsigned long start_hopping_time;
unsigned long next_hopping_time = 0;
unsigned long next_exchanging_time = 0;

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
    radio.setChannel(DEFAULT_CHANNEL);
    handShake();
  }
  //-- sending messages --
  
  else {
    //Serial.println("start hopping time ="+String(next_exchanging_time));
    hoppingChannel();
    slotTiming();

    if (present_time >= start_hopping_time) {
      if (send_state == 1) {
        if (message_count < PACKAGE_NUM) {
          if (reply_waiting == 0) {
            message = MESSAGE_HEADER+",CH "+String(radio.getChannel())+","+String(message_count);
            sendMessage();
            if (sent_channel != channel) sent_channel = channel;
            reply_waiting = 1;
            channel_state[2*sent_channel]++;
            message_count++;
            if (message_count%500 == 0) Serial.println(String(message_count) +"  messages sent");
          }
        }
      }
      else {
        receiveMessage();
        reply_waiting = 0;
        //if (present_time < start_hopping_time - RTO) {
          if (message == ACK) {
            //reply_waiting = 0;
            //Serial.println("Packet sends successfully");
            channel_state[2*sent_channel+1]++;
          }
        //}
       if (message_count >= PACKAGE_NUM) {
        if (finish == 0) {
          Serial.println("Finish sending "+String(PACKAGE_NUM)+" messages");
          finish = 1;
          int i;
          float count_send=0; float count_ack=0;
          for (i = 0; i < 125; i++){
            if (channel_state[2*i] != 0) {
              Serial.println("Number of message sent in channel "+String(i)+" : "+String(channel_state[2*i]));
              Serial.println("    Number of message sent successfully : "+String(channel_state[2*i+1]));
              count_send += channel_state[2*i];
              count_ack  += channel_state[2*i+1];
            }
          }
          Serial.println("Packet sent = "+String(count_send));
          Serial.println("Packet loss  = "+String(count_send-count_ack));
          float loss_per = (count_send-count_ack)/count_send;
          Serial.println("Average Packet Loss = "+String(100*loss_per));
        }
      }
      }

    }


    /*
    if (button_state == 1) {
      message_count = 0;
      finish = 0;
      if (digitalRead(LED_BUILTIN)==HIGH) digitalWrite(LED_BUILTIN, LOW);
      else digitalWrite(LED_BUILTIN, HIGH);
    }
    
    if (message_count < PACKAGE_NUM) {
      if (present_time - previous_exchanging_time > MESSAGE_INTERVAL) {
        message = MESSAGE_HEADER+","+String(message_count);
        sendMessage();
        message_count++;
        previous_exchanging_time += MESSAGE_INTERVAL;
        if (message_count%500 == 0) Serial.println(String(message_count) +"  messages sent");
      }
    }
    else {
      if (finish == 0) {
        Serial.println("Finish sending "+String(PACKAGE_NUM)+" messages");
        finish = 1;
      }
    }*/
  }
}

void handShake(){
  slotTiming();
  
  if (send_state == 1) {
    //-- If not sent --
    if (reply_waiting == 0) {
      message = HANDSHAKE_HEADER+","+String(hash);
      sendMessage();
      start_hopping_time = present_time+2*RTO;
      reply_waiting = 1;
      Serial.println("present time = "+String(present_time));
      Serial.println("start time = "+String(start_hopping_time));      
    }
  }
  else {
    receiveMessage();
    //if (present_time < start_hopping_time - RTO) {
      if (message == ACK) {
        reply_waiting = 0;
        connection_state = 1;
        channel = radio.getChannel();
        next_exchanging_time = start_hopping_time;
        send_state = 0;
        Serial.println("Handshake complete");
      }
    //}
  }
  //-- Resend seed if RTO is reached --
  if (present_time > start_hopping_time - RTO) {
    reply_waiting = 0;
    Serial.println("Fail to handshake, redo sequence");
    if (digitalRead(LED_BUILTIN)==HIGH) digitalWrite(LED_BUILTIN, LOW);
    else digitalWrite(LED_BUILTIN, HIGH);
  }
  /*
  if ((present_time - previous_exchanging_time) > MESSAGE_INTERVAL) {
    if (reply_waiting == 0) {
      message = HANDSHAKE_HEADER+","+String(hash);
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
        channel = radio.getChannel();
        Serial.println("Handshake complete");
      }
    }
    else {
      reply_waiting = 0;
      Serial.println("Fail to handshake, redo sequence");
      if (digitalRead(LED_BUILTIN)==HIGH) digitalWrite(LED_BUILTIN, LOW);
      else digitalWrite(LED_BUILTIN, HIGH);
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

    if (CHANNEL_TO_CHECK > 124 || CHANNEL_TO_CHECK < 0) {
      //-- changing channel using hash --
      channel = (channel + hash)%(MAX_CHANNEL-BASE_CHANNEL);
      radio.setChannel(BASE_CHANNEL+channel);
      Serial.println("Hopping to channel "+String(radio.getChannel()));
    }
    else if (channel != CHANNEL_TO_CHECK) {
      channel = CHANNEL_TO_CHECK;
      radio.setChannel(BASE_CHANNEL+channel);
      Serial.println("Hopping to channel "+String(radio.getChannel())); 
    }
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
      //Serial.println("receive "+message);
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

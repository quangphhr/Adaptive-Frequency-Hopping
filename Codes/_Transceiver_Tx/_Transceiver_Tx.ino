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
const String BLL_HEADER = "BLL";
const String ACK = "ACK";
const String BLACK = "BLACK";
String bll="";
const int RETRY_MAX = 4;
const long RTO = 2000;
const int PACKAGE_NUM = 10000;
int bll_waiting_time=0;
boolean connection_state = 0;
boolean button_state = 0;
boolean send_state = 0;
boolean reply_waiting = 0;
boolean finish = 0;
int hash = 5;
int channel = 0;
int sent_channel;
int message_count = 0;
String message="";
int channel_state[3*MAX_CHANNEL];
int blacklisted[MAX_CHANNEL];
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
  for(int i=0;i<125;i++){
    blacklisted[i]=200;
  }
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
//    if(i==0){
//    Serial.println("This is after Handshake,present time ="+String(present_time)+"start hopping  time ="+String(start_hopping_time)+"next hopping time ="+String(next_hopping_time)+"next exchanging time ="+String(next_exchanging_time));
//    }
//    i++;
    hoppingChannel();
    //Serial.println("channel number is "+String(channel));
    slotTiming();
    //Serial.println("This is after Hopping,present time ="+String(present_time)+"start hopping  time ="+String(start_hopping_time)+"next hopping time ="+String(next_hopping_time)+"next exchanging time ="+String(next_exchanging_time));

    if (present_time >= start_hopping_time) {
      if (send_state == 1) {
        if (message_count < PACKAGE_NUM) {
          if (reply_waiting == 0) {
            message = MESSAGE_HEADER+",CH "+String(radio.getChannel())+","+String(message_count);
            sendMessage();
            //Serial.println("Message just sent and sent channel is  "+String(sent_channel));
            if (sent_channel != channel) sent_channel = channel;
            reply_waiting = 1;
            channel_state[3*sent_channel]++;
            message_count++;
            if (message_count%500 == 0) Serial.println(String(message_count) +"  messages sent");
          }
        }
      }
      else {
        receiveMessage();
        reply_waiting = 0;
          if (message == ACK) {
            channel_state[3*sent_channel+1]++;
          }
      // This is where blacklisting process happens
       if(message_count%500==0) {
        for (int i = 0; i < 125; i++){
            if (channel_state[3*i] != 0) {
              Serial.println("Number of message sent in channel "+String(i)+" : "+String(channel_state[3*i]));
              Serial.println(" Number of message sent successfully : "+String(channel_state[3*i+1]));
              float count_send = channel_state[3*i];
              float count_ack  = channel_state[3*i+1];
              float PER = (count_send-count_ack)/count_send;
              channel_state[3*i+2] = PER;
              if(PER > 0.5){
                blacklisted[i]= i;
                }
            }
          }
        for(int i =0;i < 125 ; i++){
          if(blacklisted[i] != 200){
            bll = bll + ','+String(i);
            }
          }
        bll = bll +',';
        send_bll:
        bll_waiting_time = present_time + HOPPING_INTERVAL;
        if(bll!=""){
        message = BLL_HEADER+bll;
        sendMessage();
        delay(250);
        receiveMessage();
          if (message == BLACK) {
            Serial.println("Blacklist received successfully");
           }
          else {goto send_bll;}
        }
        }  
       if (message_count >= PACKAGE_NUM) {
        if (finish == 0) {
          Serial.println("Finish sending "+String(PACKAGE_NUM)+" messages");
          finish = 1;
          int i;
          float total_count_send=0; float total_count_ack=0;
          for (i = 0; i < 125; i++){
            if (channel_state[3*i] != 0) {
              Serial.println("Number of message sent in channel "+String(i)+" : "+String(channel_state[3*i]));
              Serial.println(" Number of message sent successfully : "+String(channel_state[3*i+1]));
              total_count_send += channel_state[3*i];
              total_count_ack  += channel_state[3*i+1];
            }
          }
          Serial.println("Packet sent = "+String(total_count_send));
          Serial.println("Packet loss  = "+String(total_count_send-total_count_ack));
          float loss_per = (total_count_send-total_count_ack)/total_count_send;
          Serial.println("Average Packet Loss = "+String(100*loss_per));
          exit(0);
        }
      }
      }

    }

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
 
}

void hoppingChannel(){
  //radio.stopListening();
  if (next_hopping_time < start_hopping_time) next_hopping_time = start_hopping_time;
  if (present_time > next_hopping_time) {
    next_hopping_time += HOPPING_INTERVAL;
    Serial.println("present time and next_hopping_time before Hopping : "+String(present_time)+","+String(next_hopping_time));
    if (CHANNEL_TO_CHECK > 124 || CHANNEL_TO_CHECK < 0) {
      for(int s=0 ; s<125;s++){
        if(s!=200 && channel == s - hash){
          channel = (channel + 2*hash)%(MAX_CHANNEL-BASE_CHANNEL);
          radio.setChannel(BASE_CHANNEL+channel);
          Serial.println("Hopping to channel "+String(radio.getChannel()));
          break;
          }
        }
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

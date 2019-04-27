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
boolean connection_state = 0;
boolean send_state = 1;
boolean reply_waiting = 0;
boolean finish = 0;
int hash = 0;
int channel = 0;
int message_count = 0;
int blacklisted[MAX_CHANNEL];
String message="";
unsigned long present_time;
unsigned long start_hopping_time;
unsigned long next_hopping_time = 0;
unsigned long next_exchanging_time = 0;
//int i;

void setup() {
  Serial.begin(9600);
  radio.begin();
  radio.openWritingPipe(ADDRESSES[0]);
  radio.openReadingPipe(1, ADDRESSES[1]);
  radio.setPALevel(RF24_PA_MIN);
  pinMode(LED_BUILTIN, OUTPUT);
  for(int i=0;i<125;i++){
    blacklisted[i]=200;
  }
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
//    if(i==0){
//    Serial.println("This is after Handshake, present time ="+String(present_time)+"start hopping  time ="+String(start_hopping_time)+"next hopping time ="+String(next_hopping_time)+"next exchanging time ="+String(next_exchanging_time));
//    }
//    i++;
    hoppingChannel();
    //Serial.println("channel number is "+String(channel));
    slotTiming();
    //Serial.println("This is after Hopping,present time ="+String(present_time)+"start hopping  time ="+String(start_hopping_time)+"next hopping time ="+String(next_hopping_time)+"next exchanging time ="+String(next_exchanging_time));

    if (present_time >= start_hopping_time) {
      if (send_state == 0) {
        receiveMessage();
        String header = getValue(message, ',', 0);
        if (header == MESSAGE_HEADER) {
          reply_waiting = 1;
          message_count++;
        }
        else if(header == BLL_HEADER){
          bll = getValue(message, ',', 1);
          extractnumbers(bll,blacklisted);
          message = BLACK;
          sendMessage();                        
        }
      }
      else {
        if (reply_waiting == 1) {
          message = ACK;
          sendMessage();
          //Serial.println("Acknowledgement just sent ");
          reply_waiting = 0;
        }
        if (message_count >= PACKAGE_NUM) {
        
          exit(0);
        
      }
      }
    }
   
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

void extractnumbers(String strName, int blistarray[125]){
   for (int number=0; number < strName.length(); number++){
        if(isdigit (strName[number])){
          blistarray[strName[number]]= strName[number];
        }
    }
}

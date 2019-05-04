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
String bll=",";
const int RETRY_MAX = 4;
const long RTO = 2000;
const int PACKAGE_NUM = 1250;
int bll_waiting_time=0;
boolean connection_state = 0;
boolean send_state = 1;
boolean reply_waiting = 0;
boolean finish = 0;
boolean bll_status = 0;
int hash = 0;
int channel = 0;
int message_count = 0;
int blacklisted[MAX_CHANNEL];
String message="";
unsigned long present_time;
unsigned long start_hopping_time;
unsigned long next_hopping_time = 0;
unsigned long next_exchanging_time = 0;

void setup() {
  Serial.begin(9600);
  while(!Serial);
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
    //Serial.println("start hopping time "+String(start_hopping_time));
   if(bll_status == 1){
      blacklisting();
      }
    else if (bll_status == 0){
    hoppingChannel();
    slotTiming();
    }

    if (present_time >= start_hopping_time && bll_status ==0) {
      if (send_state == 0) {
        receiveMessage();
        String header = getValue(message, ',', 0);
        message_count = getValue(message,',',2).toInt();
        if (header == MESSAGE_HEADER) {
          reply_waiting = 1;
        }
        else if(header == BLL_HEADER){
          Serial.println("the current channel is "+String(radio.getChannel()));
          if(bll!=",|"){ //&& present_time < bll_waiting_time){
            bll_status = 1;

          }                       
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

void blacklisting(){
  
  slotTiming();
  
  if (send_state == 0) {
    receiveMessage();
    String header = getValue(message, ',', 0);
    if (header == BLL_HEADER) {
      bll = getValue(message, ',', 1);
      Serial.println("bll received = "+String(hash));
      reply_waiting =1;
      next_hopping_time = present_time+2*RTO;
      extractnumbers(bll,'|',blacklisted);
      for(int i=0;i<125;i++){
        if(blacklisted[i]!=200){
          Serial.println("Channel "+String(blacklisted[i])+" is now blacklisted ");
           }
        }
    }
  }
  else {
    if (reply_waiting == 1) {
      message = BLACK;
      sendMessage();
      Serial.println("Blacklist acknowledgment sent and the blacklist recieved  was "+bll);
      //reply_waiting = 0;
    }
  }
  if ((reply_waiting == 1) && (present_time > next_hopping_time - RTO)) {
    reply_waiting = 0;
    bll_status = 0;
    channel = radio.getChannel();
    //next_exchanging_time = start_hopping_time;
    send_state = 1;
    Serial.println("Blacklisting Handshake complete, next hopping time = "+String(next_hopping_time));
  } 
  
} 
  
  
void hoppingChannel(){
  //radio.stopListening();
  if (next_hopping_time < start_hopping_time) next_hopping_time = start_hopping_time;
  if (present_time > next_hopping_time) {
    next_hopping_time += HOPPING_INTERVAL;
   // for(byte y=0 ; y < sizeof(blacklisted);y++){
    //  Serial.println("At index "+String(y)+" of blacklisted, the value is "+String(blacklisted[y]));
  //   }
    if (CHANNEL_TO_CHECK > 124 || CHANNEL_TO_CHECK < 0) {
      //-- changing channel using hash --
      hopLabel:
      channel = (channel + hash)%(MAX_CHANNEL-BASE_CHANNEL);
      for(byte s=0 ; s<125;s++){
        if(blacklisted[s]!=200 && channel == blacklisted[s] ){
          channel = (channel + hash)%(MAX_CHANNEL-BASE_CHANNEL);
          //radio.setChannel(BASE_CHANNEL+channel);
          Serial.println("Dodged channel "+ String(blacklisted[s]));
          goto hopLabel;
          }
        }
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
      Serial.println("received "+message);
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
void extractnumbers(String data,char seperator, int blistarray[125]){
    int found = 0;
    int maxIndex = data.length();
   // Serial.println("The length of bll is"+String(maxIndex));
    int los = 0;
    los = maxIndex/2;
 //   Serial.println(los);
    int sepIndex[los];
  //  Serial.println("The length of SepIndex is"+String(sizeof(sepIndex)));
    int j=0;
    for(int x=0 ; x < sizeof(sepIndex);x++){
      sepIndex[x]=200;
     }
    for(int i=0; i< maxIndex ; i++){
      if(data.charAt(i)== seperator){
        sepIndex[j] = i;
        j++;
        }
      }

  //  for(int x=0 ; x < sizeof(sepIndex);x++){
  //    if(sepIndex[x]!=200){
   //     Serial.println("At index"+String(x)+"of sepIndex, the value is"+String(sepIndex[x]));
     //   }
      }
    for (int n=0; n < sizeof(sepIndex); n++){ //0,1,2 :0,3,6
      if(data[sepIndex[n]]== seperator && data[sepIndex[n+1]]== seperator){
        String b = data.substring(sepIndex[n]+1,sepIndex[n+1]);
        Serial.println(b);
        blistarray[b.toInt()]= b.toInt();
      }
      }
//    for(int y=0 ; y < sizeof(blistarray);y++){
//      if(blistarray[y]!=200){
//        Serial.println("At index"+String(y)+"of sepIndex, the value is"+String(blistarray[y]));
//        }
//      }
}

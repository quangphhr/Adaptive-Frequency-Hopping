#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#define button 4

RF24 radio(7, 8); // CE, CSN
const byte ADDRESSES[][6] = {"REPLY", "MAIN0"};
const int HOPPING_INTERVAL = 1000;
const int MESSAGE_INTERVAL = 10;
const int DEFAULT_CHANNEL = 76;
const int MAX_CHANNEL = 125;
const int BASE_CHANNEL = 0;
const int CHANNEL_TO_CHECK = -1;            // Hopping channel if not [0,124]
const boolean BLACKLIST_MODE = 1;
const float BLACKLIST_TOL = 0.5;
const boolean DATA_ACQ_MODE = 1;            // 10.000 messages each channel. Repeat PACKAGE_TIMES times
const String MESSAGE_HEADER = "MSH";
const String HANDSHAKE_HEADER = "HSH";
const String ACK_HEADER = "ACK";
const int RETRY_MAX = 4;
const int RTO = 2000;
long PACKAGE_NUM = 10001;                   // should not be >32000 each channel
const int PACKAGE_TIMES = 1;
boolean flag_sync_state = 0;
boolean flag_button_state = 0;
boolean flag_send_slot = 1;
boolean flag_reply_waiting = 0;
boolean flag_ending_notification = 0;
int hash = 5;                               // MAX_CHANNEL-BASE_CHANNEL % hash = 0
int current_channel = 0;
int sent_channel;
long message_count = 0;
String message = "";
String con_message = "";
int channel_state[2 * MAX_CHANNEL];
boolean blacklist_state[MAX_CHANNEL];
boolean blacklist_store[MAX_CHANNEL];
int data_acq_times = 1;
float data_acq_value[2 * MAX_CHANNEL];
unsigned long present_time;
unsigned long start_hopping_time;
unsigned long next_hopping_time = 0;
unsigned long next_exchanging_time = 0;

void setup() {
  Serial.begin(9600);
  radio.begin();
  radio.openWritingPipe(ADDRESSES[1]);
  radio.openReadingPipe(1, ADDRESSES[0]);
  radio.setPALevel(RF24_PA_MIN);

  pinMode(button, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  //-- when data acc mode on
  if (DATA_ACQ_MODE) {
    PACKAGE_NUM = 250 * long(MAX_CHANNEL - BASE_CHANNEL) + 1;
    hash = 5;
  }

  //-- handshake message for start up --
  con_message = HANDSHAKE_HEADER + ",0," + String(hash);

}

void loop() {
  //-- state checking before every loop --
  present_time = millis();
  flag_button_state = digitalRead(button);

  //-- send confirmable message; seed in case of start up --
  if (!flag_sync_state) {
    //radio.setChannel(DEFAULT_CHANNEL);
    handShake();
  }

  //-- normal function: send messages --
  else {
    //Serial.println("start hopping time ="+String(next_exchanging_time));
    hoppingChannel();
    slotTiming();

    if (present_time >= start_hopping_time) {
      if (flag_send_slot) {
        if (message_count <= PACKAGE_NUM) {
          if (!flag_reply_waiting) {
            message = MESSAGE_HEADER + ",CH " + String(radio.getChannel()) + "," + String(message_count);
            sendMessage();
            flag_reply_waiting = 1;
            //-- store data --
            if (sent_channel != current_channel) sent_channel = current_channel;
            channel_state[2 * sent_channel]++;
            message_count++;
            if (message_count % 5000 == 0) {
              Serial.println(String(message_count) + "  messages sent");
              if (BLACKLIST_MODE) {
                //-- update blacklist channels --
                //int j;
                //for (j = BASE_CHANNEL; j < MAX_CHANNEL; j++)
                //  blacklist_state[j] = 0;

                //-- check for bad channel --
                boolean temp_bll = 0;
                String bll_store;
                int i; float count_send; float count_ack;
                for (i = BASE_CHANNEL; i < MAX_CHANNEL; i++) {
                  count_send = channel_state[2 * i];
                  count_ack = channel_state[2 * i + 1];
                  bll_store += String(blacklist_store[i]);
                  if (((1-count_ack / count_send) > BLACKLIST_TOL) && !blacklist_store[i]) {
                    blacklist_state[i] = 1;
                    Serial.println("Blacklist channel " + String(i));
                    temp_bll = 1;
                  }
                }
                
                Serial.println("blacklist_store = "+bll_store);

                //-- if blacklist is needed
                if (temp_bll) {
                  con_message = HANDSHAKE_HEADER + ",1";
                  int i;
                  for (i = BASE_CHANNEL; i < MAX_CHANNEL; i++) {
                    if (blacklist_state[i] && !blacklist_store[i]) {
                      con_message = con_message + "," + String(i);
                      blacklist_store[i] = 1;
                    //Serial.println(con_message);
                    }
                  }
                  flag_sync_state = 0;
                }
              }
            }
          }
        }
      }
      else {
        receiveMessage();
        flag_reply_waiting = 0;
        if (message == ACK_HEADER) {
          //Serial.println("Packet sends successfully");
          channel_state[2 * sent_channel + 1]++;
        }

        if (message_count >= PACKAGE_NUM && (!DATA_ACQ_MODE || DATA_ACQ_MODE && (PACKAGE_TIMES == 1))) {
          if (!flag_ending_notification) {
            Serial.println("Finish sending " + String(PACKAGE_NUM - 1) + " messages");
            flag_ending_notification = 1;
            int i;
            float count_send = 0; float count_ack = 0;
            for (i = BASE_CHANNEL; i < MAX_CHANNEL; i++) {
              if (channel_state[2 * i] != 0) {
                Serial.println("Number of message sent in channel " + String(i) + " : " + String(channel_state[2 * i]));
                Serial.println("    Number of message sent successfully : " + String(channel_state[2 * i + 1]));
                count_send += channel_state[2 * i];
                count_ack  += channel_state[2 * i + 1];
              }
            }
            Serial.println("Packet sent = " + String(count_send, 0));
            Serial.println("Packet loss  = " + String(count_send - count_ack, 0));
            float loss_per = (count_send - count_ack) / count_send;
            Serial.println("Average Packet Loss = " + String(100 * loss_per));
          }
        }

        if (message_count >= PACKAGE_NUM && DATA_ACQ_MODE && (PACKAGE_TIMES > 1)) {
          if (data_acq_times < PACKAGE_TIMES) {
            message_count = 0;
          }
          if (data_acq_times <= PACKAGE_TIMES) {
            data_acq_times++;

            int i;
            for (i = BASE_CHANNEL; i < MAX_CHANNEL; i++) {
              if (channel_state[2 * i] != 0) {
                float temp_per = 1 - float(channel_state[2 * i + 1]) / float(channel_state[2 * i]);
                channel_state[2 * i] = 0;
                channel_state[2 * i + 1] = 0;
                data_acq_value[2 * i] += temp_per;
                data_acq_value[2 * i + 1] += temp_per * temp_per;
              }
            }
          }
          else if (!flag_ending_notification) {
            flag_ending_notification = 1;
            int i;
            for (i = BASE_CHANNEL; i < MAX_CHANNEL; i++) {
              //if (channel_state[2*i] != 0) {
              Serial.println("Average packet loss rate of channel " + String(i) + " : " + String(data_acq_value[2 * i] / PACKAGE_TIMES));
              float temp_var = (data_acq_value[2 * i + 1] - (data_acq_value[2 * i] * data_acq_value[2 * i]) / PACKAGE_TIMES) / (PACKAGE_TIMES - 1);
              Serial.println("    Data variance : " + String(temp_var));
              //}
            }
          }
        }
      }
    }
  }
}

void handShake() {
  slotTiming();

  if (flag_send_slot) {
    //-- If not sent --
    if (!flag_reply_waiting) {
      message = con_message;
      sendMessage();
      start_hopping_time = present_time + 2 * RTO;
      flag_reply_waiting = 1;
      //Serial.println("present time = "+String(present_time));
      //Serial.println("start time = "+String(start_hopping_time));
    }
  }
  else {
    receiveMessage();
    flag_reply_waiting = 0;
    if (message == ACK_HEADER+"H") {
      flag_sync_state = 1;
      current_channel = radio.getChannel();
      next_exchanging_time = start_hopping_time;
      flag_send_slot = 1;
      Serial.println("Handshake complete");
    }
  }
  //-- Resend seed if RTO is reached --
  //-- not workig now
  //if (present_time > start_hopping_time - RTO) {
  //  flag_reply_waiting = 0;
  //  Serial.println("Fail to handshake, redo sequence");
  //  if (digitalRead(LED_BUILTIN)==HIGH) digitalWrite(LED_BUILTIN, LOW);
  //  else digitalWrite(LED_BUILTIN, HIGH);
  //}
}

void hoppingChannel() {
  //radio.stopListening();
  if (next_hopping_time < start_hopping_time) next_hopping_time = start_hopping_time;
  if (present_time > next_hopping_time) {
    next_hopping_time += HOPPING_INTERVAL;

    if (CHANNEL_TO_CHECK > 124 || CHANNEL_TO_CHECK < 0) {
      //-- changing channel using hash --
      current_channel = (current_channel - BASE_CHANNEL + hash) % (MAX_CHANNEL - BASE_CHANNEL) + BASE_CHANNEL;
      while (blacklist_state[current_channel])
        current_channel = (current_channel - BASE_CHANNEL + hash) % (MAX_CHANNEL - BASE_CHANNEL) + BASE_CHANNEL;
      radio.setChannel(current_channel);
      if (DATA_ACQ_MODE) {
        if (!flag_ending_notification)
          Serial.println("Hopping to channel " + String(radio.getChannel()));
      }
      else Serial.println("Hopping to channel " + String(radio.getChannel()));
    }
    else if (current_channel != CHANNEL_TO_CHECK) {
      current_channel = CHANNEL_TO_CHECK;
      radio.setChannel(current_channel);
      Serial.println("Hopping to channel " + String(radio.getChannel()));
    }
  }
}

void slotTiming() {
  //-- Alternate time slot --
  if (present_time > next_exchanging_time) {
    if (!flag_send_slot) {
      flag_send_slot = 1;
    }
    else flag_send_slot = 0;
    if (next_exchanging_time == 0) next_exchanging_time = present_time + MESSAGE_INTERVAL;
    else next_exchanging_time += MESSAGE_INTERVAL;
  }
}

void sendMessage() {
  radio.stopListening();
  char temp_message[32];
  message.toCharArray(temp_message, 32);
  radio.write(&temp_message, sizeof(temp_message));
}

void receiveMessage() {
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

String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

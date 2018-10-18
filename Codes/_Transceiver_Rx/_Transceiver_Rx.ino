#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(7, 8); // CE, CSN
const byte addresses[][6] = {"Reply", "Main0"};
const long Interval = 10000;
long Previous = 0;
boolean Init = 0;
boolean buttonState = 0;
int hash = 0;
int channel = 0;

void setup() {
  Serial.begin(9600);
  radio.begin();
  radio.openWritingPipe(addresses[0]);
  radio.openReadingPipe(1, addresses[1]);
  radio.setPALevel(RF24_PA_MIN);
  //radio.startListening();
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  
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
  }
  else {
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
*/
}

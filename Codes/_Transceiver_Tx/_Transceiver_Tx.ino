#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#define button 4

RF24 radio(7, 8); // CE, CSN
const byte addresses[][6] = {"Reply", "Main0"};
const long Interval = 10000;
long Previous = 0;
boolean Init = 0;
boolean buttonState = 0;
int hash = 5;
int channel = 0;

void setup() {
  Serial.begin(9600);
  pinMode(button, INPUT);
  radio.begin();
  radio.openWritingPipe(addresses[1]);      //Write to Main
  radio.openReadingPipe(1, addresses[0]);   //Receive confirmation
  radio.setPALevel(RF24_PA_MIN);
  //radio.stopListening();
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  
  if (Init == 0) {
    radio.stopListening();
    radio.write(&hash, sizeof(hash));
    //digitalWrite(LED_BUILTIN, HIGH);
    //delay(100);
    //digitalWrite(LED_BUILTIN, LOW);
    delay(5);
    
    radio.startListening();
    if (radio.available()) radio.read(&Init, sizeof(Init));
    //while(!radio.available());
    //radio.read(&Init, sizeof(Init));
    if (Init==1) {
      //radio.stopListening();
      digitalWrite(LED_BUILTIN, HIGH);
    }
    delay(5);

  }
  else {
    radio.stopListening();
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
    buttonState = digitalRead(button);
    radio.write(&buttonState, sizeof(buttonState));
    if (buttonState == 1) {
      if (digitalRead(LED_BUILTIN)==HIGH) digitalWrite(LED_BUILTIN, LOW);
      else digitalWrite(LED_BUILTIN, HIGH);
    }
    delay(5);
  }


/*
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
}


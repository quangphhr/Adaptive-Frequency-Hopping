
// Core library for code-sense - IDE-based
#include "Energia.h"


// Include application, user and local libraries

#ifndef __CC3200R1M1RGC__
#include <SPI.h>                // Do not include SPI for CC3200 LaunchPad
#endif
#include <WiFi.h>


// Define structures and classes


// Define variables and constants
char wifi_name[] = "energia";
char wifi_password[] = "launchpad";
char st_ssid[] = "Tech_D0042985";
char st_password[] = "GYJJYJRZ";

int count = 0;
int port = 5000;
int channel = 6;
unsigned long check_point = 0;
const int CHECK_PERIOD = 300000;
int message_interval = 10;
//boolean reset_flag = 0;

WiFiServer myServer(port);
uint8_t oldCountClients = 0;
uint8_t countClients = 0;


// Add setup code
void setup()
{
    Serial.begin(115200);
    delay(500);
    
    Serial.println("*** LaunchPad CC3200 WiFi Web-Server in AP Mode");

    // Start WiFi and create a network with wifi_name as the network name
    // with wifi_password as the password.
    Serial.print("Starting AP... ");
    
    WiFi.beginNetwork(wifi_name, wifi_password);
    sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_CHANNEL, 1, (unsigned char *) &channel);
    //sl_WlanRxStatStart();
    
    while (WiFi.localIP() == INADDR_NONE)
    {
        // print dots while we wait for the AP config to complete
        Serial.print('.');
        delay(300);
    }
    Serial.println("DONE");    
    Serial.print("LAN name = ");
    Serial.println(wifi_name);
    Serial.print("WPA password = ");
    Serial.println(wifi_password);
    pinMode(GREEN_LED, OUTPUT);      // set the LED pin mode
    digitalWrite(GREEN_LED, LOW);
    IPAddress ip = WiFi.localIP();
    Serial.print("Webserver IP address = ");
    Serial.println(ip);
    Serial.print("Web-server port = ");
    myServer.begin();                           // start the web server on port 5000
    Serial.println(port);
    Serial.println();
    
}

// Add loop code
void loop()
{
    countClients = WiFi.getTotalDevices();
    //Serial.println(" countClients = " + String(countClients));
    
    // Did a client connect/disconnect since the last time we checked?
    if (countClients != oldCountClients)
    {
        if (countClients > oldCountClients)
        {  // Client connect
            digitalWrite(GREEN_LED, HIGH);
            Serial.println("Client connected to AP");
            for (uint8_t k = 0; k < countClients; k++)
            {
                Serial.print("Client #");
                Serial.print(k);
                Serial.print(" at IP address = ");
                Serial.print(WiFi.deviceIpAddress(k));
                Serial.print(", MAC = ");
                Serial.println(WiFi.deviceMacAddress(k));
                //long rssi = WiFi.RSSI();
                //Serial.println("Signal Strength = " + String(rssi) + " dBm");
                Serial.println("CC3200 in AP mode only accepts one client.");
            }
        }
        else
        {  // Client disconnect
            //            digitalWrite(RED_LED, !digitalRead(RED_LED));
            Serial.println("Client disconnected from AP.");
            Serial.println();
        }
        oldCountClients = countClients;
    }

    WiFiClient myClient = myServer.available();
    Serial.println(myClient.status());
    Serial.println(WiFi.getSocket());
    Serial.println();
    
    if (myClient) {                             // if you get a client,
        Serial.println(". Client connected to server");           // print a message out the serial port
        check_point = millis();
        Serial.println("assign checkpoint complete");

        while(1) {//myClient.connected()){

            String MSH = "From channel "+String(channel)+ " to you with "+String(count)+ " love!";
            myClient.print(MSH);
            Serial.println(" Sent: "+ MSH +" to the client");
            count ++;
            delay(message_interval);

            if ((check_point + CHECK_PERIOD) <= millis()) {
                check_point = millis();
                Serial.println("assign checkpoint complete");
                channel = (channel + 5)%15;
                Serial.println(channel);
                //myClient.flush();
                //WiFi.disconnect();
                Serial.println("Check point 0");
                sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_CHANNEL, 1, (unsigned char *) &channel);
                Serial.println("assign channel complete");

                Serial.println("Check point 1");
                myClient.flush();
                if (myClient) myClient.stop();
                delay(2000);

                Serial.println("Check point 2");
                sl_WlanDisconnect();
                delay(2000);
                
                Serial.println("Check point 3");
                sl_WlanProfileDel(0xff);
                sl_Stop(0);
                
                Serial.println("Check point 4");
                sl_Start(NULL, NULL, NULL);
                
                Serial.println("RESET complete");
                delay(1000);
                myServer.begin();
                delay(1000);
                break;
            }
          }
        // close the connection:
        //myClient.stop();
        Serial.println(" Client disconnected from server ");
        Serial.println();
        digitalWrite(GREEN_LED, LOW);             
    }
    //Serial.println(" Loop checking ");
    //myClient.stop();
    //myServer.begin();
    //delay(500);
 
    if (WiFi.getSocket() == 255) {
        Serial.println(WiFi.getSocket());

        WiFi._initialized = false;
        WiFi._connecting = false;
        WiFi.begin(st_ssid, st_password);
        //while(WiFi.status() != WL_CONNECTED) {
        //  Serial.println("~");
        //  delay(300);
        //}
        delay(1000);

        WiFi.beginNetwork(wifi_name, wifi_password);
        while(WiFi.localIP() == INADDR_NONE) {
          Serial.println(".");
          delay(300);
        }
        myServer.begin();
        delay(1000);
    }
}

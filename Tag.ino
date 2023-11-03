#include <SPI.h>
#include <RH_RF95.h>
#include <Crypto.h>
#include <ChaCha.h>
#include <ArduinoJson.h>

// Singleton instance of radio driver
RH_RF95 rf95;
int led = 13;  // Define LED pin in case we want to use it to demonstrate activity
int MESSAGELENGTH = 36;
int SEQ;
int TAGID;
int TTL;
int thisRSSI;
int RELAYID = 0;  // Identity of tag. Set to 0 for the tag.
int RELAY;
int TYPE = 0;          // Message type
int DELAY = 1000;      // Time between transmissions (milliseconds)
double CSMATIME = 10;  // Check the status of the channel every 10 ms

// Chacha
ChaCha chacha;
uint8_t chachaKey[32] = "ThisIsASecretKey1234567890";  // 256-bit key
uint8_t chachaNonce[12] = "123456789012";              // 96-bit nonce

void setup() {
  pinMode(led, OUTPUT);
  Serial.begin(9600);
  Serial.println("Tag version 1");
  while (!Serial)
    Serial.println("Waiting for serial port");

  while (!rf95.init()) {
    Serial.println("Initialisation of LoRa sender failed");
    delay(1000);
  }

  if (rf95.init()) {
    Serial.println("Initialisation of LoRa sender successful");
  }

  rf95.setFrequency(915.0);
  rf95.setTxPower(5, false);
  rf95.setSignalBandwidth(500000);
  rf95.setSpreadingFactor(12);
}

void loop() {
  if (Serial.available() > 0) {
    String customMessage = Serial.readString();
    customMessage.trim();
    if (customMessage.length() > 0) {

      // Convert the customMessage string to a byte array
      uint8_t messageBytes[MESSAGELENGTH];
      uint8_t messageBytesOutput[MESSAGELENGTH];
      customMessage.getBytes(messageBytes, MESSAGELENGTH);

      // Set the ChaCha key and nonce
      chacha.setKey(chachaKey, 32);
      chacha.setIV(chachaNonce, 12);

      // Encrypt the message using ChaCha20
      chacha.encrypt(messageBytesOutput, messageBytes, MESSAGELENGTH);

      rf95.setModeIdle();
      while (rf95.isChannelActive()) {
        delay(CSMATIME);
        Serial.println("Tag node looping on isChannelActive()");
      }
      Serial.print("Transmitted message: ");
      Serial.println((char *)messageBytesOutput);
      rf95.send(messageBytesOutput, MESSAGELENGTH);
      rf95.waitPacketSent();
      delay(DELAY);
    }
  }
}

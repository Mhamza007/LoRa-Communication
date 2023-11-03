#include <SPI.h>
#include <RH_RF95.h>
#include <Crypto.h>
#include <ChaCha.h>
#include <ArduinoJson.h>

// Singleton instance of radio driver
RH_RF95 rf95;
int led = 13;  // Define LED pin in case you want to use it to demonstrate activity
int MESSAGELENGTH = 36;
int SEQ;
int TAGID;
int TTL;
int thisRSSI;
int RELAYID = 1;
int RELAY;
int TYPE = 1;          // Message type. All messages from a relay are type 1.
int DELAY = 1000;      // Time between transmissions (milliseconds)
double CSMATIME = 10;  // Check the status of the channel every 10 ms

// ChaCha
ChaCha chacha;
uint8_t chachaKey[32] = "ThisIsASecretKey1234567890";  // 256-bit key
uint8_t chachaNonce[12] = "123456789012";              // 96-bit nonce

void setup() {
  pinMode(led, OUTPUT);
  Serial.begin(9600);
  Serial.println("Receiver Version 1");

  while (!Serial)
    Serial.println("Waiting for the serial port");
  while (!rf95.init()) {
    Serial.println("Initialization of LoRa receiver failed");
    delay(1000);
  }
  if (rf95.init()) {
    Serial.println("Initialization of LoRa receiver successful");
  }
  rf95.setFrequency(915.0);
  rf95.setTxPower(23, false);
  rf95.setSignalBandwidth(500000);
  rf95.setSpreadingFactor(12);
}

void loop() {
  uint8_t buf[MESSAGELENGTH];
  uint8_t bufOut[MESSAGELENGTH];
  uint8_t len = sizeof(buf);
  if (rf95.available()) {
    if (rf95.recv(buf, &len)) {

      // Set the ChaCha key and nonce
      chacha.setKey(chachaKey, 32);
      chacha.setIV(chachaNonce, 12);

      // Decrypt the received message using ChaCha20
      chacha.decrypt(bufOut, buf, MESSAGELENGTH);

      // Convert the decrypted byte array to a string
      String decryptedMessage = String((char*)bufOut);

      StaticJsonDocument<200> jsonDoc;
      DeserializationError error = deserializeJson(jsonDoc, decryptedMessage);
      if (error) {
        // Serial.print(F("Parsing failed: "));
        // Serial.println(error.c_str());
        Serial.println(decryptedMessage);
        return;
      }

      const char* name = jsonDoc["user"];
      const char* message = jsonDoc["message"];
      String msg = String(name) + ": " + String(message);

      Serial.print("Received message from ");
      Serial.println(msg);

    } else {
      Serial.println("recv failed");
    }
  }
}

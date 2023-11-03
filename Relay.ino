/////
//
// Relay code. Listens for TAG messages, formats them, and retransmits them.
// Also forwards messages from other relays. 
// Message format is 
//  SEQ       %5d
//  TYPE (0,1,9)  %5d     //a 0 from a tag is changed to a 1 (from a relay). a 9 is a reset counters
//  TAGID     %5d
//  RELAYID   %5d
//  TTL       %5d
//  RSSI      %5d
// 
/////



#include <SPI.h>
#include <RH_RF95.h>

//Singleton instance of radio driver
RH_RF95 rf95;
int MESSAGELENGTH = 36;
int led = 13;   // Define LED pin in case we want to use it to demonstrate activity
int SEQ;
int TAGID;
int TTL;
int thisRSSI;
int RELAYID = 1;  // Identity of Relay. Will eventually be read from an SD card.
int RELAY;
int TYPE = 1;   // Message type. All messages from a relay are type 1.
int DELAY = 1000; // Time between transmissions (milliseconds)
int MAXSEQ = 0; // Highest sequence number seen so far (assumes only one tag)
double CSMATIME = 10;  // Check the status of the channel every 10 ms

void setup() {
  pinMode(led, OUTPUT);
  Serial.begin(9600); 
  Serial.println("Relay Version 1");
  while (!Serial)
    Serial.println("Waiting for serial port");  //Wait for serial port to be available.
  while (!rf95.init())
  {
    Serial.println("Initialisation of LoRa relay failed");
    delay(1000);
  }
  if (rf95.init()) {
    Serial.println("Initialisation of LoRa relay seccessful");
  }
  rf95.setFrequency(915.0);   
  rf95.setTxPower(0, false);
  rf95.setSignalBandwidth(500000);
  rf95.setSpreadingFactor(12);
}

void loop() {
  uint8_t buf[MESSAGELENGTH];
  uint8_t len = sizeof(buf);
  if (rf95.available())
  {
    // Should be a message for us now   
    if (rf95.recv(buf, &len))
    {
      Serial.print("Received message : "); Serial.println((char*)buf);  //DEBUG
      digitalWrite(led, HIGH);
    }
    else
    {
      Serial.println("recv failed");
    }
  
    // Wait an exponentially distributed amount of time before transmitting (mean DELAY)
    int WAITTIME = round(-log(1 - (double)random(1,9999)/10000)*DELAY);
    delay(WAITTIME);
  
    // Unpack message
    char str[MESSAGELENGTH];
    for (int i=0; i < MESSAGELENGTH; i++)
      str[i] = buf[i];
      
    // Now extract subfields 
    sscanf(str, "%5d %5d %5d %5d %5d %5d", &SEQ, &TYPE, &TAGID, &RELAY, &TTL, &thisRSSI);
    if (TYPE == 0) //Need to fill in fields
    {
      TYPE = 1;
      RELAY = RELAYID;
      thisRSSI = rf95.lastRssi();
    }
    TTL--;    //decrement TTL
    if (((TTL > 0) && (SEQ > MAXSEQ)) || ((TTL > 0) && (TYPE == 9)))  // Decide whether to transmit or not
    {
        if (TYPE == 9)
          MAXSEQ = 0; //Reset counter
        else
          MAXSEQ = SEQ;
        
      sprintf(str, "%5d %5d %5d %5d %5d %5d", SEQ, TYPE, TAGID, RELAY, TTL, thisRSSI);
      for (int i=0; i < MESSAGELENGTH; i++)
        buf[i] = str[i];  
      rf95.setModeIdle(); // some obscure bug causing loss of every second message  
      // Channel should be idle but if not wait for it to go idle
      while (rf95.isChannelActive())
      {
        delay(CSMATIME);   // wait for channel to go idle by checking frequently
        Serial.println("relay node looping on isChannelActive()"); //DEBUG
      }
      // Transmit message
      Serial.print("Forwarded message: ");  //DEBUG
      Serial.println((char*)buf);           //DEBUG
      digitalWrite(led, HIGH);
      rf95.send(str, sizeof(buf));
      rf95.waitPacketSent();
    } 
    digitalWrite(led, LOW);
  }
}

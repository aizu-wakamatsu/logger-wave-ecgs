/*
logger-wave-ecgs.ino
s1250039
*/

/*
 Udp NTP Client

 Get the time from a Network Time Protocol (NTP) time server
 Demonstrates use of UDP sendPacket and ReceivePacket
 For more on NTP time servers and the messages needed to communicate with them,
 see https://en.wikipedia.org/wiki/Network_Time_Protocol

 created 4 Sep 2010
 by Michael Margolis
 modified 9 Apr 2012
 by Tom Igoe
 modified 02 Sept 2015
 by Arturo Guadalupi

 This code is in the public domain.

 */

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SD.h>
#include "config.h"
#include "addrmacs.h"

//HARDWARE SPECIFIC, DO NOT CHANGE!!
#define PIN_EL A0  // HR input pin
#define PIN_CS 4  // CS at eth shield
// MAC Address is defined in "config.h".

const int NTP_PACKET_SIZE = 48;      // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[NTP_PACKET_SIZE];  //buffer to hold incoming and outgoing packets
// A UDP instance to let us send and receive packets over UDP
EthernetUDP Udp;

unsigned long timeunixs;
unsigned long timeoffsets;


int readHR(void) {

  int val_raw;
  val_raw = analogRead(PIN_EL);
}

// from L. 56-70 at UdpNtpClient
void beginEth(void) {
  // start Ethernet and UDP
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // Check for Ethernet hardware present
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    } else if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet cable is not connected.");
    }
    // no point in carrying on, so do nothing forevermore:
    while (true) {
      delay(1);
    }
  }
  Udp.begin(localPort);
}

// send an NTP request to the time server at the given address
void sendNTPpacket(const char* address) {
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;  // LI, Version, Mode
  packetBuffer[1] = 0;           // Stratum, or type of clock
  packetBuffer[2] = 6;           // Polling Interval
  packetBuffer[3] = 0xEC;        // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123);  // NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

void getTimeNTP(void) {
  sendNTPpacket(timeServer);  // send an NTP packet to a time server
  // wait to see if a reply is available
  delay(1000);
  if (Udp.parsePacket()) {
    // We've received a packet, read the data from it
    Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read the packet into the buffer
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long timentps = highWord << 16 | lowWord;
    Serial.print("Seconds since Jan 1 1900 = ");
    Serial.println(timentps);
    // now convert NTP time into everyday time:
    Serial.print("Unix time = ");
    // 1970-01-01
    timeunixs = timentps - 2208988800UL;
    // print Unix time:
    Serial.println(timeunixs);
    timeoffsets = millis();
    Ethernet.maintain();
  }
}

unsigned long getTimeNow(void) {
  return timeunixs * 1000 + millis() - timeoffsets;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  if (!SD.begin(PIN_CS)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    while (1);
  }
  // ntp
  beginEth();
  getTimeNTP();
  //タイムスタンプを元にファウ生成
}

void loop() {
  // put your main code here, to run repeatedly:
  static File dataFile = SD.open("data.csv", FILE_WRITE);
  unsigned long time = getTimeNow();
  int data = readHR();
  Serial.print(time);
  Serial.print(',');
  Serial.println(data);
  dataFile.print(time);
  dataFile.print(',');
  dataFile.println(data);
  delay(1000 / rate_samples);
  if (getTimeNow() - timeunixs * 1000 > 10000) {
    dataFile.close();
    while (1) {
    }
  }
}
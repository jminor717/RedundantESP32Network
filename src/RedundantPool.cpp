#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h> // UDP library from: bjoern@cs.stanford.edu 12/30/2008

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
//byte mac[] = {
 //   0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
//IPAddress ip(192, 168, 0, 177);
//IPAddress gateway(192, 168, 0, 1);
//IPAddress subnet(255, 255, 255, 0);

unsigned int UDPport = 5005;           // local port to listen for UDP packets
IPAddress UDPServer(192, 168, 0, 255); // destination device server

// buffers for receiving and sending data
//buffer to hold incoming packet,
char ReplyBuffer[] = "acknowledged"; // a string to send back

// An EthernetUDP instance to let us send and receive packets over UDP
EthernetUDP Udp;

const int UDP_PACKET_SIZE = 64;

byte packetBuffer[UDP_PACKET_SIZE] = "0";

unsigned int noChange = 0;

// A UDP instance to let us send and receive packets over UDP

unsigned long currentTime;
unsigned long secondTime;

unsigned long msPerSecond = 100UL;

int UDPCount = 0;
boolean packetOut = false;

unsigned int udpCount = 0;

// send an NTP request to the time server at the given address
void sendUDPpacket(IPAddress &address)
{
    if (packetOut)
        Serial.println("Missed packet");

    udpCount++;

    // set all bytes in the buffer to 0
    memset(packetBuffer, 0, UDP_PACKET_SIZE);

    sprintf((char *)packetBuffer, "Arduino count %u", udpCount);

    if (Udp.beginPacket(address, UDPport) == 0)
    {
        Serial.println(F("BeginPacket fail"));
        return;
    }

    Udp.write(packetBuffer, UDP_PACKET_SIZE);

    if (Udp.endPacket() == 0)
    {
        Serial.println(F("endPacket fail"));
        return;
    }

    packetOut = true;
}

void getUDPpacket()
{
    if (Udp.parsePacket())
    {
        // We've received a packet, read the data from it

        if (Udp.remoteIP() == UDPServer)
        {
            Serial.print(F("UDP IP OK  "));
        }
        else
        {
            Serial.print(F("UDP IP Bad "));
            Serial.println(Udp.remoteIP());
            //      return;
        }
        if (Udp.remotePort() == UDPport)
        {
            Serial.println(F("Port OK"));
        }
        else
        {
            Serial.println(F("Port Bad"));
            return;
        }

        Udp.read(packetBuffer, UDP_PACKET_SIZE); // read the packet into the buffer

        Serial.print(F("Received: "));
        Serial.println((char *)packetBuffer);

        packetOut = false;
    }
}
/*
void setup()
{
    currentTime = millis();
    secondTime = currentTime;
}*/

void CheckUDPServer()
{
    // if there's data available, read a packet
    int packetSize = Udp.parsePacket();
    if (packetSize)
    {
        char packetBuffer[packetSize] = {0};
        Serial.print("Received packet of size ");
        Serial.println(packetSize);
        Serial.print("From ");
        IPAddress remote = Udp.remoteIP();
        for (int i = 0; i < 4; i++)
        {
            Serial.print(remote[i], DEC);
            if (i < 3)
            {
                Serial.print(".");
            }
        }
        Serial.print(", port ");
        Serial.println(Udp.remotePort());

        // read the packet into packetBufffer
        Udp.read(packetBuffer, packetSize);
        Serial.println("Contents:");
        for (size_t i = 0; i < packetSize; i++)
        {
            /* code */
            Serial.print(packetBuffer[i]);
        }
        Serial.println("");

        // send a reply, to the IP address and port that sent us the packet we received
        Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
        Udp.write(ReplyBuffer);
        Udp.endPacket();
    }
}

void sendUDPBroadcast()
{
    //currentTime = millis();
    getUDPpacket();
    //
    //if (currentTime - secondTime > msPerSecond)
    //{
    byte rtnVal = Ethernet.maintain();
    switch (rtnVal)
    {
    case 1:
        Serial.println(F("\r\nDHCP renew fail"));
        break;
    case 2:
        Serial.println(F("\r\nDHCP renew ok"));
        break;
    case 3:
        Serial.println(F("\r\nDHCP rebind fail"));
        break;
    case 4:
        Serial.println(F("\r\nDHCP rebind ok"));
        break;
    }

    Serial.println(F("\r\nUDP send"));
    sendUDPpacket(UDPServer); // send an NTP packet to a time server
    packetOut = true;
    //    secondTime += msPerSecond;
    //}
}
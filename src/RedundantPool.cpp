#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h> // UDP library from: bjoern@cs.stanford.edu 12/30/2008

// The IP address will be dependent on your local network:
uint16_t UDPport = 8888;                 // local port to listen for UDP packets
IPAddress UDPServer(169, 254, 255, 255); // destination device server

// An EthernetUDP instance to let us send and receive packets over UDP
EthernetUDP Udp;
const int UDP_PACKET_SIZE = 64;
byte packetBuffer[UDP_PACKET_SIZE] = "0";

void UDPSetup()
{
    Udp.begin(UDPport);
}

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
    }
}

void sendUDPBroadcast()
{
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
    Udp.beginPacket(UDPServer, UDPport);
    String s = Ethernet.localIP().toString();
    size_t len = s.length();
    char data[len+1] = {0};
    for (int i = 0; i < len; i++)
    {
        data[i] = s.charAt(i);
    }
    Udp.write(data);
    Udp.write("::ESP32");
    Udp.endPacket();
}
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h> // UDP library from: bjoern@cs.stanford.edu 12/30/2008
#include <RedundantPool.hpp>

// The IP address will be dependent on your local network:

const int UDP_PACKET_SIZE = 64;
byte packetBuffer[UDP_PACKET_SIZE] = "0";

PoolManagment::PoolManagment(IPAddress selfadr, IPAddress adr, uint16_t port) //
{
    this->self.address = selfadr;
    this->UDPServer = adr;
    this->UDPport = port;
}

void PoolManagment::UDPSetup()
{
    this->Udp.begin(this->UDPport);
}

void PoolManagment::CheckUDPServer()
{
    // if there's data available, read a packet
    int packetSize = this->Udp.parsePacket();
    if (packetSize)
    {
        char packetBuffer[packetSize] = {0};
        Serial.print("Received packet of size ");
        Serial.println(packetSize);
        Serial.print("From ");
        IPAddress remote = this->Udp.remoteIP();
        for (int i = 0; i < 4; i++)
        {
            Serial.print(remote[i], DEC);
            if (i < 3)
            {
                Serial.print(".");
            }
        }
        Serial.print(", port ");
        Serial.println(this->Udp.remotePort());

        // read the packet into packetBufffer
        this->Udp.read(packetBuffer, packetSize);
        Serial.println("Contents:");
        for (size_t i = 0; i < packetSize; i++)
        {
            /* code */
            Serial.print(packetBuffer[i]);
        }
        Serial.println("");
    }
}

void PoolManagment::sendUDPBroadcast()
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
    this->Udp.beginPacket(this->UDPServer, this->UDPport);
    String s = Ethernet.localIP().toString();
    size_t len = s.length();
    char data[len + 1] = {0};
    for (int i = 0; i < len; i++)
    {
        data[i] = s.charAt(i);
    }
    this->Udp.write(data);
    this->Udp.write("::ESP32");
    this->Udp.endPacket();
}
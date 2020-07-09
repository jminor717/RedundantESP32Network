#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h> // UDP library from: bjoern@cs.stanford.edu 12/30/2008
#include <RedundantPool.hpp>

EthernetClient ethClient;
atomic<bool> PoolManagment::broadcastIP(true);
TimerHandle_t stopBZoradcasthanndle;
long lastReconnectAttempt = 0;
TickType_t continueBrodcastingUntill = pdMS_TO_TICKS(5000);

void callback3333(TimerHandle_t xTimer)
{
    PoolManagment::broadcastIP = false;

}

// The IP address will be dependent on your local network:

const int UDP_PACKET_SIZE = 64;
byte packetBuffer[UDP_PACKET_SIZE] = "0";

PoolDevice::PoolDevice(IPAddress addd)
{
    this->address = addd;
    this->isMaster = false;
}

PoolDevice::PoolDevice()
{
    this->isMaster = false;
}

PoolManagment::PoolManagment(IPAddress selfadr, IPAddress adr, uint16_t port) //
{
    this->self.address = selfadr;
    this->UDPBroadcastAddress = adr;
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
        IPAddress remote = this->Udp.remoteIP();

        // read the packet into packetBufffer
        this->Udp.read(packetBuffer, packetSize);
        //Serial.println("Contents:");
        uint8_t sourceLenght = 0;
        uint8_t seperatorLocation = 0;
        for (size_t i = 0; i < packetSize; i++)
        {
            if (packetBuffer[i] == 17)
                seperatorLocation++;
            if (seperatorLocation == 0)
            {
                sourceLenght++;
            }
        }
        char source[sourceLenght] = {0};
        for (size_t i = 0; i < sourceLenght; i++)
        {
            source[i] = (char)packetBuffer[i];
            //Serial.print(source[i]);
        }
        
        int dif = strcmp(source, "ESP32");
        bool formPool = false;
        if (dif >= sourceLenght || dif <= -sourceLenght){
            formPool = true;
            Serial.println("formPool");
        }

        if (formPool)
        {
            bool seen = false;
            for (size_t i = 0; i < this->pool.size(); i++)
            {
                if (remote == this->pool[i]->address)
                {
                    seen = true;
                    if (stopBZoradcasthanndle == NULL)//if the stop timer has been started it will not be null
                    {
                        Serial.print("SEEN____________________________________________________________________  ");
                        Serial.println(this->pool[i]->address.toString());
                        stopBZoradcasthanndle = xTimerCreate("StopBrodcasting", continueBrodcastingUntill, pdFALSE, (void *)99, callback3333);
                        //create a single fire timer that will stop the udp broadcasting after continueBrodcastingUntill tics
                        if (xTimerStart(stopBZoradcasthanndle, 0) != pdPASS)
                        {
                            stopBZoradcasthanndle = NULL;
                            // The timer could not be set into the Active state.
                        }
                    }
                }
            }
            if (seen)
                return;
        }
        this->pool.push_back(new PoolDevice(remote));
        //add additional pool fomation logic could also be usefull to but it in callback3333 to do this after a delay

        Serial.print("Received packet of size ");
        Serial.println(packetSize);
        Serial.print("From ");
        Serial.print(remote.toString());
        Serial.print(", port ");
        Serial.println(this->Udp.remotePort());
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
    this->Udp.beginPacket(this->UDPBroadcastAddress, this->UDPport);
    String s = Ethernet.localIP().toString();
    size_t len = s.length();
    char data[len + 1] = {0};
    for (int i = 0; i < len; i++)
    {
        data[i] = s.charAt(i);
    }
    this->Udp.write("ESP32");
    this->Udp.write(17);
    this->Udp.write(data);
    this->Udp.endPacket();
}

void PoolManagment::broadcastMessage(char *data)
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
    this->Udp.beginPacket(this->UDPBroadcastAddress, this->UDPport);
    this->Udp.write("DBG");
    this->Udp.write(17);
    this->Udp.write(data);
    this->Udp.endPacket();
}
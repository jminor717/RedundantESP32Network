#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h> // UDP library from: bjoern@cs.stanford.edu 12/30/2008
#include <RedundantPool.hpp>
#include <myConfig.hpp>

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
    this->Address = addd;
    this->IsMaster = false;
}

PoolDevice::PoolDevice()
{
    this->IsMaster = false;
}

PoolManagment::PoolManagment(IPAddress selfadr, IPAddress adr, uint16_t port) //
{
    this->self.Address = selfadr;
    this->self.Health = 50;
    this->self.RandomFactor = esp_random();
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
            if (packetBuffer[i] == 17) //use ascii device controll 1 a seperator
                seperatorLocation++;
            if (seperatorLocation == 0) //section before the first seperator
                sourceLenght++;
        }
        char source[sourceLenght] = {0};
        for (size_t i = 0; i < sourceLenght; i++)
        {
            source[i] = (char)packetBuffer[i];
            //Serial.print(source[i]);
        }

        int dif = strcmp(source, "ESP32"); //strcmp outputs position of the first differnece between the 2 strings
        bool formPool = false;
        if (dif >= sourceLenght || dif <= -sourceLenght) //if this position is larger than the length of the string they are equal
        {                                                //this will indicate a discovery broadcast from another ESP32
            formPool = true;
            Serial.println("formPool");
        }

        if (formPool)
        {
            bool seen = false;
            for (size_t i = 0; i < this->pool.size(); i++)
            {
                if (remote == this->pool[i]->Address)
                {
                    seen = true;
                    if (stopBZoradcasthanndle == NULL) //if the stop timer has been started it will not be null
                    {
                        Serial.print("SEEN____________________________________________________________________  ");
                        Serial.println(this->pool[i]->Address.toString());
                        stopBZoradcasthanndle = xTimerCreate("StopBrodcasting", continueBrodcastingUntill, pdFALSE, (void *)99, callback3333);
                        //create a single fire timer that will stop the udp broadcasting after continueBrodcastingUntill tics
                        if (xTimerStart(stopBZoradcasthanndle, 0) != pdPASS)
                        {
                            stopBZoradcasthanndle = NULL;
                            // The timer could not be set into the Active state.
                        }
                    }
                }
                if (seen)
                    return;
            }
        }
        this->pool.push_back(new PoolDevice(remote));
        //add additional pool fomation logic could also be usefull to but it in callback3333 to do this after a delay
        uint8_t reply[14];

        Serial.println(this->self.Transmit_Prep(reply));
        EthernetClient sendClient;
        if (sendClient.connect(remote, TCPPORT))
        {
            for (size_t i = 0; i < 14; i++)
            {
                sendClient.write((char)reply[i]);
            }
            Serial.println("TCP connected");
        }
        else
        {
            Serial.print("TCP connection failed with ");
            Serial.println(remote.toString());
        }

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

//ascii stops at 127
int PoolDevice::Transmit_Prep(uint8_t *data)
{
    int i = 0;
    data[i++] = this->OBJID; // code to use for identifing this object
    data[i++] = this->Address[0];
    data[i++] = this->Address[1];
    data[i++] = this->Address[2];
    data[i++] = this->Address[3];

    data[i++] = this->IsMaster;

    data[i++] = this->Health & 0x000000ff;
    data[i++] = (this->Health & 0x0000ff00) >> 8;
    data[i++] = (this->Health & 0x00ff0000) >> 16;
    data[i++] = (this->Health & 0xff000000) >> 24;

    data[i++] = this->RandomFactor & 0x000000ff;
    data[i++] = (this->RandomFactor & 0x0000ff00) >> 8;
    data[i++] = (this->RandomFactor & 0x00ff0000) >> 16;
    data[i++] = (this->RandomFactor & 0xff000000) >> 24;
    return i; //return data length 14
}

int PoolDevice::From_Transmition(uint8_t *data)
{
    int i = 0;
    this->Address[0] = data[++i];
    this->Address[1] = data[++i];
    this->Address[2] = data[++i];
    this->Address[3] = data[++i];

    this->IsMaster = (bool)data[++i];

    this->Health = data[++i];
    this->Health += data[++i] << 8;
    this->Health += data[++i] << 16;
    this->Health += data[++i] << 24;

    this->RandomFactor = data[++i];
    this->RandomFactor += data[++i] << 8;
    this->RandomFactor += data[++i] << 16;
    this->RandomFactor += data[++i] << 24;
    return ++i; //return data length 14
}
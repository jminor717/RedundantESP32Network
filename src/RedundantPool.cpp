#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h> // UDP library from: bjoern@cs.stanford.edu 12/30/2008
#include <RedundantPool.hpp>
#include <myConfig.hpp>
#include <EthernetHelp.hpp>
#include <cstring>
#include <version.h>

atomic<bool> PoolManagment::broadcastIP(true);
atomic<bool> PoolManagment::ControlRoomFound(false); //! change this to false once control room detection is finished
atomic<bool> PoolManagment::InUpdateMode(false);
atomic<bool> PoolManagment::CreateNewPool(false);
atomic<bool> PoolManagment::ContactedByPool(false);
TimerHandle_t stopBZoradcasthanndle;
TimerHandle_t createPool;
long lastReconnectAttempt = 0;
TickType_t continueBrodcastingUntill = pdMS_TO_TICKS(5000);

void stopBroadcastCallback(TimerHandle_t xTimer)
{
    PoolManagment::CreateNewPool = true;
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
    this->SelfStartingAddress = selfadr;
    this->self.Health = 50;
    this->self.RandomFactor = esp_random();
    this->self.version = VERSION;
    this->UDPBroadcastAddress = adr;
    this->UDPport = port;
}

void PoolManagment::UDPSetup()
{
    this->Udp.begin(this->UDPport);
}

void PoolManagment::stopBroadcasts(IPAddress Address)
{
    if (stopBZoradcasthanndle == NULL) //if the stop timer has been started it will not be null
    {
        Serial.print("SEEN____________________________________________________________________  ");
        Serial.println(Address.toString());
        stopBZoradcasthanndle = xTimerCreate("StopBrodcasting", continueBrodcastingUntill, pdFALSE, (void *)99, stopBroadcastCallback);
        //create a single fire timer that will stop the udp broadcasting after continueBrodcastingUntill tics
        if (xTimerStart(stopBZoradcasthanndle, 0) != pdPASS)
        {
            stopBZoradcasthanndle = NULL;
            // The timer could not be set into the Active state.
        }
    }
}

void PoolManagment::CheckUDPServer()
{
    // if there's data available, read a packet
    int packetSize = this->Udp.parsePacket();
    if (packetSize)
    {
        char packetBuffer[packetSize] = {0};
        IPAddress remote = this->Udp.remoteIP();
        char printbuffer[300];
        // read the packet into packetBufffer
        this->Udp.read(packetBuffer, packetSize);
        if (packetBuffer[0] == POOL_DEVICE_TRANSMIT_ID)
        {
            PoolManagment::ContactedByPool = true;
            PoolDevice dev1;
            //Serial.println("remote device found");
            uint8_t bytebuf[packetSize] = {0};
            for (size_t i = 0; i < packetSize; i++)
            {
                bytebuf[i] = packetBuffer[i];
            }

            dev1.From_Transmition(bytebuf, packetSize);
            sprintf(printbuffer, "remote address: %s   from %s ___helt: %u  __rf: %u   __isM: %u", dev1.Address.toString().c_str(), remote.toString().c_str(), dev1.Health, dev1.RandomFactor,dev1.IsMaster);
            Serial.println(printbuffer);
            bool found = false;
            PoolDevice *devout;
            for (size_t i = 0; i < this->pool.size(); i++)
            {
                PoolDevice *dev = this->pool.at(i);
                if (dev->Address == dev1.Address)
                {
                    dev->Health = dev1.Health;
                    dev->RandomFactor = dev1.RandomFactor;
                    dev->IsMaster = dev1.IsMaster;
                    dev->Vote = dev1.Vote;
                    devout = dev;
                    found = true;
                }
            }
            if (!found)
            {
                this->pool.push_back(new PoolDevice(remote));
                PoolDevice *dev = this->pool.back();
                dev->Address = dev1.Address;
                dev->Health = dev1.Health;
                dev->RandomFactor = dev1.RandomFactor;
                dev->IsMaster = dev1.IsMaster;
                dev->Vote = dev1.Vote;
                devout = dev;
                if (!PoolManagment::broadcastIP)
                {
                    this->sendUDPBroadcast();
                }
            }
            this->stopBroadcasts(devout->Address);
        }
        else
        {
            uint8_t sourceLenght = 0;
            uint8_t seperatorLocation = 0;
            uint8_t Section2Lenght = 0;
            uint8_t Section3Lenght = 0;
            for (size_t i = 0; i < packetSize; i++)
            {
                if (packetBuffer[i] == MESSAGE_DELIMINER_CODE_ASCII) //use ascii device controll 1 a seperator
                    seperatorLocation++;
                else if (seperatorLocation == 0) //section before the first seperator used to denote what type of device comand is comming from
                    sourceLenght++;              //eg. ESP32   or    controlroom
                else if (seperatorLocation == 1) //section after the first seperator used as a comand comming from the controll room
                    Section2Lenght++;
                else if (seperatorLocation == 2) //section after the second seperator value for this command
                    Section3Lenght++;            //!note this is not implamented in the controll room only in the nodejs file i made to interface with the esp32
            }
            String source;
            for (size_t i = 0; i < sourceLenght; i++)
            {
                source += (char)packetBuffer[i];
            }
            String comand;
            if (Section2Lenght > 0)
            {
                for (size_t i = 0; i < Section2Lenght; i++)
                {
                    comand += (char)packetBuffer[sourceLenght + 1 + i];
                    //Serial.print(comand[i]);
                }
            }
            String value; // new is neccary because static arays dont suport a length of 0 and cause a crash
            if (Section3Lenght > 0)
            {
                for (size_t i = 0; i < Section3Lenght; i++)
                {
                    value += (char)packetBuffer[sourceLenght + 1 + Section2Lenght + 1 + i];
                    //Serial.print(value[i]);
                }
            }

            if (source.equals("CONTROL"))
            { //this will be a broadcast from the controll room
                if (!ControlRoomFound)
                {
                    if (comand.equals("DISCOVER"))
                    {

                        PoolManagment::ControlRoomFound = true;
                        this->ControlRoomIP = remote;
                        Serial.print("control room found at ");
                        Serial.println(remote.toString());
                        this->SendPoolInfo(this->ControlRoomIP, false);
                    }
                }
                if (comand.equals("SETUPDATE"))
                {
                    Serial.print("control room set update mode to ");
                    if (value.equals("true"))
                    {
                        if (!PoolManagment::InUpdateMode)
                        {
                            PoolManagment::InUpdateMode = true;
                            Serial.println(value);
                            //StartWifi(*this);
                            IPAddress  wifiAdr = StartWifi();
                            sprintf(printbuffer, " Connected to %s     IP address: %s", WIFIssid, wifiAdr.toString().c_str());
                            //Serial.println(printbuffer);
                            this->broadcastMessage(printbuffer);
                        }
                    }
                    else
                    {
                        if (PoolManagment::InUpdateMode)
                        {
                            PoolManagment::InUpdateMode = false;
                            Serial.println(value);
                            endWifi();
                        }
                    }
                }
                return;
            }
        }
    }
}

void PoolManagment::SendPoolInfo(IPAddress remote, bool vote)
{
    int dataseize = this->self.Transmit_Size();
    uint8_t reply[dataseize];
    this->self.Transmit_Prep(reply, vote);
    EthernetClient sendClient;
    if (sendClient.connect(remote, TCP_PORT))
    {
        sendClient.write(reply, dataseize);
        Serial.print("TCP connected to remote    ");
        Serial.println(remote.toString());
        sendClient.flush();
        sendClient.stop();
    }
    else
    {
        Serial.print("TCP connection failed with ");
        Serial.println(remote.toString());
    }
}

void PoolManagment::FowardDataToControlRoom(uint8_t *data, size_t DataLength)
{
    EthernetClient sendClient;
    int x = sendClient.connect(this->ControlRoomIP, TCP_PORT);
    if (x)
    {
        delay(5);
        sendClient.write(data, DataLength);
        delay(5);
        sendClient.flush();
        sendClient.stop();
    }
    else
    {
        delay(5);
        sendClient.write(data, DataLength);
        delay(5);
        sendClient.flush();
        sendClient.stop();
        Serial.print("TCP NOTTTT connected  : ");
        Serial.print(sendClient.connected());
        Serial.print("   ");
        Serial.println(x);
    }
}

void PoolManagment::sendUDPBroadcast()
{
    this->Udp.beginPacket(this->UDPBroadcastAddress, this->UDPport);
    int dataseize = this->self.Transmit_Size();
    uint8_t reply[dataseize];
    this->self.Transmit_Prep(reply, false);

    this->Udp.write(reply, dataseize);
    this->Udp.endPacket();
    this->Udp.flush();
    /*
    this->Udp.beginPacket(this->UDPBroadcastAddress, this->UDPport);
    String s = Ethernet.localIP().toString();
    size_t len = s.length();
    char data[len + 1] = {0};
    for (int i = 0; i < len; i++)
    {
        data[i] = s.charAt(i);
    }
    this->Udp.write("ESP32");
    this->Udp.write(MESSAGE_DELIMINER_CODE_ASCII); //device control code 1 in ascii
    this->Udp.write(data);
    this->Udp.endPacket();
    this->Udp.flush();
    */
}

void PoolManagment::broadcastMessage(const char *data)
{
    if (PoolManagment::ControlRoomFound)
    {
        this->Udp.beginPacket(this->ControlRoomIP, this->UDPport);
        this->Udp.write("DBG");
        this->Udp.write(MESSAGE_DELIMINER_CODE_ASCII);
        this->Udp.write(data);
        this->Udp.endPacket();
    }
}

bool PoolManagment::createPool()
{

    if (!PoolManagment::ContactedByPool)
    {
        EthernetClient sendClient;
    //!    this->broadcastMessage("Creating Pool");
        for (size_t i = 0; i < this->pool.size(); i++)
        {
            //this->SendPoolInfo(this->pool.at(i)->Address);
            PoolDevice *dev = this->pool.at(i);
            int dataseize = this->self.Transmit_Size();
            uint8_t reply[dataseize];
            this->self.Transmit_Prep(reply, false);
            if (sendClient.connect(dev->Address, TCP_PORT))
            {
                dev->LastContact = esp_timer_get_time();
                String x = "TCP connected to esp32: ";
                x += dev->Address.toString();
            //!    this->broadcastMessage(x.c_str());
                sendClient.write(reply, dataseize);
                sendClient.flush();
                sendClient.stop();
            }
            else
            {
                Serial.print("TCP connection failed with esp32: ");
                Serial.println(dev->Address.toString());
            }
        }
    }
    return true;
}

//ascii stops at 127
int PoolDevice::Transmit_Prep(uint8_t *data, bool vote)
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

    data[i++] = vote;

    for (size_t j = 0; j < this->version.length(); j++)
    {
        data[i++] = (uint8_t)this->version.charAt(j);
    }
    return i; //return data length 15 + version length
}

int PoolDevice::Transmit_Size()
{
    int i = 15;
    i+=this->version.length();
    return i; // same as i in Transmit_Prep
}

int PoolDevice::From_Transmition(uint8_t *data, uint16_t datalength)
{
    int i = 0;
    this->Address[0] = data[++i];
    this->Address[1] = data[++i];
    this->Address[2] = data[++i];
    this->Address[3] = data[++i];

    this->IsMaster = (bool)data[++i];

    unsigned char dumbShit[4]{0};
    dumbShit[0] = data[++i];
    dumbShit[1] = data[++i];
    dumbShit[2] = data[++i];
    dumbShit[3] = data[++i];
    uint32_t value;
    std::memcpy(&value, dumbShit, sizeof(uint32_t));
    this->Health = value;

    dumbShit[0] = data[++i];
    dumbShit[1] = data[++i];
    dumbShit[2] = data[++i];
    dumbShit[3] = data[++i];
    value = 0;
    std::memcpy(&value, dumbShit, sizeof(uint32_t));
    this->RandomFactor = value;

    this->Vote = (bool)data[++i];
    int length = datalength - (i+1);
    for (size_t j = 0; j < length; j++)
    {
        this->version += (char)data[++i];
    }
    
     return ++i; //return data length 15 + version length
}
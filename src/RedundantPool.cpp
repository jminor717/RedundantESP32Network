#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h> // UDP library from: bjoern@cs.stanford.edu 12/30/2008
#include <RedundantPool.hpp>
#include <myConfig.hpp>
#include <EthernetHelp.hpp>

atomic<bool> PoolManagment::broadcastIP(true);
atomic<bool> PoolManagment::ControlRoomFound(false); //! change this to false once control room detection is finished
atomic<bool> PoolManagment::InUpdateMode(false);
atomic<bool> PoolManagment::CreateNewPool(false);
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
        if (source.equals("ESP32"))
        { //this will indicate a discovery broadcast from another ESP32
            Serial.println("formPool");
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
                        stopBZoradcasthanndle = xTimerCreate("StopBrodcasting", continueBrodcastingUntill, pdFALSE, (void *)99, stopBroadcastCallback);
                        //create a single fire timer that will stop the udp broadcasting after continueBrodcastingUntill tics
                        if (xTimerStart(stopBZoradcasthanndle, 0) != pdPASS)
                        {
                            stopBZoradcasthanndle = NULL;
                            // The timer could not be set into the Active state.
                        }
                    }
                }
                if (seen)
                {
                    return;
                }
            }
            this->pool.push_back(new PoolDevice(remote));
            this->SendPoolInfo(remote); //add additional pool fomation logic could also be usefull to but it in stopBroadcastCallback to do this after a delay
            return;
        }
        else if (source.equals("CONTROL"))
        { //this will be a broadcast from the controll room
            if (!ControlRoomFound)
            {
                if (comand.equals("DISCOVER"))
                {

                    PoolManagment::ControlRoomFound = true;
                    this->ControlRoomIP = remote;
                    Serial.print("control room found at ");
                    Serial.println(remote.toString());
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
                        StartWifi();
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

void PoolManagment::SendPoolInfo(IPAddress remote)
{
    int dataseize = this->self.Transmit_Size();
    uint8_t reply[dataseize];
    this->self.Transmit_Prep(reply);
    EthernetClient sendClient;
    if (sendClient.connect(remote, TCP_PORT))
    {
        for (size_t i = 0; i < dataseize; i++)
        {
            sendClient.write(reply[i]);
        }
        Serial.println("TCP connected");
        sendClient.flush();
        sendClient.stop();
    }
    else
    {
        Serial.print("TCP connection failed with ");
        Serial.println(remote.toString());
    }
}

void PoolManagment::sendUDPBroadcast()
{
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
}

void PoolManagment::broadcastMessage(char *data)
{
    this->Udp.beginPacket(this->UDPBroadcastAddress, this->UDPport);
    this->Udp.write("DBG");
    this->Udp.write(MESSAGE_DELIMINER_CODE_ASCII);
    this->Udp.write(data);
    this->Udp.endPacket();
}

bool PoolManagment::createPool()
{
    EthernetClient sendClient;

    for (size_t i = 0; i < pool.size(); i++)
    {
        /* code */
    }
    return true;
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

int PoolDevice::Transmit_Size()
{
    return 14; // same as i in Transmit_Prep
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
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h> // UDP library from: bjoern@cs.stanford.edu 12/30/2008
#include <RedundantPool.hpp>
#include <myConfig.hpp>

EthernetClient ethClient;
atomic<bool> PoolManagment::broadcastIP(true);
atomic<bool> PoolManagment::ControlRoomFound(false); //! change this to false once control room detection is finished
atomic<bool> PoolManagment::InUpdateMode(false);
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
        uint8_t Section2Lenght = 0;
        uint8_t Section3Lenght = 0;
        for (size_t i = 0; i < packetSize; i++)
        {
            if (packetBuffer[i] == 17) //use ascii device controll 1 a seperator
                seperatorLocation++;
            else if (seperatorLocation == 0) //section before the first seperator used to denote what type of device comand is comming from
                sourceLenght++;              //eg. ESP32   or    controlroom
            else if (seperatorLocation == 1) //section after the first seperator used as a comand comming from the controll room
                Section2Lenght++;
            else if (seperatorLocation == 2) //section after the second seperator value for this command
                Section3Lenght++;            //!note this is not implamented in the controll room only in the nodejs file i made to interface with the esp32
        }
        //char source[sourceLenght] = {0};
        char *source = new char[sourceLenght];
        for (size_t i = 0; i < sourceLenght; i++)
        {
            source[i] = (char)packetBuffer[i];
            //Serial.print(source[i]);
        }
        char *comand = new char[Section2Lenght];
        if (Section2Lenght > 0)
        {
            for (size_t i = 0; i < Section2Lenght; i++)
            {
                comand[i] = (char)packetBuffer[sourceLenght + 1 + i];
                //Serial.print(comand[i]);
            }
        }
        char *value = new char[Section3Lenght]; // new is neccary because static arays dont suport a length of 0 and cause a crash
        if (Section3Lenght > 0)
        {
            for (size_t i = 0; i < Section3Lenght; i++)
            {
                value[i] = (char)packetBuffer[sourceLenght + 1 + Section2Lenght + 1 + i];
                //Serial.print(value[i]);
            }
        }

        int dif = strcmp(source, "ESP32");               //strcmp outputs position of the first differnece between the 2 strings
        if (dif >= sourceLenght || dif <= -sourceLenght) //if this position is larger than the length of the string they are equal
        {                                                //this will indicate a discovery broadcast from another ESP32
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
            this->pool.push_back(new PoolDevice(remote));
            this->SendPoolInfo(remote);

        }
        //add additional pool fomation logic could also be usefull to but it in callback3333 to do this after a delay
        dif = strcmp(source, "CONTROL");                 //strcmp outputs position of the first differnece between the 2 strings
        if (dif >= sourceLenght || dif <= -sourceLenght) //if this position is larger than the length of the string they are equal
        {                                                //this will indicate a discovery broadcast from another ESP32
            if (!ControlRoomFound)
            {
                dif = strcmp(comand, "DISCOVER");
                if (dif >= Section2Lenght || dif <= -Section2Lenght)
                {
                    PoolManagment::ControlRoomFound = true;
                    this->ControlRoomIP = remote;
                    Serial.print("control room found at ");
                    Serial.println(remote.toString());
                }
            }
            dif = strcmp(comand, "SETUPDATE");
            if (dif >= Section2Lenght || dif <= -Section2Lenght)
            {
                Serial.print("control room set update mode to ");

                dif = strcmp(value, "true");
                if (dif >= Section3Lenght || dif <= -Section3Lenght)
                {
                    PoolManagment::InUpdateMode = true;
                    Serial.println("true");
                }
                else
                {
                    PoolManagment::InUpdateMode = false;
                    Serial.println("false");
                }
            }
            return;
        }
        /*
        Serial.print("Received packet of size ");
        Serial.println(packetSize);
        Serial.print("From ");
        Serial.print(remote.toString());
        Serial.print(", port ");
        Serial.println(this->Udp.remotePort());*/
    }
}

void PoolManagment::SendPoolInfo(IPAddress remote)
{
    int dataseize = this->self.Transmit_Size();
    for (size_t i = 0; i < this->pool.size(); i++)
    {
        dataseize += this->self.Transmit_Size(); //dont need to do this for the each pool device since there all the same
    }

    uint8_t reply[dataseize];
    this->self.Transmit_Prep(reply);
    for (size_t i = 0; i < this->pool.size(); i++)
    {
        this->pool.at(i)->Transmit_Prep(reply+ (this->self.Transmit_Size() * i));//transmit knoledge about all devices currently in the pool
    }
    
    EthernetClient sendClient;
    if (sendClient.connect(remote, TCPPORT))
    {
        for (size_t i = 0; i < dataseize; i++)
        {
            sendClient.write(reply[i]);
        }
        Serial.println("TCP connected");
    }
    else
    {
        Serial.print("TCP connection failed with ");
        Serial.println(remote.toString());
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
    this->Udp.write(17); //device control code 1 in ascii
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
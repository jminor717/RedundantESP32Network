/*  the libraries used have some errors that will need changes
    in Server.h change line below  
    virtual void begin(uint16_t port=0) =0;
    virtual void begin() = 0;
*/

/*
always keep a udp server running listening for broadcasts
and tcp server for direct connections
at startup broadcast udp to share this ip adress, until a tcp coection is made 
keep udp brodcasting for another 5 seconds to ensure all esp32 get the packets

vscode extension Better Comments
*  ss
?  ss
!  ss
// ss
TODO: implament watchdog reboot
TODO: https://medium.com/@supotsaeea/esp32-reboot-system-when-watchdog-timeout-4f3536bf17ef
@param param
*/
//#include <stdio.h>
//#include <freertos/FreeRTOS.h>
//#include <freertos/task.h>
//#include "sdkconfig.h"

#include <Arduino.h>
#include <Ethernet.h>
#include <EthernetHelp.hpp>
#include <spihelp.hpp>
#include <TempHelp.hpp>
#include <RedundantPool.hpp>
#include <myConfig.hpp>
#include <MyTimers.hpp>

//deffinitions
#define tempBufSize 32

bool isMaster = false;
bool poolActive = false;

//ethernet data
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress gateway(169, 254, 205, 1);
IPAddress subnet(255, 255, 0, 0);
//ethernet server
EthernetServer server(TCP_PORT);
PoolManagment fullpool(
    IPAddress(169, 254, 205, 47),
    IPAddress(169, 254, 255, 255),
    UDP_PORT);

//spi classes
SPIClass *PrimarySPI = NULL; // primary spi buss, used as the default for most libraries

ADXL345_SPI *accSPI_AZ;
ADXL345_SPI *accSPI_EL;
ADXL345_SPI *accSPI_Ballence;
ADXLbuffer acc1buffer(ACC_BUFFER_SIZE); //; //= ADXLbuffer(1600);
ADXLbuffer acc2buffer(ACC_BUFFER_SIZE);
ADXLbuffer acc3buffer(ACC_BUFFER_SIZE);

short AZTempBuf[tempBufSize];
uint16_t AZTempBufSize = 0;
short ELTempBuf[tempBufSize];
uint16_t ELTempBufSize = 0;

Tempsensor AZTempSense(AZ_Temp_Line);
Tempsensor ELTempSense(EL_Temp_Line);

void setup()
{
    Serial.begin(115200);

    startTimersNoPool();
    initilizeSpiBuss(); //needs to be called befor initilizing spi sensors
    accSPI_EL = new ADXL345_SPI(AdxlSS_AZ, AdxlInt_AZ);
    //accSPI_EL->init();

    accSPI_AZ = new ADXL345_SPI(AdxlSS_EL, AdxlInt_EL);
    //accSPI_AZ->init();

    accSPI_Ballence = new ADXL345_SPI(AdxlSS_Ballence, AdxlInt_Ballence);
    //accSPI_Ballence->init();
    accSPI_Ballence->spi_clock_speed = 200000;

    // initialize the spi bus used by Ethernet.h to have the pinnout we specify
    SPI.begin(PrimarySPI_SCLK, PrimarySPI_MISO, PrimarySPI_MOSI, PrimarySPI_SS);
    // You can use Ethernet.init(pin) to configure the CS pin
    ////SCLK = 18, MISO = 19, MOSI = 23, SS = 5
    Ethernet.init(PrimarySPI_SS);
    // initialize the ethernet device
    //Ethernet.begin(mac);
    // Ethernet.begin(mac, fullpool.SelfStartingAddress, gateway, gateway, subnet);
    //fullpool.self.Address = findOpenAdressAfterStart(mac, fullpool.SelfStartingAddress, gateway, subnet);
    fullpool.self.Address = findOpenAdressAfterStart();
    // Open serial communications and wait for port to open:



    // start listening for clients
    server.begin();
    fullpool.UDPSetup();
    Serial.print("ethernet server address:");
    Serial.println(Ethernet.localIP());
    Serial.print("ethernet server port:");
    Serial.println(TCP_PORT);
}
int tem = 0;
void loop()
{
    //char data2[30];
    if (CheckEthernet)
    {
        CheckEthernet = false;
        // wait for a new client:
        EthernetClient client = server.available();
        // when the client sends the first byte, say hello:
        if (client)
        {
            if (client.available())
            {
                size_t bytes = client.available();
                uint8_t *ptr;
                uint8_t data[bytes] = {0};
                ptr = data;
                client.read(ptr, bytes);
                //int len = processOneTimeConnection(client, bytes, data, Serial);
                //if (len == -1 || len == -2)
                // {

                char msg[bytes];

                if (data[0] == fullpool.self.OBJID)
                {
                    PoolDevice dev1;
                    Serial.println("remote device found");
                    dev1.From_Transmition(data);
                    Serial.print("remote device address: ");
                    Serial.println(dev1.Address.toString().c_str());
                    Serial.print("remote device health: ");
                    Serial.println(dev1.Health);
                    Serial.print("remote device random factor: ");
                    Serial.println(dev1.RandomFactor);
                    bool found = false;
                    for (size_t i = 0; i < fullpool.pool.size(); i++)
                    {
                        PoolDevice *dev = fullpool.pool.at(i);
                        if (dev->Address == dev1.Address)
                        {
                            dev->Health = dev1.Health;
                            dev->RandomFactor = dev1.RandomFactor;
                            dev->IsMaster = dev1.IsMaster.load();
                            found = true;
                        }
                    }
                    if (!found)
                    {
                        fullpool.pool.push_back(new PoolDevice());
                        PoolDevice *dev = fullpool.pool.back();
                        dev->Health = dev1.Health;
                        dev->RandomFactor = dev1.RandomFactor;
                        dev->IsMaster = dev1.IsMaster.load();
                    }
                }
                else
                {
                    /* for (int j = 0; j < bytes; j++)
                    {
                        msg[j] = (char)data[j];
                        Serial.print(data[j]);
                        Serial.print("  ");
                        Serial.println(msg[j]);
                    }*/
                    Serial.print("message is bytes long");
                    Serial.println(bytes);
                }

                client.write("acknoledge");
                // give time to receive the data
                delay(2);
                // close the connection:
                client.stop();
                //  }
            }
        }
    }

    if (accSPI_AZ->bufferFull)
    {
        accSPI_AZ->bufferFull = false;
        // Serial.print("AZ_");
        accbuffer buffer = emptyAdxlBuffer((SPI_DEVICE)*accSPI_AZ);
        for (size_t i = 0; i < buffer.lenght; i++)
        {
            acc1buffer.buffer.push(buffer.buffer[i]);
        }
        // sprintf(data2, "%d, %d, %d,    1    %d", acc1buffer.buffer.front().x, acc1buffer.buffer.front().y, acc1buffer.buffer.front().z, acc1buffer.buffer.size());
        // Serial.println(data2);
        //acc1buffer.buffer.pop();
    }
    if (accSPI_EL->bufferFull)
    {
        accSPI_EL->bufferFull = false;
        accbuffer buffer = emptyAdxlBuffer((SPI_DEVICE)*accSPI_EL);
        for (size_t i = 0; i < buffer.lenght; i++)
        {
            acc2buffer.buffer.push(buffer.buffer[i]);
        }
    }
    if (accSPI_Ballence->bufferFull)
    {
        accSPI_Ballence->bufferFull = false;
        accbuffer buffer = emptyAdxlBuffer((SPI_DEVICE)*accSPI_Ballence);
        for (size_t i = 0; i < buffer.lenght; i++)
        {
            acc3buffer.buffer.push(buffer.buffer[i]);
        }
    }

    if (measureAZTemp)
    {
        measureAZTemp = false;
        if (AZTempBufSize >= tempBufSize - 2)
        {
            AZTempBuf[AZTempBufSize] = AZTempSense.read();
        }
        else
        {
            AZTempBuf[AZTempBufSize++] = AZTempSense.read();
        }
        //sprintf(data2, "AZ: %d", AZTempSense.read());
        //Serial.print(data2);
    }
    if (measureELTemp)
    {
        measureELTemp = false;
        if (ELTempBufSize >= tempBufSize - 2)
        {
            ELTempBuf[ELTempBufSize] = ELTempSense.read();
        }
        else
        {
            ELTempBuf[ELTempBufSize++] = ELTempSense.read();
        }
    }
    if (CheckUDP)
    {
        CheckUDP = false;
        fullpool.CheckUDPServer();
    }
    if (BroadcastUDP)
    {
        BroadcastUDP = false;
        fullpool.sendUDPBroadcast();
        if (PoolManagment::CreateNewPool) //we only want to do this once and we can do it after we stop UDP broadcasts
        {
            PoolManagment::CreateNewPool = false;
        }
    }
    if (SendDataToControlRoom)
    {
        SendDataToControlRoom = false;
        uint32_t dataSize = calcTransitSize(&acc1buffer, &acc2buffer, &acc3buffer, AZTempBufSize, ELTempBufSize); // determine the size of the array that needs to be alocated
        uint8_t *dataToSend;
        /*
        uint32_t heapspace = xPortGetFreeHeapSize();
        uint32_t freeblock = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
        Serial.println(heapspace);
        Serial.println(freeblock);
        Serial.println(dataSize); */
        dataToSend = (uint8_t *)malloc(dataSize * sizeof(uint8_t)); //malloc needs to be used becaus stack size on the loop task is about 4k so this needs to go on the heap
        prepairTransit(dataToSend, dataSize, &acc1buffer, &acc2buffer, &acc3buffer, AZTempBuf, AZTempBufSize, ELTempBuf, ELTempBufSize);
        AZTempBufSize = 0;
        ELTempBufSize = 0;
        FowardDataToControlRoom(dataToSend, dataSize, fullpool.ControlRoomIP, TCP_PORT);
        // this includes a flush and can be a very lengthy process the ethernet coms should at some point be put on their own thread
        free(dataToSend);
    }
    if (PoolManagment::InUpdateMode)
    {
        handleWifiClient();
    }
}

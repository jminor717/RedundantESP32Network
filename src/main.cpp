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
int votesForMaster = 0;

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
SPIClass *PrimarySPI = NULL;   // primary spi buss, used as the default for most libraries
SPIClass *SecondarySPI = NULL; // secondary, mostly unused by library code so free to use for this code

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

    //initilizeSpiBuss(); //needs to be called befor initilizing spi sensors
    PrimarySPI = new SPIClass(HSPI);
    // initialize the spi bus used by Ethernet.h to have the pinnout we specify
    SPI.begin(PrimarySPI_SCLK, PrimarySPI_MISO, PrimarySPI_MOSI, PrimarySPI_SS);
    // You can use Ethernet.init(pin) to configure the CS pin
    ////SCLK = 18, MISO = 19, MOSI = 23, SS = 5
    Ethernet.init(PrimarySPI_SS);
    // initialize the ethernet device
    Serial.println("starting...");
    //Ethernet.begin(mac);
    // Ethernet.begin(mac, fullpool.SelfStartingAddress, gateway, gateway, subnet);
    fullpool.self.Address = findOpenAdressAfterStart(fullpool.SelfStartingAddress, gateway, subnet);
    //fullpool.self.Address = findOpenAdressAfterStart();
    // Open serial communications and wait for port to open:
    Serial.print("ethernet server address: ");
    Serial.println(fullpool.self.Address.toString());
    Serial.print("ethernet server port:");
    Serial.println(TCP_PORT);

    startTimersNoPool();
    // start listening for clients
    server.begin();
    fullpool.UDPSetup();
}
int tem = 0;
void loop()
{

    char data2[30];
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
                //char printbuffer[100];
                size_t bytes = client.available();
                uint8_t *ptr;
                uint8_t data[bytes] = {0};
                ptr = data;
                client.read(ptr, bytes);
                //int len = processOneTimeConnection(client, bytes, data, Serial);
                //if (len == -1 || len == -2)
                // {
                //client.
                //char msg[bytes];
                /* for (int j = 0; j < bytes; j++)
                {
                    msg[j] = (char)data[j];
                    Serial.print(data[j]);
                    Serial.print("  ");
                    Serial.println(msg[j]);
                }*/
                Serial.print("message is bytes long");
                Serial.println(bytes);

                //client.write("acknoledge");
                // give time to receive the data
                //delay(2);
                // close the connection:
                //client.stop();
                //  }
            }
        }
    }
    if (fullpool.self.IsMaster)
    {
        if (accSPI_AZ->bufferFull)
        {
            Serial.print("A");
            accSPI_AZ->bufferFull = false;
            accbuffer buffer = emptyAdxlBuffer((SPI_DEVICE)*accSPI_AZ);
            for (size_t i = 0; i < buffer.lenght; i++)
            {
                acc1buffer.buffer.push(buffer.buffer[i]);
            }
            accSPI_AZ->LastEmptyBuffer = esp_timer_get_time();
        }
        if (accSPI_EL->bufferFull)
        {
            Serial.print("E");
            accSPI_EL->bufferFull = false;
            accbuffer buffer = emptyAdxlBuffer((SPI_DEVICE)*accSPI_EL);
            for (size_t i = 0; i < buffer.lenght; i++)
            {
                acc2buffer.buffer.push(buffer.buffer[i]);
            }
            accSPI_EL->LastEmptyBuffer = esp_timer_get_time();
        }
        if (accSPI_Ballence->bufferFull)
        {
            Serial.print("B");
            accSPI_Ballence->bufferFull = false;
            accbuffer buffer = emptyAdxlBuffer((SPI_DEVICE)*accSPI_Ballence);
            for (size_t i = 0; i < buffer.lenght; i++)
            {
                acc3buffer.buffer.push(buffer.buffer[i]);
            }
            accSPI_Ballence->LastEmptyBuffer = esp_timer_get_time();
        }
    }
    if (measureAZTemp)
    {
        measureAZTemp = false;
        int16_t sssss = AZTempSense.read();
        if (AZTempBufSize >= tempBufSize - 2)
        {
            AZTempBuf[AZTempBufSize] = sssss;
        }
        else
        {
            AZTempBuf[AZTempBufSize++] = sssss;
        }
        sprintf(data2, "AZ: %d", sssss);
        //!    fullpool.broadcastMessage(data2);
    }
    if (measureELTemp)
    {
        measureELTemp = false;
        int16_t sssss = ELTempSense.read();
        if (ELTempBufSize >= tempBufSize - 2)
        {
            ELTempBuf[ELTempBufSize] = sssss;
        }
        else
        {
            ELTempBuf[ELTempBufSize++] = sssss;
        }
        sprintf(data2, "EL: %d", sssss);
        //!    fullpool.broadcastMessage(data2);
    }
    if (CheckUDP)
    {
        CheckUDP = false;
        fullpool.CheckUDPServer();
        if (DHCPRefresh) //dont need to check this every loop so only to it after checking udp
        {
            byte rtnVal = Ethernet.maintain();

            switch (rtnVal)
            {
            case 1:
                Serial.println(F("\r\nDHCP renew fail"));
                break;
            case 2:
                //Serial.println(F("\r\nDHCP renew ok"));
                break;
            case 3:
                Serial.println(F("\r\nDHCP rebind fail"));
                break;
            case 4:
                //Serial.println(F("\r\nDHCP rebind ok"));
                break;
            }
        }
    }
    if (BroadcastUDP)
    {
        BroadcastUDP = false;
        fullpool.sendUDPBroadcast();
        if (PoolManagment::CreateNewPool) //we only want to do this once and we can do it after we stop UDP broadcasts
        {
            char printbuffer[100];
            PoolManagment::CreateNewPool = false;
            //fullpool.createPool();
            isMaster = true;
            // sprintf(printbuffer, "voting started with %u devices", fullpool.pool.size());
            //!    fullpool.broadcastMessage(printbuffer);
            for (size_t i = 0; i < fullpool.pool.size(); i++)
            {
                PoolDevice *dev = fullpool.pool.at(i);

                /* code */
                bool vote = false;
                if (fullpool.self.Health == dev->Health)
                {
                    vote = fullpool.self.RandomFactor > dev->RandomFactor;
                }
                else
                {
                    vote = fullpool.self.Health > dev->Health;
                }
                // sprintf(printbuffer, "dev %s, rf: %u, vote: %u, ismat: %u, myrf: %u", dev->Address.toString().c_str(), dev->RandomFactor, vote, dev->IsMaster, fullpool.self.RandomFactor);
                //!     fullpool.broadcastMessage(printbuffer);
                if (dev->IsMaster)
                {
                    vote = false;
                }
                if (!vote)
                {
                    isMaster = false;
                }
                else
                {
                    //sprintf(printbuffer, "i was voted for by:  %s ", dev->Address.toString().c_str());
                    //!      fullpool.broadcastMessage(printbuffer);
                }
            }
            if (isMaster)
            {
                //pinMode(SecondarySPI_MISO, INPUT);
                fullpool.broadcastMessage("i have been elected master");
                /*gpio_set_level(GPIO_NUM_27, 0);
                delayMicroseconds(20);
                gpio_set_level(GPIO_NUM_27, 1);
                delayMicroseconds(20);
                gpio_set_level(GPIO_NUM_27, 0);*/
                SecondarySPI = new SPIClass(HSPI); //VSPI ,HSPI
                //SCLK = 14, MISO = 12, MOSI = 13, SS = 15
                SecondarySPI->begin(SecondarySPI_SCLK, SecondarySPI_MISO, SecondarySPI_MOSI, SecondarySPI_SS);

                accSPI_EL = new ADXL345_SPI(SecondarySPI, AdxlSS_AZ, AdxlInt_AZ);
                accSPI_EL->init();

                accSPI_AZ = new ADXL345_SPI(SecondarySPI, AdxlSS_EL, AdxlInt_EL);
                accSPI_AZ->init();

                accSPI_Ballence = new ADXL345_SPI(SecondarySPI, AdxlSS_Ballence, AdxlInt_Ballence);
                accSPI_Ballence->init();
                //accSPI_Ballence->spi_clock_speed = 200000;

                startMeasuringTemp();
                startTransmitingToControlroom();
                fullpool.self.IsMaster = true;
                delay(100);
                startACCRefresh();
                //pinMode(SecondarySPI_MISO, INPUT_PULLUP);
            }
            else
            {
                //SecondarySPI->end();

                pinMode(SecondarySPI_MISO, INPUT);
                pinMode(SecondarySPI_MOSI, INPUT);
                pinMode(SecondarySPI_SCLK, INPUT);

                pinMode(AdxlSS_AZ, INPUT);
                pinMode(AdxlSS_EL, INPUT);
                pinMode(AdxlSS_Ballence, INPUT);
                //pinMode(AdxlInt_AZ, INPUT_PULLDOWN);
                //pinMode(AdxlInt_EL, INPUT_PULLDOWN);
                //pinMode(AdxlInt_Ballence, INPUT_PULLDOWN);
            }
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
        if (dataSize > 15)
        {
            //Serial.println(dataSize);
            //Serial.println(fullpool.ControlRoomIP.toString());
            //fullpool.SendPoolInfo(fullpool.ControlRoomIP, false);
            //fullpool.FowardDataToControlRoom(dataToSend, dataSize);
            FowardDataToControlRoom(dataToSend, dataSize, fullpool.ControlRoomIP, TCP_PORT);
            // this includes a flush and can be a very lengthy process the ethernet coms should at some point be put on their own thread
        }
        free(dataToSend);
    }
    if (PoolManagment::InUpdateMode)
    {
        handleWifiClient();
    }
    if (ACCRefresh)
    {
        ACCRefresh = false;
        Serial.println("refreshing ACC");
        int64_t now = esp_timer_get_time();
        if ((now - 10000) > accSPI_EL->LastEmptyBuffer)
        {
            accSPI_EL->bufferFull = true;
        }
        if ((now - 10000) > accSPI_AZ->LastEmptyBuffer)
        {
            accSPI_AZ->bufferFull = true;
        }
        if ((now - 10000) > accSPI_Ballence->LastEmptyBuffer)
        {
            accSPI_Ballence->bufferFull = true;
        }
    }
}

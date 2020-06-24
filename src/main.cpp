/* in Server.h change line below  
    virtual void begin(uint16_t port=0) =0;
    virtual void begin() = 0;
    in lib\picohttpparser\test.c remove line
    #include "picotest/picotest.h"
*/

/*
always keep a udp server running listening for broadcasts
and tcp server for direct connections
at startup broadcast udp to share this ip adress, until a tcp coection is made 
keep udp brodcasting for another 5 seconds to ensure all esp32 get the packets

*/
//#include <stdio.h>
//#include <freertos/FreeRTOS.h>
//#include <freertos/task.h>
//#include "sdkconfig.h"

#include <Arduino.h>
#include <Ethernet.h>
#include "../lib/picohttpparser/picohttpparser.h"
#include <EthernetHelp.hpp>
#include <spihelp.hpp>
#include <TempHelp.hpp>
#include <RedundantPool.hpp>
#include "freertos/timers.h"

//pin deffinitions
#define PrimarySPI_MISO 19
#define PrimarySPI_MOSI 23
#define PrimarySPI_SCLK 18
#define PrimarySPI_SS 5

#define SecondarySPI_MISO 12
#define SecondarySPI_MOSI 13
#define SecondarySPI_SCLK 14
#define SecondarySPI_SS -1

#define AdxlSS_AZ 15
#define AdxlSS_EL 32
#define AdxlSS_Ballence 17

#define AdxlInt_AZ 25
#define AdxlInt_EL 26
#define AdxlInt_Ballence 27

#define SDA 21
#define SCL 22

#define AZ_Temp_Line 4
#define EL_Temp_Line 16

bool isMaster = false;
bool poolActive = false;

//ethernet data
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress gateway(169, 254, 205, 1);
IPAddress subnet(255, 255, 0, 0);
const int ethernetPort = 1602;
//ethernet server
EthernetServer server(ethernetPort);
PoolManagment fullpool(
    IPAddress(169, 254, 205, 177),
    IPAddress(169, 254, 255, 255),
    8888);

//spi classes
SPIClass *PrimarySPI = NULL;   // primary spi buss, used as the default for most libraries
SPIClass *SecondarySPI = NULL; // secondary, mostly unused by library code

ADXL345_SPI *accSPI_AZ;
ADXL345_SPI *accSPI_EL;
ADXL345_SPI *accSPI_Ballence;
ADXLbuffer acc1buffer(1600); //; //= ADXLbuffer(1600);
ADXLbuffer acc2buffer(1600);
ADXLbuffer acc3buffer(1600);

Tempsensor AZTempSense(AZ_Temp_Line);
Tempsensor ELTempSense(EL_Temp_Line);


//timer definitions
#define NUM_TIMERS 5
TimerHandle_t xTimers[NUM_TIMERS];

uint32_t AZ_id = 0;
uint32_t EL_id = 1;
uint32_t CheckUDP_id = 2;
uint32_t CheckEthernet_id = 3;
uint32_t BroadcastUDP_id = 4;
std::atomic<bool> measureAZTemp;
std::atomic<bool> measureELTemp;
std::atomic<bool> CheckUDP;
std::atomic<bool> CheckEthernet;
std::atomic<bool> BroadcastUDP;

void TimerCallback(TimerHandle_t xTimer)
{
    uint32_t Timer_id;
    Timer_id = (uint32_t)pvTimerGetTimerID(xTimer);
    //vTimerSetTimerID(xTimer, (void *)ulCount);
    if (Timer_id == AZ_id)
    {
        measureAZTemp = true;
        //xTimerStop(xTimer, 0);
    }
    else if (Timer_id == EL_id)
    {
        measureELTemp = true;
    }
    else if (Timer_id == CheckUDP_id)
    {
        CheckUDP = true;
    }
    else if (Timer_id == CheckEthernet_id)
    {
        CheckEthernet = true;
    }
    else if (Timer_id == BroadcastUDP_id)
    {
        BroadcastUDP = true;
    }
}

TickType_t CheckUDPTicks = pdMS_TO_TICKS(250);
TickType_t CheckEthernetTicks = pdMS_TO_TICKS(250);
TickType_t BroadcastUDPTicks = pdMS_TO_TICKS(1000);
TickType_t MeasureAZTempTicks = pdMS_TO_TICKS(1000);
TickType_t MeasureELTempTicks = pdMS_TO_TICKS(1000);

void setup()
{
    xTimers[AZ_id] = xTimerCreate("MeasureAZTemp", MeasureAZTempTicks, pdTRUE, (void *)AZ_id, TimerCallback);
    xTimers[EL_id] = xTimerCreate("MeasureELTemp", MeasureELTempTicks, pdTRUE, (void *)EL_id, TimerCallback);
    xTimers[CheckUDP_id] = xTimerCreate("CheckUDP", CheckUDPTicks, pdTRUE, (void *)CheckUDP_id, TimerCallback);
    xTimers[CheckEthernet_id] = xTimerCreate("CheckEthernet", CheckEthernetTicks, pdTRUE, (void *)CheckEthernet_id, TimerCallback);
    xTimers[BroadcastUDP_id] = xTimerCreate("BroadcastUDP", BroadcastUDPTicks, pdTRUE, (void *)BroadcastUDP_id, TimerCallback);

    long x;
    for (x = 0; x < NUM_TIMERS; x++)
    {

        if (xTimers[x] == NULL)
        {
            /* The timer was not created. */
        }
        else
        {
            if (xTimerStart(xTimers[x], 0) != pdPASS)
            {
                /* The timer could not be set into the Active
                 state. */
            }
        }
    }
    // vTaskStartScheduler();

    SecondarySPI = new SPIClass(HSPI);
    //SCLK = 14, MISO = 12, MOSI = 13, SS = 15
    SecondarySPI->begin(SecondarySPI_SCLK, SecondarySPI_MISO, SecondarySPI_MOSI, SecondarySPI_SS);

    accSPI_EL = new ADXL345_SPI(SecondarySPI, AdxlSS_AZ, AdxlInt_AZ);
    accSPI_EL->init();

    accSPI_AZ = new ADXL345_SPI(SecondarySPI, AdxlSS_EL, AdxlInt_EL);
    accSPI_AZ->init();

    accSPI_Ballence = new ADXL345_SPI(SecondarySPI, AdxlSS_Ballence, AdxlInt_Ballence);
    accSPI_Ballence->init();
    accSPI_Ballence->spi_clock_speed = 200000;
        //acc1buffer = new ADXLbuffer(1600);

        // You can use Ethernet.init(pin) to configure the CS pin
        //SCLK = 18, MISO = 19, MOSI = 23, SS = 5
        Ethernet.init(PrimarySPI_SS);
    // initialize the ethernet device
    Ethernet.begin(mac, fullpool.self.address, gateway, gateway, subnet);

    // Open serial communications and wait for port to open:
    Serial.begin(115200);

    while (!Serial)
    {
        ; // wait for serial port to connect. Needed for native USB port only
    }

    // Check for Ethernet hardware present
    if (Ethernet.hardwareStatus() == EthernetNoHardware)
    {
        Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
        while (true)
        {
            delay(1); // do nothing, no point running without Ethernet hardware
        }
    }
    if (Ethernet.linkStatus() == LinkOFF)
    {
        Serial.println("Ethernet cable is not connected.");
    }

    // start listening for clients
    server.begin();
    fullpool.UDPSetup();
    Serial.print("ethernet server address:");
    Serial.println(Ethernet.localIP());
    Serial.print("ethernet server port:");
    Serial.println(ethernetPort);
}
int tem = 0;
void loop()
{
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
                int len = processOneTimeConnection(client, bytes, data, Serial);
                if (len == -1)
                {
                }
            }
        }
    }

 
    // Serial.println(data2);
    
    if (measureAZTemp)
    {
        measureAZTemp = false;
        accbuffer buffer = emptyAdxlBuffer((SPI_DEVICE)*accSPI_AZ);
        for (size_t i = 0; i < buffer.lenght; i++)
        {
            acc1buffer.buffer.push(buffer.buffer[i]);
        }
        buffer = emptyAdxlBuffer((SPI_DEVICE)*accSPI_EL);
        for (size_t i = 0; i < buffer.lenght; i++)
        {
            acc2buffer.buffer.push(buffer.buffer[i]);
        }
        char data2[30];
        sprintf(data2, "%d, %d, %d,    1    %d", acc1buffer.buffer.front().x, acc1buffer.buffer.front().y, acc1buffer.buffer.front().z, acc1buffer.buffer.size());
        acc1buffer.buffer.pop();
        //  Serial.println(data2);
        sprintf(data2, "%d, %d, %d,    2    %d", acc2buffer.buffer.front().x, acc2buffer.buffer.front().y, acc2buffer.buffer.front().z, acc2buffer.buffer.size());
        acc2buffer.buffer.pop();
        //Serial.println(AZTempSense.read());
    }
    if (measureELTemp)
    {
        measureELTemp = false;
        //Serial.println(ELTempSense.read());
    }

    if (CheckUDP)
    {
        CheckUDP = false;
        //Serial.println("____________________________________recieve");
        //fullpool.CheckUDPServer();
    }
    if (BroadcastUDP)
    {
        BroadcastUDP = false;
        //Serial.println("____________________________________send");
        //fullpool.sendUDPBroadcast();
    }
}

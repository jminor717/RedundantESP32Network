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

#include <Arduino.h>
#include <Ethernet.h>
#include "../lib/picohttpparser/picohttpparser.h"
#include <EthernetHelp.hpp>
#include <spihelp.hpp>
#include <TempHelp.hpp>
#include <RedundantPool.hpp>

//pin deffinitions
#define PrimarySPI_MISO 19
#define PrimarySPI_MOSI 23
#define PrimarySPI_SCLK 18
#define PrimarySPI_SS 5

#define SecondarySPI_MISO 12
#define SecondarySPI_MOSI 13
#define SecondarySPI_SCLK 14
#define SecondarySPI_SS -1

#define Adxl1SS 15
#define Adxl2SS 32

#define AZ_Temp_Line 4
#define EL_Temp_Line 17

bool isMaster = false;
bool poolActive = false;

//ethernet data
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(169, 254, 205, 177);
IPAddress gateway(169, 254, 205, 1);
IPAddress subnet(255, 255, 0, 0);
const int ethernetPort = 1602;
//ethernet server
EthernetServer server(ethernetPort);
//EthernetClient client2();
//spi classes
SPIClass *PrimarySPI = NULL;   // primary spi buss, used as the default for most libraries
SPIClass *SecondarySPI = NULL; // secondary, mostly unused by library code

ADXL345_SPI *accSPI1;
ADXL345_SPI *accSPI2;
ADXLbuffer acc1buffer(1600); //; //= ADXLbuffer(1600);
ADXLbuffer acc2buffer(1600);
ADXLbuffer acc3buffer(1600);

Tempsensor AZTempSense(AZ_Temp_Line);
Tempsensor ELTempSense(EL_Temp_Line);

unsigned long currentTime;

unsigned long usTimeCheckUDPserver = 250000UL;
unsigned long previousTimeCheckUDP;
unsigned long usTimeCheckEthernetServer = 250000UL;
unsigned long previousTimeCheckEthernet;
unsigned long usTimeBroadcastUDP = (unsigned long)(1 * 1000 * 1000);
unsigned long previousTimeBroadcastUDP;
unsigned long usTimeMeasureTemp = (unsigned long)(1 * 1000 * 1000);
unsigned long previousTimeMeasureTemp;

void setup()
{
    SecondarySPI = new SPIClass(HSPI);
    //SCLK = 14, MISO = 12, MOSI = 13, SS = 15
    SecondarySPI->begin(SecondarySPI_SCLK, SecondarySPI_MISO, SecondarySPI_MOSI, SecondarySPI_SS);

    accSPI2 = new ADXL345_SPI(SecondarySPI, Adxl1SS);
    accSPI2->init();

    accSPI1 = new ADXL345_SPI(SecondarySPI, Adxl2SS);
    accSPI1->init();
    accSPI1->spi_clock_speed = 200000;
    //acc1buffer = new ADXLbuffer(1600);

    // You can use Ethernet.init(pin) to configure the CS pin
    //SCLK = 18, MISO = 19, MOSI = 23, SS = 5
    Ethernet.init(PrimarySPI_SS);
    // initialize the ethernet device
    Ethernet.begin(mac, ip, gateway, gateway, subnet);

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
    UDPSetup();
    Serial.print("ethernet server address:");
    Serial.println(Ethernet.localIP());
    Serial.print("ethernet server port:");
    Serial.println(ethernetPort);
    currentTime = micros();
    previousTimeCheckUDP = currentTime;
    previousTimeCheckEthernet = currentTime;
    previousTimeBroadcastUDP = currentTime;
    previousTimeMeasureTemp = currentTime;
}
int tem = 0;
void loop()
{
    currentTime = micros();
    if (currentTime - previousTimeCheckEthernet > usTimeCheckEthernetServer)
    {
        previousTimeCheckEthernet = micros();
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
    /*
    accbuffer buffer = emptyAdxlBuffer((SPI_DEVICE)*accSPI2);
    for (size_t i = 0; i < buffer.lenght; i++)
    {
        acc1buffer.buffer.push(buffer.buffer[i]);
    }
    buffer = emptyAdxlBuffer((SPI_DEVICE)*accSPI1);
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
    // Serial.println(data2);
    if (currentTime - previousTimeMeasureTemp > usTimeMeasureTemp)
    {
        previousTimeMeasureTemp = micros();
    //  Serial.println(ELTempSense.read());
    //  Serial.println(AZTempSense.read());
    }

    */
    if (currentTime - previousTimeCheckUDP > usTimeCheckUDPserver)
    {
        previousTimeCheckUDP = micros();
        Serial.println("____________________________________recieve");
        CheckUDPServer();
    }
    if (currentTime - previousTimeBroadcastUDP > usTimeBroadcastUDP)
    {
        previousTimeBroadcastUDP = micros();
        Serial.println("____________________________________send");
        sendUDPBroadcast();
    }
}

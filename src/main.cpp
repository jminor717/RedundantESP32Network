#include <Arduino.h>
#include <Ethernet.h>
#include "../lib/picohttpparser/picohttpparser.h"
#include <EthernetHelp.hpp>
#include <spihelp.hpp>

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

//ethernet data
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(169, 254, 205, 177);
IPAddress subnet(255, 255, 0, 0);
const int ethernetPort = 1602;
//ethernet server
EthernetServer server(ethernetPort);
//spi classes
SPIClass *PrimarySPI = NULL;   // primary spi buss, used as the default for most libraries
SPIClass *SecondarySPI = NULL; // secondary, mostly unused by library code

ADXL345_SPI *accSPI1;
ADXL345_SPI *accSPI2;
ADXLbuffer acc1buffer(1600); //; //= ADXLbuffer(1600);
ADXLbuffer acc2buffer(1600);
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
    Ethernet.begin(mac, ip, subnet);

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

    Serial.print("ethernet server address:");
    Serial.println(Ethernet.localIP());
    Serial.print("ethernet server port:");
    Serial.println(ethernetPort);
}
int tem = 0;
void loop()
{
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
    char data[30];
    sprintf(data, "%d, %d, %d,    1    %d", acc1buffer.buffer.front().x, acc1buffer.buffer.front().y, acc1buffer.buffer.front().z, acc1buffer.buffer.size());
    acc1buffer.buffer.pop();
    Serial.println(data);
    sprintf(data, "%d, %d, %d,    2    %d", acc2buffer.buffer.front().x, acc2buffer.buffer.front().y, acc2buffer.buffer.front().z, acc2buffer.buffer.size());
    acc2buffer.buffer.pop();
    Serial.println(data);
    delay(20);
}

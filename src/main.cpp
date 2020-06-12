#include <Arduino.h>
#include <Ethernet.h>
#include "../lib/picohttpparser/picohttpparser.h"
#include <EthernetHelp.h>
#include <spihelp.h>

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(169, 254, 205, 177);
IPAddress subnet(255, 255, 0, 0);

const int ethernetPort = 1602;
EthernetServer server(ethernetPort);

void setup()
{
    // You can use Ethernet.init(pin) to configure the CS pin
    Ethernet.init(5);
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
            uint8_t data[bytes] ={0};
            ptr = data;
            client.read(ptr, bytes);
            int len = processOneTimeConnection(client, bytes, data, Serial);
            if(len == -1){
                
            }
        }
    }
}

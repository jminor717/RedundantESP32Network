//#include "../lib/picohttpparser/picohttpparser.h"
#include <Arduino.h>
#include <Ethernet.h>
#include <EthernetHelp.hpp>
#include <myConfig.hpp>


//*Ethernet
void sendEthernetMessage(const char *msg, size_t length, IPAddress destination)
{
    EthernetClient sendClient;
    if (sendClient.connect(destination, TCPPORT))
    {
        Serial.println("connected");
        sendClient.println(msg);
        sendClient.println();
    }
    else
    {
        Serial.println("connection failed");
    }
}

void FowardDataToControlRoom(uint8_t *data, size_t DataLength, IPAddress ctrlRmAddress, uint16_t ctrlRmPort)
{
    EthernetClient sendClient;
    if (sendClient.connect(ctrlRmAddress, ctrlRmPort))
    {
        sendClient.write(data, DataLength);
        sendClient.flush();
        sendClient.stop();
    }
    else
    {
        Serial.println("TCP NOTTTT connected");
    }
}


//*Wifi
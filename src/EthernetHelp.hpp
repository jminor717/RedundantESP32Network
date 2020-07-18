#include "../lib/picohttpparser/picohttpparser.h"
#include <Arduino.h>
#include <Ethernet.h>

int processOneTimeConnection(EthernetClient, size_t, uint8_t *, HardwareSerial);
void printHTTPheader(const char *, size_t , const char *, size_t , int , phr_header headers[100], size_t , int , HardwareSerial &);
void sendEthernetMessage(const char *, size_t, IPAddress);
void FowardDataToControlRoom(uint8_t *, size_t, IPAddress,uint16_t);

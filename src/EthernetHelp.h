#include "../lib/picohttpparser/picohttpparser.h"
#include <Arduino.h>
#include <Ethernet.h>

int processOneTimeConnection(EthernetClient, size_t, uint8_t *, HardwareSerial);
//void printHTTPheader(const char *, size_t, const char *, size_t, int, phr_header[100], size_t, int, HardwareSerial);


#include <Arduino.h>
#include <Ethernet.h>
//#include <RedundantPool.hpp>

//void printHTTPheader(const char *, size_t , const char *, size_t , int , phr_header headers[100], size_t , int , HardwareSerial &);
void sendEthernetMessage(const char *, size_t, IPAddress);
void FowardDataToControlRoom(uint8_t *, size_t, IPAddress,uint16_t);
//void StartWifi(PoolManagment &fullpool);
IPAddress StartWifi();
void endWifi();
void handleWifiClient();
IPAddress findOpenAdressAfterStart( IPAddress SelfAddress, IPAddress gateway, IPAddress subnet);
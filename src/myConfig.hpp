// https://randomnerdtutorials.com/esp32-pinout-reference-gpios/
// 20, 24, 28, 29, 30, 31 are not exposed on the qfn package
// 0, 2, 5, 12, 15 can affect boot sequence if used improperly
// 6, 7, 8, 9, 10, 11 are connected to internal spi flash
// 32, 33 are connected to intrenal crystal and cannot be used
// 34, 35, 36, 39 are input only
// 1, 3, 5, 14, 15 output pwm durring boot/reboot, do not tie between boards

//unused input: 18, 36, 39
#define PrimarySPI_MISO 23
#define PrimarySPI_MOSI 22
#define PrimarySPI_SCLK 21
#define PrimarySPI_SS 19

#define SecondarySPI_MISO 12 //this needs to be changed inorder to run updates  https://www.esp32.com/viewtopic.php?t=3452
#define SecondarySPI_MOSI 26
#define SecondarySPI_SCLK 27
#define SecondarySPI_SS -1

#define AdxlSS_AZ 13
#define AdxlSS_EL 2
#define AdxlSS_Ballence 4

#define AdxlInt_AZ 34
#define AdxlInt_EL 35
#define AdxlInt_Ballence 25

#define AZ_Temp_Line 16
#define EL_Temp_Line 17
//config
#define TCP_PORT 1602
#define UDP_PORT 8888
#define WIFI_PORT 80
#define DATA_TRANSMIT_ID 131
#define POOL_DEVICE_TRANSMIT_ID  130
#define MESSAGE_DELIMINER_CODE_ASCII 17//device code one used as deliminer in messages so any standard ascii char can be part of the message
#define POOL_DEVICE_TIMEOUT 10*1000*1000 // time in micorseconds afterwich prior status data from one of the pool devices will be considered invalid

#define ACC_BUFFER_SIZE 1600

//#define WIFIssid "ESP32-Access-Point"
#define WIFIssid "Pretty fly for a wifi"
//#define WIFIpassword "123456789"
#define WIFIpassword "JRMinor1!"
#define WIFI_HOST "esp32"

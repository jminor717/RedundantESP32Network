#include <Arduino.h>
#include <Ethernet.h>
#include <EthernetHelp.hpp>
#include <myConfig.hpp>
#include <version.h>
//wifi
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <esp_wifi.h>

WebServer WIFIserver(WIFI_PORT);
bool inUpdateMode = false;

//* Style
String style =
    "<style>#file-input,input{width:100%;height:44px;border-radius:4px;margin:10px auto;font-size:15px}"
    "input{background:#f1f1f1;border:0;padding:0 15px}body{background:darkred;font-family:sans-serif;font-size:14px;color:#777}"
    "#file-input{padding:0;border:1px solid #ddd;line-height:44px;text-align:left;display:block;cursor:pointer}"
    "#bar,#prgbar{background-color:#f1f1f1;border-radius:10px}#bar{background-color:darkred;width:0%;height:10px}"
    "#d{background:#fff;max-width:258px;margin:75px auto;padding:30px;border-radius:5px;text-align:center}"
    ".btn{background:darkred;color:#fff;cursor:pointer}</style>";

//* Login page
String loginIndex =
    "<form name=loginForm>"
    "<h1>ESP32 Login</h1>"
    "<input name=userid placeholder='User ID'> "
    "<input name=pwd placeholder=Password type=Password> "
    "<input type=submit onclick=check(this.form) class=btn value=Login></form>"
    "<script>"
    "function check(form) {"
    "if(form.userid.value=='admin' && form.pwd.value=='admin')"
    "{window.open('/serverIndex')}"
    "else"
    "{alert('Error Password or Username')}"
    "}"
    "</script>" +
    style;

//* Server Index Page
String serverIndex =
    "<div id='d'>"
    "<div style = margin: 0 auto;>"
    "current version:"
    "</div>"
    "<div style = margin: 0 auto;>" VERSION
    "</div>"
    "<input type='file' name='update' id='file' onchange='sub(this,this.files[0])' style=display:none>"
    "<label id='file-input' for='file'>Choose file...</label>"
    "<input type='submit' class=btn value='Update' onclick='upload();'>"
    "<div id='prg'></div>"
    "<br>"
    "<div id='prgbar'>"
    "<div id='bar'></div>"
    "</div>"
    "</div>"
    "<script>"
    "var file = new FormData();"
    "function upload() {"
    "let xhr = new XMLHttpRequest();"
    "xhr.timeout = 60000;"
    "xhr.upload.onprogress = function (event) {"
    "if (event.lengthComputable) {"
    " var per = event.loaded / event.total;"
    "document.getElementById('prg').innerHTML = 'progress: ' + Math.round(per * 100) + '%';"
    "document.getElementById('bar').style.width = Math.round(per * 100) + '%';"
    "}"
    "};"
    "xhr.onloadend = function () {"
    "if (xhr.status == 200) {"
    "console.log('success');"
    "} else {"
    "console.log('error' + this.status);"
    "}"
    "};"
    "xhr.open('POST', '/update');"
    "xhr.contentType = false;"
    "xhr.processData = false;"
    "console.log(file);"
    "xhr.send(file);"
    "}"
    "function sub(obj, vle) {"
    "file.append('file', vle);"
    "var fileName = obj.value.split('\\\\');"
    "document.getElementById('file-input').innerHTML = '   ' + fileName[fileName.length - 1];"
    "}"
    "</script>" +
    style;

//*Ethernet
void sendEthernetMessage(const char *msg, size_t length, IPAddress destination)
{
    EthernetClient sendClient;
    if (sendClient.connect(destination, TCP_PORT))
    {
        Serial.println("connected");
        sendClient.println(msg);
        sendClient.println();
        sendClient.flush();
        sendClient.stop();
    }
    else
    {
        Serial.println("connection failed");
    }
}

void FowardDataToControlRoom(uint8_t *data, size_t DataLength, IPAddress ctrlRmAddress, uint16_t ctrlRmPort)
{
    EthernetClient sendClient;
    if (sendClient.connect(ctrlRmAddress, TCP_PORT))
    {
        sendClient.write(data, DataLength);
        sendClient.flush();
        sendClient.stop();
    }
    else
    {
        Serial.print("TCP NOTTTT connected  sdtcrlm: ");
        Serial.println(sendClient.connected());
    }
}

IPAddress findOpenAdressAfterStart(IPAddress SelfAddress, IPAddress gateway, IPAddress subnet)
{
    // initialize the ethernet device
    byte mac[6];
    if (WiFiGenericClass::getMode() == WIFI_MODE_NULL)
    {
        esp_read_mac(mac, ESP_MAC_WIFI_STA);
    }
    else
    {
        esp_wifi_get_mac(WIFI_IF_STA, mac);
    }
    delay(100); //ensure ethernet module has time to initilize

    if (Ethernet.begin(mac) == 0)
    { // windows dhcp server can be found here https://www.dhcpserver.de/cms/running_the_server/
        // use comand:  nmap --script broadcast-dhcp-discover    to make sure the dhcp server is working
        Serial.println("Failed to configure Ethernet using DHCP");
        Serial.println("Please reconfigure the settings and try again.");
        // no point in carrying on, so do nothing forevermore:
        Serial.println("REBOOTING...");
        delay(3000);
        ESP.restart();
    }
    else
    {
        Serial.println("Ethernet ready");
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

    return Ethernet.localIP();
}

//*Wifi
//void StartWifi(PoolManagment &fullpool)
IPAddress StartWifi()
{
    IPAddress wifiadr;
    if (!inUpdateMode) //start wifi if it hasnt been already
    {
        inUpdateMode = true;
        //WiFi.softAP(WIFIssid, WIFIpassword);
        WiFi.begin(WIFIssid, WIFIpassword);
        //*
        while (WiFi.status() != WL_CONNECTED)
        {
            delay(500);
            Serial.print(".");
        }
        Serial.println(""); //*/
        char printbuffer[100];
        // wifiadr = WiFi.softAPIP();
        wifiadr = WiFi.localIP();
        sprintf(printbuffer, " Connected to %s     IP address: %s", WIFIssid, wifiadr.toString().c_str());
        Serial.println(printbuffer);
        // fullpool.broadcastMessage(printbuffer);
        /*use mdns for host name resolution*/
        if (!MDNS.begin(WIFI_HOST))
        { //http://esp32.local
            Serial.println("Error setting up MDNS responder!");
            while (1)
            {
                delay(1000);
            }
        }
        Serial.println("mDNS responder started");
        /*return index page which is stored in serverIndex */
        WIFIserver.on("/", HTTP_GET, []() {
            WIFIserver.sendHeader("Connection", "close");
            WIFIserver.send(200, "text/html", loginIndex);
        });
        WIFIserver.on("/serverIndex", HTTP_GET, []() {
            WIFIserver.sendHeader("Connection", "close");
            WIFIserver.send(200, "text/html", serverIndex);
        });
        /*handling uploading firmware file */
        WIFIserver.on(
            "/update", HTTP_POST, []() {
        Serial.println("update started");
        WIFIserver.sendHeader("Connection", "close");
        WIFIserver.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
        ESP.restart(); }, []() {
        HTTPUpload &upload = WIFIserver.upload();
        if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("Update: %s\n", upload.filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
            Update.printError(Serial);
        }
        }
        else if (upload.status == UPLOAD_FILE_WRITE)
        {
            /* flashing firmware to ESP*/
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
            {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) { //true to set the size to the current progress
            Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
            WIFIserver.sendHeader("Connection", "close");
            WIFIserver.send(200, "application/json", "{status:'Update Success' }");
            WIFIserver.close();
        } else {
            Update.printError(Serial);
        }
        } });
        WIFIserver.begin();
    }
    else
    {
        //wifiadr = WiFi.softAPIP();
        wifiadr = WiFi.localIP();
    }
    return wifiadr;
}

void endWifi()
{
    if (inUpdateMode) //end the wifi if its active
    {
        WIFIserver.close();
        WIFIserver.stop();
        WiFi.mode(WIFI_MODE_NULL); //stop wifi
        btStop();                  //make sure radio is off
    }
}

void handleWifiClient()
{
    WIFIserver.handleClient();
}
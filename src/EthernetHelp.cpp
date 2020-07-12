#include "../lib/picohttpparser/picohttpparser.h"
#include <Arduino.h>
#include <Ethernet.h>
#include <EthernetHelp.hpp>
#include <myConfig.hpp>

const char *loginIndex =
    "<form name='loginForm'>"
    "<table width='20%' bgcolor='A09F9F' align='center'>"
    "<tr>"
    "<td colspan=2>"
    "<center><font size=4><b>ESP32 Login Page</b></font></center>"
    "<br>"
    "</td>"
    "<br>"
    "<br>"
    "</tr>"
    "<td>Username:</td>"
    "<td><input type='text' size=25 name='userid'><br></td>"
    "</tr>"
    "<br>"
    "<br>"
    "<tr>"
    "<td>Password:</td>"
    "<td><input type='Password' size=25 name='pwd'><br></td>"
    "<br>"
    "<br>"
    "</tr>"
    "<tr>"
    "<td><input type='submit' onclick='check(this.form)' value='Login'></td>"
    "</tr>"
    "</table>"
    "</form>"
    "<script>"
    "function check(form)"
    "{"
    "if(form.userid.value=='admin' && form.pwd.value=='admin')"
    "{"
    "window.open('/serverIndex')"
    "}"
    "else"
    "{"
    " alert('Error Password or Username')/*displays error message*/"
    "}"
    "}"
    "</script>";

/*
 * Server Index Page
 */

const char *serverIndex =
    "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
    "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
    "<input type='file' name='update'>"
    "<input type='submit' value='Update'>"
    "</form>"
    "<div id='prg'>progress: 0%</div>"
    "<script>"
    "$('form').submit(function(e){"
    "e.preventDefault();"
    "var form = $('#upload_form')[0];"
    "var data = new FormData(form);"
    " $.ajax({"
    "url: '/update',"
    "type: 'POST',"
    "data: data,"
    "contentType: false,"
    "processData:false,"
    "xhr: function() {"
    "var xhr = new window.XMLHttpRequest();"
    "xhr.upload.addEventListener('progress', function(evt) {"
    "if (evt.lengthComputable) {"
    "var per = evt.loaded / evt.total;"
    "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
    "}"
    "}, false);"
    "return xhr;"
    "},"
    "success:function(d, s) {"
    "console.log('success!')"
    "},"
    "error: function (a, b, c) {"
    "}"
    "});"
    "});"
    "</script>";

void printHTTPheader(const char *method, size_t method_len, const char *path, size_t path_len, int minor_version, phr_header headers[100], size_t num_headers, int pret, HardwareSerial &Serial)
{
    printf("request is %d bytes long\n", pret);
    printf("method is %.*s\n", (int)method_len, method);
    printf("path is %.*s\n", (int)path_len, path);
    printf("HTTP version is 1.%d\n", minor_version);
    Serial.println("headers:");
    for (int q = 0; q != num_headers; ++q)
    {
        printf("    %.*s: %.*s\n", (int)headers[q].name_len, headers[q].name,
               (int)headers[q]
                   .value_len,
               headers[q].value);
    }
}

int processOneTimeConnection(EthernetClient client, size_t bytes, uint8_t *ptr, HardwareSerial Serial)
{
    Serial.println("############################################################################################\nnew client");

    const char *method, *path;
    int pret, minor_version;
    struct phr_header headers[100];
    size_t method_len, path_len, num_headers;

    //uint8_t data[bytes];
    char msg[bytes];
    //ptr = data;//ptr is a pointer to the data read from the ethernet client

    for (int j = 0; j < bytes; j++)
    {
        msg[j] = (char)ptr[j];
    }
    // printf("%.*s\n", (int)bytes, msg);
    // printf("message is %d bytes long\n", bytes);
    pret = phr_parse_request(msg, bytes, &method, &method_len, &path, &path_len, &minor_version, headers, &num_headers, (size_t)0);
    //printHTTPheader(method, method_len, path, path_len, minor_version, headers, num_headers, pret, Serial);
    if (pret == -1)
    { // not a valid http request
        //printHTTPheader(method, method_len, path, path_len, minor_version, headers, num_headers, pret, Serial);
        return pret;
    }
    else if (pret == -2)
    {
        printf("request was too large at %u bytes long\n", bytes);
        // printf("%.*s\n", (int)bytes, msg);
        return pret;
    }
    char shortPath[path_len + 1] = {0};
    for (int w = 0; w < path_len; w++)
        shortPath[w] = path[w];
    printf("path is %s\n", shortPath);
    if (sizeof(path_len) == 1 && strcmp(shortPath, "/") == 0)
    {
        // send a standard http response header
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/html");
        client.println("Connection: close"); // the connection will be closed after completion of the response
        //client.println("Refresh: 30");       // refresh the page automatically every 5 sec
        client.println();
        client.println(loginIndex);
    }
    else if (strcmp(shortPath, "/LOL") == 0)
    {
        Serial.println("client LOLed");
        client.println("{\"resp\":\"sup\"}");
    }
    else if (strcmp(shortPath, "/serverIndex") == 0)
    {
        // send a standard http response header
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/html");
        client.println("Connection: close"); // the connection will be closed after completion of the response
        //client.println("Refresh: 30");       // refresh the page automatically every 5 sec
        client.println();
        client.println(serverIndex);
    }
    else if (strcmp(shortPath, "/update") == 0)
    {
        Serial.println("attempt update");
    }
    else
    {
        client.println("no responce");
    }

    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
    return pret;
}

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


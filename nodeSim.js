'use strict';
/*
var net = require('net');

// Configuration parameters
var HOST = '169.254.205.47';
var PORT = 1602;

// Create Server instance 
var server = net.createServer(onClientConnected);

server.listen(PORT, HOST, function () {
    console.log('server listening on %j', server.address());
});

function onClientConnected(sock) {
    var remoteAddress = sock.remoteAddress + ':' + sock.remotePort;
    console.log('new client connected: %s', remoteAddress);
    sock.write("dd");
    sock.on('data', function (data) {
        console.log('%s Says: %s', remoteAddress, data);
        sock.write(data);
        sock.write(' exit');
    });
    sock.on('close', function () {
        console.log('connection from %s closed', remoteAddress);
    });
    sock.on('error', function (err) {
        console.log('Connection %s error: %s', remoteAddress, err.message);
    });
};
*/

//npm install low-pass-filter -g
var os = require('os');
var ifaces = os.networkInterfaces();
var selfIP = "169.254.205.47";
var broadCastTarget = "169.254.255.255";


var dgram = require('dgram');
var UDP_Client = dgram.createSocket("udp4");
const UDPPORT = 8888;

const tcpport = 1602;
var recivedfrom = []
var broadcast;
var controlRoomBroadcast;


console.log(selfIP);
console.log(broadCastTarget);

const MULTICAST_ADDR = "233.255.255.255";// "255.255.255.0";//
//192.168.0.70

const process = require("process");



class pooldev {
    Address = [4];
    IsMaster = false;
    Health = 0;
    RandomFactor = 0;
    version ="";
    constructor(addrSTR) {
        let dat = addrSTR.split('.')
        for (let index = 0; index < dat.length; index++) {
            const element = dat[index];
            this.Address[index] = parseInt(element, 10)
        }
    }
    OBJID = 130
    Transmit_Prep() {
        let i = 0;
        let buffer = new ArrayBuffer(14);
        let data = new Uint8Array(buffer, 0, buffer.length);
        data[i++] = this.OBJID; // code to use for identifing this object
        data[i++] = this.Address[0];
        data[i++] = this.Address[1];
        data[i++] = this.Address[2];
        data[i++] = this.Address[3];

        data[i++] = this.IsMaster;

        data[i++] = this.Health & 0x000000ff;
        data[i++] = (this.Health & 0x0000ff00) >> 8;
        data[i++] = (this.Health & 0x00ff0000) >> 16;
        data[i++] = (this.Health & 0xff000000) >> 24;

        data[i++] = this.RandomFactor & 0x000000ff;
        data[i++] = (this.RandomFactor & 0x0000ff00) >> 8;
        data[i++] = (this.RandomFactor & 0x00ff0000) >> 16;
        data[i++] = (this.RandomFactor & 0xff000000) >> 24;
        return data; //return data length 14
    }

    From_Transmition(data) {
        let i = 0;
        this.Address[0] = data[++i];
        this.Address[1] = data[++i];
        this.Address[2] = data[++i];
        this.Address[3] = data[++i];

        this.IsMaster = data[++i];

        this.Health = data[++i];
        this.Health += data[++i] << 8;
        this.Health += data[++i] << 16;
        this.Health += data[++i] << 24;

        this.RandomFactor = data[++i];
        this.RandomFactor += data[++i] << 8;
        this.RandomFactor += data[++i] << 16;
        this.RandomFactor += data[++i] << 24;

        this.vote = !!data[++i];
        let length = data.length -(i+1)
        this.version ="";
        for (let index = 0; index < length; index++) {
            this.version += String.fromCharCode(data[++i]);
            
        }

        return ++i; //return data length 15 + version length
    }
}


const selfDev = new pooldev(selfIP);

Object.keys(ifaces).forEach(function (ifname) {
    var alias = 0;
    ifaces[ifname].forEach(function (iface) {
        if ('IPv4' !== iface.family || iface.internal !== false) {
            // skip over internal (i.e. 127.0.0.1) and non-ipv4 addresses
            return;
        }
        if (alias >= 1) {
            // this single interface has multiple ipv4 addresses
            console.log(ifname + ':' + alias, iface.address);
        } else {
            // this interface has only one ipv4 adress
            //console.log(ifname + "    " + iface.address);
            if (ifname == "Ethernet 4") {
                selfIP = iface.address;
                let blockIP = selfIP.split(".");
                let blockSubNet = iface.netmask.split(".");

                for (let index = 0; index < blockSubNet.length; index++) {
                    if (blockSubNet[index] === '0') {
                        blockIP[index] = "255";
                    }
                }
                broadCastTarget = blockIP.join().replace(/,/g, '.');
            }
        }
        ++alias;
    });
});





function bin2String(array) {
    let result = [];
    let substr = "";
    let len = 0;
    for (var i = 0; i < array.length; i++) {
        if (array[i] == 17) {
            result.push(substr);
            substr = "";
            len = 0;
        } else {
            substr += String.fromCharCode(array[i]);
            len++;
        }
    }
    if (len > 0) {
        result.push(substr);
    }
    return result;
}

var accbufferAZ = [], accbufferEL = [], accbufferBAL = [], AZTEmpBuffer = [], ELTEmpBuffer = []

function TwosCompliment16bit(lsb, msb) {
    var unsignedValue = (msb << 8) | lsb;
    if (unsignedValue & 0x8000) {
        // If the sign bit is set, then set the two first bytes in the result to 0xff.
        return unsignedValue | 0xffff0000;
    } else {
        // If the sign bit is not set, then the result  is the same as the unsigned value.
        return unsignedValue;
    }
}

var lastAZ = { x: 0, y: 0, z: 200 }, lastEL = { x: 0, y: 0, z: 200 }, lastBAL = { x: 0, y: 0, z: 200 }
var azinit = false, elinit = false, balinit = false;
function parseBigData(buf) {
    let accBufSixeAZ = buf[6];
    accBufSixeAZ += (buf[5] << 8);
    let accBufSixeEL = buf[8];
    accBufSixeEL += (buf[7] << 8);
    let accBufSixeBAL = buf[10];
    accBufSixeBAL += (buf[9] << 8);

    let AZTempSize = buf[12];
    AZTempSize += (buf[11] << 8);
    let ELTempSize = buf[14];
    ELTempSize += (buf[13] << 8);
    accBufSixeAZ = accBufSixeAZ / 6;
    accBufSixeEL = accBufSixeEL / 6;
    accBufSixeBAL = accBufSixeBAL / 6;
    let i = 15;
    let accDataAZ = [], accDataEL = [], accDataBAL = [], AZtempData = [], ELtempData = [];
    for (let index = 0; index < accBufSixeAZ; index++) {
        let element = {}
        element.x = TwosCompliment16bit(buf[i++], buf[i++])
        element.y = TwosCompliment16bit(buf[i++], buf[i++])
        element.z = TwosCompliment16bit(buf[i++], buf[i++])
        if (Math.abs(element.x) > 500)
            element.x = lastAZ.x
        if (Math.abs(element.y) > 500)
            element.y = lastAZ.y
        if (Math.abs(element.z) > 500)
            element.z = lastAZ.z
        if (azinit) {
            if (Math.abs(element.x - lastAZ.x) > 170)
                element.x = lastAZ.x
            if (Math.abs(element.y - lastAZ.y) > 170)
                element.y = lastAZ.y
            if (Math.abs(element.z - lastAZ.z) > 170)
                element.z = lastAZ.z
        }
        azinit = true;
        accDataAZ.push(element)
    }
    for (let index = 0; index < accBufSixeEL; index++) {
        let element = {}
        element.x = TwosCompliment16bit(buf[i++], buf[i++])
        element.y = TwosCompliment16bit(buf[i++], buf[i++])
        element.z = TwosCompliment16bit(buf[i++], buf[i++])
        if (Math.abs(element.x) > 500)
            element.x = lastEL.x
        if (Math.abs(element.y) > 500)
            element.y = lastEL.y
        if (Math.abs(element.z) > 500)
            element.z = lastEL.z
        if (elinit) {
            if (Math.abs(element.x - lastEL.x) > 170)
                element.x = lastEL.x
            if (Math.abs(element.y - lastEL.y) > 170)
                element.y = lastEL.y
            if (Math.abs(element.z - lastEL.z) > 170)
                element.z = lastEL.z
        }
        elinit = true;
        accDataEL.push(element)
    }
    for (let index = 0; index < accBufSixeBAL; index++) {
        let element = {}
        element.x = TwosCompliment16bit(buf[i++], buf[i++])
        element.y = TwosCompliment16bit(buf[i++], buf[i++])
        element.z = TwosCompliment16bit(buf[i++], buf[i++])
        if (Math.abs(element.x) > 500)
            element.x = lastBAL.x
        if (Math.abs(element.y) > 500)
            element.y = lastBAL.y
        if (Math.abs(element.z) > 500)
            element.z = lastBAL.z
        if (balinit) {
            if (Math.abs(element.x - lastBAL.x) > 170)
                element.x = lastBAL.x
            if (Math.abs(element.y - lastBAL.y) > 170)
                element.y = lastBAL.y
            if (Math.abs(element.z - lastBAL.z) > 170)
                element.z = lastBAL.z
        }
        balinit = true;
        accDataBAL.push(element)
    }
    for (let j = 0; j < AZTempSize; j++) {
        let element = buf[i++];
        element += (buf[i++] << 8)
        AZtempData.push(element);
    }
    for (let j = 0; j < ELTempSize; j++) {
        let element = buf[i++];
        element += (buf[i++] << 8)
        ELtempData.push(element);
    }
    accbufferAZ = accDataAZ
    accbufferEL = accDataEL
    accbufferBAL = accDataBAL
    AZTEmpBuffer = AZtempData
    ELTEmpBuffer = ELtempData
    //console.log(`az${AZTEmpBuffer.length}   el${ELTEmpBuffer.length}    azacc${accbufferAZ.length}   elacc${accbufferEL.length}`)
}

// Include Nodejs' net module.
const Net = require('net');

// Use net.createServer() in your code. This is just for illustration purpose.
// Create a new TCP server.
const server = new Net.Server();
// The server listens to a socket for a client to make a connection request.
// Think of a socket as an end point.
server.listen(tcpport, selfIP, function () {
    console.log(`Server listening for tcp connection localhost: ${JSON.stringify(server.address())}`);
});
// When a client requests a connection with the server, the server creates a new
// socket dedicated to that client.
server.on('connection', function (socket) {
    server.address();
    let bigdata = false;
    let smalldata = false;
    let arr = Buffer.alloc(1);
    let expectedSize = -1;
    let parsed = false;
    let adr = ""
    // The server can also receive data from the client by reading from its socket.
    socket.on('data', function (chunk) {
        //console.log(`Data received from client: ${chunk.length} bytes long`);
        //console.log(chunk);
        if (bigdata) {
            arr = Buffer.concat([arr, chunk]);
            if (expectedSize == -1 && arr.length > 11) {
                expectedSize = arr[4]
                expectedSize += (arr[3] << 8)
                expectedSize += (arr[2] << 16)
                expectedSize += (arr[1] << 24)
            }
            else if (expectedSize != -1 && arr.length >= expectedSize - 1) {
                //console.log(arr);
                if (!parsed) {
                    //console.log(`data connection established with ${expectedSize} expected bytes`);
                    parsed = true;
                    parseBigData(arr);
                }
                socket.end();
            }
        } else if (smalldata) {
            arr = Buffer.concat([arr, chunk]);
            //console.log(arr);
        }
        else if (chunk[0] === 131) {
            bigdata = true;
            arr = chunk;
            adr = socket.remoteAddress
            if (expectedSize == -1 && arr.length > 11) {
                expectedSize = arr[4]
                expectedSize += (arr[3] << 8)
                expectedSize += (arr[2] << 16)
                expectedSize += (arr[1] << 24)
            }
            //console.log(`data connection established with ${expectedSize} bytes of data first backet:${chunk.length}`);
        }
        else if (chunk[0] === selfDev.OBJID) {
            smalldata = true
            arr = chunk;
            console.log(`tcp connect from ${socket.remoteAddress}`);
        } else {
            console.log(`unknown connection ${chunk.length}`);
        }
        //socket.write('Acknoledge');
    });

    // When the client requests to end the TCP connection with the server, the server
    // ends the connection.
    socket.on('end', function () {
        //console.log(`Closing connection with the client with data length ${expectedSize} and array size ${arr.length}`);
        if (!parsed && bigdata) {
             parsed = true;
              parseBigData(arr);
          }
        if (smalldata) {
            //console.log(adr);
            //console.log(arr);
            let dev1 = new pooldev(adr);
            dev1.From_Transmition(arr);
            console.log(dev1);
        }

    });

    // Don't forget to catch error, for your own sake.
    socket.on('error', function (err) {
        console.log(`Error: ${err}`);
    });
});


function broadcastNew() {
    let buf1 = Buffer.from("ESP32", 'ascii');
    var buf2 = Buffer.alloc(1);
    buf2.writeUInt8(17, 0);
    let buf3 = Buffer.from(selfIP, 'ascii');
    var buf = Buffer.concat([buf1, buf2, buf3]);
    UDP_Client.send(buf, 0, buf.length, UDPPORT, broadCastTarget);//"169.254.205.177"
}

function broadcastControlRoomDiscover() {
    let buf1 = Buffer.from("CONTROL", 'ascii');
    var buf2 = Buffer.alloc(1);
    buf2.writeUInt8(17, 0);
    let buf3 = Buffer.from("DISCOVER", 'ascii');
    var buf4 = Buffer.alloc(1);
    buf4.writeUInt8(17, 0);
    let buf5 = Buffer.from(selfIP, 'ascii');
    var buf = Buffer.concat([buf1, buf2, buf3, buf4, buf5]);
    UDP_Client.send(buf, 0, buf.length, UDPPORT, broadCastTarget);//"169.254.205.177"
}

function broadcastsetUpdateMode() {
    let buf1 = Buffer.from("CONTROL", 'ascii');
    var buf2 = Buffer.alloc(1);
    buf2.writeUInt8(17, 0);
    let buf3 = Buffer.from("SETUPDATE", 'ascii');
    var buf4 = Buffer.alloc(1);
    buf4.writeUInt8(17, 0);
    let buf5 = Buffer.from("true", 'ascii');
    var buf = Buffer.concat([buf1, buf2, buf3, buf4, buf5]);
    UDP_Client.send(buf, 0, buf.length, UDPPORT, broadCastTarget);//"169.254.205.177"
}

UDP_Client.bind(function () {
    UDP_Client.setBroadcast(true)
    UDP_Client.setMulticastTTL(255);
    //broadcast = setInterval(broadcastNew, 2500);//esp32 sym
    controlRoomBroadcast = setInterval(broadcastControlRoomDiscover, 2500);
});


const UDPsocket = dgram.createSocket({ type: "udp4", reuseAddr: true });
UDPsocket.bind(UDPPORT);
//socket.bind(UDPPORT, selfIP);
UDPsocket.on("listening", function () {
    UDPsocket.addMembership(MULTICAST_ADDR);
    const address = UDPsocket.address();
    console.log(
        `UDP socket listening on ${address.address}:${address.port} pid: ${
        process.pid
        }`
    );
});


UDPsocket.on("message", function (message, rinfo) {

    if (selfIP == rinfo.address) {
        return;
    }
    let strmessage = bin2String(message);

    //console.log(message)
    if (strmessage[0] === "DBG") {
        console.info(`debug from: ${rinfo.address}:${rinfo.port} - ${strmessage[1]}`);
        return;
    }
    //console.log(message);
    var seen = false;
    recivedfrom.forEach(element => {
        if (element.address == rinfo.address) {
            seen = true;
        }
    });
    //if (seen) return;
    //console.info(`Message from: ${rinfo.address}:${rinfo.port} - ${message}`);
    if (seen) return;
    recivedfrom.push({ address: rinfo.address, lastFrom: new Date().getTime() })
});


var fs = require('fs');
var path = require('path');
var http = require('http');
const { Console } = require('console');

//create a server object:
http.createServer(function (request, response) {
    var filePath = '.' + request.url;
    //console.log(request.url);
    if (request.url == '/data') {
        let respobj = { ACC: accbufferAZ, ACCEL: accbufferEL, ACCBAL: accbufferBAL, ELT: ELTEmpBuffer, AZT: AZTEmpBuffer }
        let respstr = JSON.stringify(respobj)
        //console.log(respobj)
        response.writeHead(200, { 'Content-Type': 'application/json' });
        response.end(respstr, 'utf-8');
    }
    else if (request.url == '/update') {
        //console.log(request);
        broadcastsetUpdateMode();
        response.end(JSON.stringify(recivedfrom), 'utf-8');
        //        response.end(JSON.stringify({ lol:'🧠'}));
    }
    else {
        if (filePath == './')
            filePath = './index.html';


        var extname = path.extname(filePath);
        var contentType = 'text/html';
        switch (extname) {
            case '.js':
                contentType = 'text/javascript';
                break;
            case '.css':
                contentType = 'text/css';
                break;
            case '.json':
                contentType = 'application/json';
                break;
            case '.png':
                contentType = 'image/png';
                break;
            case '.jpg':
                contentType = 'image/jpg';
                break;
            case '.wav':
                contentType = 'audio/wav';
                break;
        }

        fs.readFile(filePath, function (error, content) {
            if (error) {
                if (error.code == 'ENOENT') {
                    fs.readFile('./404.html', function (error, content) {
                        response.writeHead(200, { 'Content-Type': contentType });
                        response.end(content, 'utf-8');
                    });
                }
                else {
                    response.writeHead(500);
                    response.end('Sorry, check with the site admin for error: ' + error.code + ' ..\n');
                    response.end();
                }
            }
            else {
                response.writeHead(200, { 'Content-Type': contentType });
                response.end(content, 'utf-8');
            }
        });
    }
}).listen(8080);
//*/
'use strict';

var os = require('os');
var ifaces = os.networkInterfaces();
var selfIP = "169.254.205.47";
var broadCastTarget = "169.254.255.255";

function bin2String(array) {
    let result = [];
    let substr = "";
    let len = 0;
    for (var i = 0; i < array.length; i++) {
        if (array[i] == 17){
            result.push(substr);
            substr = "";
            len = 0;
        }else{
            substr += String.fromCharCode(array[i]);
            len++;
        }
    }
    if(len>0){
        result.push(substr);
    }
    return result;
}

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


var dgram = require('dgram');
var net = require('net');
var UDP_Client = dgram.createSocket("udp4");
const PORT = 8888;
const DBGPORT = 8000;

var tcpport = 1602;
var recivedfrom = []
var broadcast;
UDP_Client.bind(function () {
    UDP_Client.setBroadcast(true)
    UDP_Client.setMulticastTTL(255);
    broadcast = setInterval(broadcastNew, 1000);
});

console.log(selfIP);
console.log(broadCastTarget);

function broadcastNew() {
    let buf1 = Buffer.from("ESP32", 'ascii');
    var buf2 = Buffer.alloc(1);
    buf2.writeUInt8(17,0);
    let buf3 = Buffer.from(selfIP, 'ascii');
    var buf = Buffer.concat([buf1, buf2, buf3]);
    UDP_Client.send(buf, 0, buf.length, PORT, broadCastTarget);//"169.254.205.177"
}



const MULTICAST_ADDR = "233.255.255.255";// "255.255.255.0";//
//192.168.0.70

const process = require("process");
const socket = dgram.createSocket({ type: "udp4", reuseAddr: true });

socket.bind(PORT);

socket.on("listening", function () {
    socket.addMembership(MULTICAST_ADDR);
    const address = socket.address();
    console.log(
        `UDP socket listening on ${address.address}:${address.port} pid: ${
        process.pid
        }`
    );
});


socket.on("message", function (message, rinfo) {
    if (selfIP == rinfo.address) {
        return;
    }
    message = bin2String(message);
    //console.log(message)
    if (message[0] === "DBG"){
        console.info(`debug from: ${rinfo.address}:${rinfo.port} - ${message[1]}`);
        return;
    }
    var seen = false;
    recivedfrom.forEach(element => {
        if (element.address == rinfo.address) {
            seen = true;
            setTimeout(() => {
                clearInterval(broadcast)
            }, 5000)
        }
    });
    //if (seen) return;
    recivedfrom.push({ address: rinfo.address, lastFrom: new Date().getTime() })
    console.info(`Message from: ${rinfo.address}:${rinfo.port} - ${message}`);
    if (seen) return;
    var client = new net.Socket();
    client.connect(tcpport, rinfo.address, function () {
        console.log('Connected');
        //let obb = { adress: selfIP, name: "esp111", arr: [1, 3, 4, 6] };
        // client.write(JSON.stringify(obb));
        let buffer = new ArrayBuffer(258);

       /* let usernameView = new Uint8Array(buffer, 0, buffer.length);
        for (let index = 0; index < usernameView.length; index++) {
            usernameView[index] = index; "␇"
        }
        client.write(usernameView); */
    });

    client.on('data', function (data) {
        console.log('Received: ' + data);
        client.destroy(); // kill client after server's response
    });

    client.on('close', function () {
        console.log('Connection closed');
    });
});



/*

0   ␀
1   ␁
2   ␂
3   ␃
4   ␄
5   ␅
6   ␆
7   ␇
8
9
10

11   ␋
12   ␌
13
14   ␎
15   ␏
16   ␐
17   ␑
18   ␒
19   ␓
20   ␔
21   ␕
22   ␖
23   ␗
24   ␘
25   ␙
26   ␚
27   ␛
28   ␜
29   ␝
30   ␞
31   ␟

const tcpserv = net.createServer((socket) => {
    console.log('client connected');
    //console.log(socket)
    //socket.write('hello\r\n');
    //socket.pipe(socket);
    //socket.end('goodbye\n');
}).on('error', (err) => {
    // Handle errors here.
    throw err;
});

// Grab an arbitrary unused port.
tcpserv.listen(1602,"0.0.0.0",() => {
    console.log('opened server on', tcpserv.address());
});


tcpserv.on("connection", function (client) {  // on connection event, when someone connects
    console.log(tcpserv.connections); // write number of connection to the command line
    client.on("data", function (data) {   //event when a client writes data to the server
        console.log("_________________ ");
        console.log("Server received data: "); // log what the client sent
        console.log("Server received data: "+data); // log what the client sent
        console.log( data);
    });
});

*/
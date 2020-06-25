var dgram = require('dgram');
var net = require('net');
var server = dgram.createSocket("udp4");
var selfIP = "169.254.205.47";
const PORT = 8888;

var tcpport = 1602;
var recivedfrom =[]
var broadcast;
server.bind(function () {
    server.setBroadcast(true)
    server.setMulticastTTL(255);
    broadcast = setInterval(broadcastNew, 1000);
});



function broadcastNew() {
    var message = "ESP32::" + selfIP;
    server.send(message, 0, message.length, PORT, "169.254.255.255");//"169.254.205.177"
    //console.log("Sent " + message + " to the wire...");
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
    if (selfIP == rinfo.address){
        return;
    }
    var seen = false;
    recivedfrom.forEach(element => {
        if (element.address == rinfo.address){
            seen= true;
            setTimeout(()=>{
                clearInterval(broadcast)
            },5000)
        }
    });
    if(seen)return;
    recivedfrom.push({ address : rinfo.address, lastFrom : new Date().getTime()})
    console.info(`Message from: ${rinfo.address}:${rinfo.port} - ${message}`);
    var client = new net.Socket();
    client.connect(tcpport, rinfo.address, function () {
        console.log('Connected');
        let obb = { adress: selfIP, name: "esp111", arr: [1, 3, 4, 6] };
        client.write(JSON.stringify(obb));
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
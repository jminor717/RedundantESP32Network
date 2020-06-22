var dgram = require('dgram');
var server = dgram.createSocket("udp4");
server.bind(function () {
    server.setBroadcast(true)
    server.setMulticastTTL(255);
    setInterval(broadcastNew, 1000);
});



function broadcastNew() {
    var message = new Buffer("169.254.205.47::ESP32");
    server.send(message, 0, message.length, 8888, "169.254.255.255");//"169.254.205.177"
    console.log("Sent " + message + " to the wire...");
}



const PORT = 8888;
const MULTICAST_ADDR = "233.255.255.255";// "255.255.255.0";//
//192.168.0.70

const process = require("process");

const socket = dgram.createSocket({ type: "udp4", reuseAddr: true });

socket.bind(PORT);

socket.on("listening", function () {
    socket.addMembership(MULTICAST_ADDR);
    //setInterval(sendMessage, 5000);
    const address = socket.address();
    console.log(
        `UDP socket listening on ${address.address}:${address.port} pid: ${
        process.pid
        }`
    );
});

function sendMessage() {
    const message = Buffer.from(`Message from process ${process.pid}`);
    socket.send(message, 0, message.length, PORT, MULTICAST_ADDR, function () {
        console.info(`Sending message "${message}"`);
    });
}

socket.on("message", function (message, rinfo) {
    console.info(`Message from: ${rinfo.address}:${rinfo.port} - ${message}`);
});


/*

// Require dgram module.
var dgram = require('dgram');

// Create udp server socket object.
var server = dgram.createSocket("udp4");

// Make udp server listen on port 5005.
server.bind(5005);

// When udp server receive message.
server.on("message", function (message) {
    // Create output message.
    var output = "Udp server receive message : " + message + "\n";
    // Print received message in stdout, here is log console.
    process.stdout.write(output);
});

// When udp server started and listening.
server.on('listening', function () {
    // Get and print udp server listening ip address and port number in log console.
    var address = server.address();
    console.log('UDP Server started and listening on ' + address.address + ":" + address.port);
});*/
// app.js
const express = require('express');
var app = express();
var server = require('http').createServer(app);
var serveIndex = require('serve-index');
var mqtt = require('mqtt');
var chacha = require('chacha-js');
var io = require('socket.io').listen(server);

var mqtt_cli = mqtt.connect('mqtts://m24.cloudmqtt.com',{
    port: '22376',
    username: 'ikfemazr',
    password: 'bQklMK_j41al'
});

var bodyParser = require('body-parser');
app.use(bodyParser.json()); // support json encoded bodies
app.use(bodyParser.urlencoded({ extended: true })); // support encoded bodies


connections = []; //save all socket connections to this array

server.listen(5000);
socket_connect();
mqtt_connect();
mqtt_send();

function socket_connect() {
    io.sockets.on('connection', function (socket) {
        connections.push(socket);
        console.log("%s device(s) connected.", connections.length);

        // handle disconnect
        socket.on('disconnect', function (data) {
            connections.splice(connections.indexOf(socket), 1);
            console.log('%s device(s) disconnected', connections.length);
        });

        socket.on('repeat', function(data){
            mqtt_cli.publish('wemos/repeat', JSON.stringify(json_processed));
        });
    });
}

function mqtt_connect() {
    // mqtt connect

    mqtt_cli.on('connect', function () {

        // subscribe to wemos/sensors
        mqtt_cli.subscribe('wemos/encrypt', function (err) {
            // check subscribed or not
            if (!err) {
                console.log("Subscribed to wemos/sensors");
            }
        });

    });
}

function mqtt_send(){
    mqtt_cli.on('message', function (topic, message) {

        if (topic === 'wemos/encrypt'){
            console.log(topic + ':' + message.toString());
            
            json_received = JSON.parse(message.toString());
            json_processed = {
                "encrypted" : json_received.encrypted.toString().split(";"),
                "nonce" : json_received.nonce.toString().split(";"),
                "time" : json_received.time,
                "cycle" : json_received.cycle,
                "counter" : json_received.counter.toString().split(";"),
                "length": json_received.length,
            };

            mqtt_cli.publish('wemos/encrypt_decrypt', JSON.stringify(json_processed));
            io.emit('sensor', message.toString());
        }
    });

    mqtt_cli.on('message', function (topic, message) {

        if (topic === 'wemos/status'){
            console.log(topic + ':' + message.toString());
            io.emit('status', message.toString());
        }

    });
}


// ExpressJS routing
app.use('/public', express.static(__dirname + "/public"));

// GET method
app.get("/", function (req, res) {
    res.sendFile(__dirname + '/public/');
});

app.get("/beeper_status", function (req, res) {
    res.sendFile(__dirname + '/public/home_automation.html');
});

// POST method
app.post("/send-command", function (req, res) {
    var command = req.body.command;
    mqtt_cli.publish('wemos/command', command);
});

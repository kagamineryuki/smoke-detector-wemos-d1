// app.js
const express = require('express');
var app = express();
var server = require('http').createServer(app);
var serveIndex = require('serve-index');
var mqtt = require('mqtt');
var io = require('socket.io').listen(server);
// var mqtt_cli = mqtt.connect({
//     host: '192.168.88.3',
//     port: '1883',
//     username: 'kaze',
//     password: '5D3v1#vW'
// });
var mqtt_cli = mqtt.connect('mqtts://m24.cloudmqtt.com',{
    port: '22376',
    username: 'ikfemazr',
    password: '1sZHdnmhMuRc'
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

        socket.on('wemos/cmd', function(data){
            mqtt_cli.publish('wemos/cmd', data.toString());
        });
    });
}

function mqtt_connect() {
    // mqtt connect

    mqtt_cli.on('connect', function () {
        // subscribe to wemos/status
        mqtt_cli.subscribe('wemos/status', function (err) {
            // check subscribed or not
            if (!err) {
                console.log("Subscribed to wemos/status");
            }
        });

        // subscribe to wemos/sensors_dht
        mqtt_cli.subscribe('wemos/sensors_dht', function (err) {
            // check subscribed or not
            if (!err) {
                console.log("Subscribed to wemos/sensors_dht");
            }
        });

        
        // subscribe to wemos/sensors_mq2
        mqtt_cli.subscribe('wemos/sensors_mq2', function (err) {
            // check subscribed or not
            if (!err) {
                console.log("Subscribed to wemos/sensors_mq2");
            }
        });
    });
}

function mqtt_send(){
    mqtt_cli.on('message', function (topic, message) {
        
        if (topic === 'wemos/status'){
            console.log(topic + ':' + message.toString());
            io.emit('status', message.toString());
        }

        if (topic === 'wemos/sensors_dht'){
            console.log(topic + ':' + message.toString());
            io.emit('sensor_dht', message.toString());
        }

        if (topic === 'wemos/sensors_mq2'){
            console.log(topic + ':' + message.toString());
            io.emit('sensor_mq2', message.toString());
        }
        
    });
}


// ExpressJS routing
app.use('/public', express.static(__dirname + "/public"));

// GET method
app.get("/", function (req, res) {
    res.sendFile(__dirname + '/public/');
});

app.get("/home_automation", function (req, res) {
    res.sendFile(__dirname + '/public/home_automation.html');
});

// POST method
app.post("/send-command", function (req, res) {
    var command = req.body.command;
    mqtt_cli.publish('wemos/cmd', command);
});
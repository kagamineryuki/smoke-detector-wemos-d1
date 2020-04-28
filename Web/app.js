// app.js
const express = require('express');
var app = express();
var server = require('http').createServer(app);
var serveIndex = require('serve-index');
var mqtt = require('mqtt');
var io = require('socket.io').listen(server);

var mqtt_cli = mqtt.connect('mqtt://maqiatto.com',{
    port: '1883',
    username: 'kagamineryuki@gmail.com',
    password: 'kazekazekaze'
});

var bodyParser = require('body-parser');
app.use(bodyParser.json()); // support json encoded bodies
app.use(bodyParser.urlencoded({ extended: true })); // support encoded bodies

connections = []; //save all socket connections to this array

server.listen(5000);
mqtt_connect();
mqtt_send();

function mqtt_connect() {
    // mqtt connect

    mqtt_cli.on('connect', function () {

        // subscribe to kagamineryuki@gmail.com/aes-128/encrypt
        mqtt_cli.subscribe('kagamineryuki@gmail.com/aes-128/encrypt', function (err) {
            // check subscribed or not
            if (!err) {
                console.log("Subscribed to kagamineryuki@gmail.com/aes-128/encrypt");
            }
        });

    });
}

function mqtt_send(){
    mqtt_cli.on('message', function (topic, message) {

        if (topic === 'kagamineryuki@gmail.com/aes-128/encrypt'){
            console.log(topic + ':' + message.toString());
            
            json_received = JSON.parse(message.toString());
            var encrypted = json_received.encrypted.toString();
            encrypted = encrypted.split(";");
            json_processed = {
                "encrypted" : encrypted,
                "time" : json_received.time,
                "cycle" : json_received.cycle,
                "length": json_received.length,
                "aes_size" : json_received.aes_size,
                "machine_id" : json_received.machine_id,
                "encryption_type" : json_received.encryption_type
            };

            mqtt_cli.publish('kagamineryuki@gmail.com/aes-128/encrypt_decrypt', JSON.stringify(json_processed));
        }
    });

    mqtt_cli.on('message', function (topic, message) {

        if (topic === 'kagamineryuki@gmail.com/aes-128/decrypted'){
            console.log(topic + ':' + message.toString());

            json_received = JSON.parse(message.toString());
            json_processed = {
                "decrypted" : json_received.decrypted,
                "time" : json_received.time,
                "cycle" : json_received.cycle,
                "length": json_received.length,
                "machine_id" : json_received.machine_id,
                "encryption_type" : json_received.encryption_type
            };


        }
    });
}


// ExpressJS routing
app.use('/public', express.static(__dirname + "/public"));

// GET method
// app.get("/", function (req, res) {
//     res.sendFile(__dirname + '/public/');
// });

// app.get("/beeper_status", function (req, res) {
//     res.sendFile(__dirname + '/public/home_automation.html');
// });

// POST method
// app.post("/send-command", function (req, res) {
//     var command = req.body.command;
//     mqtt_cli.publish('wemos/command', command);
// });

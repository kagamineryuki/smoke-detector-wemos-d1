// app.js
const express = require('express');
var app = express();
var server = require('http').createServer(app);
var serveIndex = require('serve-index');
var mqtt = require('mqtt');
var pg = require('pg').Pool;
var pool = new pg({
    user: 'kaze',
    host: '192.168.20.3',
    database: 'project',
    password: '5D3v1#vW',
    port: 5432,
})

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

        // subscribe to chacha20/encrypt
        mqtt_cli.subscribe('kagamineryuki@gmail.com/chacha20/encrypt', function (err) {
            // check subscribed or not
            if (!err) {
                console.log("Subscribed to kagamineryuki@gmail.com/chacha20/encrypt");
            }
        });
        
        // subscribe to chacha20/current
        mqtt_cli.subscribe('kagamineryuki@gmail.com/chacha20/current', function (err) {
            // check subscribed or not
            if (!err) {
                console.log("Subscribed to kagamineryuki@gmail.com/chacha20/current");
            }
        });

         // subscribe to chacha20/current
        mqtt_cli.subscribe('kagamineryuki@gmail.com/chacha20/decrypted', function (err) {
            // check subscribed or not
            if (!err) {
                console.log("Subscribed to kagamineryuki@gmail.com/chacha20/decrypted");
            }
        });
    });
}

function mqtt_send(){
    mqtt_cli.on('message', function (topic, message) {

        if (topic === 'kagamineryuki@gmail.com/chacha20/encrypt'){
            console.log(topic + ':' + message.toString());
            
            json_received = JSON.parse(message.toString());
            json_processed = {
                "encrypted" : json_received.encrypted.toString().split(";"),
                "nonce" : json_received.nonce.toString().split(";"),
                "counter" : json_received.counter.toString().split(";"),
                "time" : json_received.time,
                "cycle" : json_received.cycle,
                "length": json_received.length,
                "machine_id" : json_received.machine_id,
                "encryption_type" : json_received.encryption_type
            };

            mqtt_cli.publish('kagamineryuki@gmail.com/chacha20/encrypt_decrypt', JSON.stringify(json_processed));

            query = 'INSERT INTO encrypted(encrypted, cycle, time, length, machine_id, encryption_type) VALUES($1,$2,$3,$4,$5,$6) RETURNING *';
            values = [json_received.encrypted, json_received.cycle, json_received.time, json_received.length, json_received.machine_id, json_received.encryption_type];
            
            pool
            .query(query, values)
            .then(res => {
                console.log("Results : " + res.rows[0]);
            })
            .catch(e => console.log("error : " + e.stack));
        }

        if (topic === 'kagamineryuki@gmail.com/chacha20/decrypted'){
            console.log(topic + ':' + message.toString());

            json_received = JSON.parse(message.toString());
            json_processed = {
                "decrypted" : json_received.encrypted,
                "time" : json_received.time,
                "cycle" : json_received.cycle,
                "length": json_received.length,
                "machine_id" : json_received.machine_id,
                "encryption_type" : json_received.encryption_type
            };

            query = 'INSERT INTO decrypted(decrypted, cycle, time, length, machine_id, encryption_type) VALUES($1,$2,$3,$4,$5,$6)';
            values = [json_received.decrypted, json_received.cycle, json_received.time, json_received.length, json_received.machine_id, json_received.encryption_type];

            pool
            .query(query, values)
            .then(res => {
                console.log("Results : " + res.rows[0]);
            })
            .catch(e => console.log("error : " + e.stack));
        }

        if (topic === 'kagamineryuki@gmail.com/chacha20/current'){
            console.log(topic + ':' + message.toString());

            json_received = JSON.parse(message.toString());
            json_processed = {
                "decrypted" : json_received.current,
                "machine_id" : json_received.machine_id,
                "encryption_type" : json_received.encryption_type
            };

            query = 'INSERT INTO current (current, machine_id, encryption_type) VALUES ($1,$2,$3)';
            values = [json_received.current, json_received.machine_id, json_received.encryption_type];

            pool
            .query(query, values)
            .then(res => {
                console.log("Results : " + res.rows[0]);
            })
            .catch(e => console.log("error : " + e.stack));
        }
    });
}


// ExpressJS routing
app.use('/public', express.static(__dirname + "/public"));

// GET method
app.get("/", function (req, res) {
    res.sendFile(__dirname + '/public/');
});

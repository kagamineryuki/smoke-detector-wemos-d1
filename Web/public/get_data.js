$(document).ready(function () {
    var socket = io.connect();

    var switches_status = {
        "relay_1": false,
        "relay_2": false
    }

    $('#switch-1-on').on('click', function(){
        switches_status.relay_1 = true;
        Send_Mqtt_Cmd();
    });

    $('#switch-1-off').on('click', function(){
        switches_status.relay_1 = false;
        Send_Mqtt_Cmd();
    });
    
    $('#switch-2-on').on('click', function(){
        switches_status.relay_2 = true;
        Send_Mqtt_Cmd();
    });

    $('#switch-2-off').on('click', function(){
        switches_status.relay_2 = false;
        Send_Mqtt_Cmd();
    });

    socket.on('status', function(data){
        var msg = JSON.parse(data.toString());

        Switch_Status(msg);

    });

    // change each switch status accordingly
    function Switch_Status(msg){
        // relay 1
        if (msg.relay_1_status == true){
            $('#switch-1-status').text("ON");
        } else if (msg.relay_1_status == false){
            $('#switch-1-status').text("OFF");
        } else {
            $('#switch-1-status').text("NaN");
        }

        // relay 2
        if (msg.relay_2_status == true){
            $('#switch-2-status').text("ON");
        } else if (msg.relay_2_status == false){
            $('#switch-2-status').text("OFF");
        } else {
            $('#switch-2-status').text("NaN");
        }

    }

    // action for each on/off switch button
    function Send_Mqtt_Cmd(){
        socket.emit('wemos/cmd', JSON.stringify(switches_status));
    }
});
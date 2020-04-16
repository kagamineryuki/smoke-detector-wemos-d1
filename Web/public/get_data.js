$(document).ready(function () {
    var socket = io.connect();

    var switches_status = {
        "beeper": false
    }

    $('#switch-1-on').on('click', function(){
        switches_status.beeper = true;
        Send_Mqtt_Cmd();
    });

    $('#switch-1-off').on('click', function(){
        switches_status.beeper = false;
        Send_Mqtt_Cmd();
    });

    socket.on('status', function(data){
        var msg = JSON.parse(data.toString());

        Switch_Status(msg);

    });

    // change each switch status accordingly
    function Switch_Status(msg){
        // beeper
        if (msg.beeper == true){
            $('#switch-1-status').text("ON");
        } else if (msg.beeper == false){
            $('#switch-1-status').text("OFF");
        } else {
            $('#switch-1-status').text("NaN");
        }

    }

    // action for each on/off switch button
    function Send_Mqtt_Cmd(){
        socket.emit('command', JSON.stringify(switches_status));
    }
});

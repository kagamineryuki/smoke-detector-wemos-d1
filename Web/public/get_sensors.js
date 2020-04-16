$(document).ready(function () {
    var socket = io.connect();

    socket.on('sensor', function(data){
        var msg = JSON.parse(data);
        $('#temp-input').val(msg.temperature + " °C");
        $('#humidity-input').val(msg.humidity + " %");
        $('#dew-input').val(msg.dew_point + " °C");
        $('#abs-humidity-input').val(msg.abs_humidity + " g/m³");
        $('#lpg-input').val(msg.lpg + " PPM");
        $('#methane-input').val(msg.methane + " PPM");
        $('#hydrogen-input').val(msg.hydrogen + " PPM");
        $('#smoke-input').val(msg.smoke + " PPM");
    });
});

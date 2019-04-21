var net = require('net');

var port = 10001;
var client = new net.Socket();

client.setEncoding('utf8');

var connect = function () {
    client.connect(port, '192.168.1.99');
}

connect();

process.stdin.resume();
process.stdin.on('data', function (data) {
    client.write(data);
});

client.on('data', function (data) {
    console.log('client: ' + data);
});

client.on('close', function () {
    console.log('client: connection is closed');
    setTimeout(connect, 6000);
});

client.on('error', function () {
    console.log('client: made error');
});

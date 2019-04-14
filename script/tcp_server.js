require('date-utils');

var net = require('net');
var port = 10001;
var server;

var dt = new Date();
var formatted = dt.toFormat("HH24:MI:SS");
var startTime=0;
var endTime;
var len=0;

var fs = require('fs');
var fname = "TCP_TEST"
var wdata;


// File Write Function
function writeFile(path, data) {
    fs.appendFile(path, data, function (err) {
	if (err) {
            throw err;
	}
    });
}


server = net.createServer(function(conn){
    console.log('server-> tcp server created');

    conn.on('data', function(data){
	endTime = new Date();
	len = len + data.length;
	
	if( endTime - startTime > 1000 ){
	    dt = new Date();
	    formatted = dt.toFormat("HH24:MI:SS");
	    console.log(formatted, Math.floor(len*8/(endTime-startTime)), "Kbps");
	    wdata = formatted + "," + Math.floor(len*8/(endTime-startTime)) + "Kbps\r\n";
	    writeFile(fname, wdata);
	    len = 0;
	    startTime = endTime;
	}
    });
	
    conn.on('close', function(){
	console.log('server-> client closed connection');
    });

    
}).listen(port);


if( process.argv.length > 2 ){
    fname = fname + "_" + process.argv[2] + ".log";
}
else{
    console.log('usage: node tcp_server.js <log file name>');
    testtime = new Date();
    formatted = testtime.toFormat("HH24MISS");
    fname = fname + "_" + formatted + ".log";
}

console.log('listening on port %d', port);

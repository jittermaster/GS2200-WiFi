var http = require('http');
//var fs = require('fs');
var querystring = require('querystring');
var url = require('url');
var data;
var postData;
 
var server = http.createServer(function(req, res) {
    // Access '/', response back with the latest postData
    if (req.url === '/' && req.method === 'GET') {
	console.log( 'GET Response :', postData['data']);
	res.end( postData['data'] );
    }
    // Access '/postPage' by POST method
    else if (req.url === '/postData' && req.method === 'POST') {
	data = '';
	req.on('data', function(chunk) {
	    data += chunk;
	});
	req.on('end', function() {
	    console.log( 'Recv Size :', data.length );
	    res.end('OK');
	});
    }
    else {
	res.statusCode = 404;
	res.end('NotFound');
    }
});

	  
// Listen port 10080
server.listen(10080);

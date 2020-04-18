var express = require('express');
var app = express.createServer();

app.configure(function () {
    app.use(express.static(__dirname + '/../'));
    app.use(express.logger());
    app.use(app.router);
});

app.configure('development', function(){
    app.use(express.errorHandler({
        dumpExceptions: true,
        showStack: true
    }));
});

app.configure('production', function(){
    app.use(express.errorHandler());
});

var UPLOAD_DIRECTORY = __dirname + '/uploads';

function saveUpload(data) {
    var filename = UPLOAD_DIRECTORY + '/' + (data.date + '_' + data.ip).replace(/[^A-Za-z0-9_.-]/g, '_') + '.json';
    require('fs').writeFile(filename, JSON.stringify(data) + '\n', 'utf8');
}

app.post('/results', function (req, res) {
    var date = Date.now();

    var jsonData = '';
    req.on('data', function (data) {
        jsonData += data.toString('utf8');
    });

    req.on('end', function () {
        var userData = JSON.parse(jsonData);
        saveUpload({
            date: date,
            ip: req.connection.remoteAddress,
            userData: userData
        });
        res.end();
    });

    req.resume();
});

app.listen(3002);
console.log('Listening on :3002');

var jsBase = __dirname + '/../js';
var un = require(jsBase + '/unrequire');
un.require.config({ baseUrl: jsBase });

var input = process.stdin;
var output = process.stdout;

un.require([ 'fullReport' ], function (fullReport) {
    var jsonData = '';
    input.on('data', function (data) {
        jsonData += data.toString('utf8');
    });

    input.on('end', function () {
        var data = JSON.parse(jsonData);
        var csv = fullReport.csvReport(data.userData.results, data.userData.agentMetadata);
        output.write(csv);
    });
    input.resume();
});

var NUMERIC_SORT = function(a,b){ return a-b; };

var fs = require('fs');
var path = require('path');

var jsBase = __dirname + '/../js';
var un = require(jsBase + '/unrequire');
un.require.config({ baseUrl: jsBase });

var dataStructure = {};

var basicDataStructure = {};

var fileCount = 0;

var THING = 'sprites';
var PARAM = 'image';

var RESULTS_DIR = path.join(__dirname, '..', 'results');
var INPUT_DIR = path.join(RESULTS_DIR, 'raw');
var OUTPUT_DIR = path.join(RESULTS_DIR);

un.require( ['util/report'], function(report){
    fs.readdir( INPUT_DIR, function(err, files){
        if( err ){
            throw err;
        }
        var toProcess = files.filter( function(filename){
            return filename.match(/\.json$/);
        });
    
        fileCount = toProcess.length;
    
        toProcess.forEach( function(filename){
            fs.readFile(path.join(INPUT_DIR, filename), 'utf8', function (err, json) {
                if( err ){
                    throw err;
                }
                var data = JSON.parse(json);
                var name = data.userData.agentMetadata.name;
                var browser = data.userData.agentMetadata.browser;
                var userAgent = data.userData.agentMetadata.userAgent;
                var type = data.userData.agentMetadata.type;
                var combo = type + "-" + browser;
                
                var row = [name];
                var columnLabels = [""];
                
                // keys are techniques - like css2DImg
                Object.keys(data.userData.results[THING][PARAM]).sort().forEach( function(key){
                    var spriteResults = data.userData.results[THING][PARAM][key];
                    
                    // testTypes are things like "scale", "rotation", and "translation"
                    Object.keys( spriteResults ).sort().forEach( function(testType){
                        var testResults = spriteResults[testType];
                        var vals = [null];
                        if( testResults ){
                            vals = [testResults.objectCount];
                        
                            if(!basicDataStructure[type]){
                                basicDataStructure[type] = {};
                            }
                            if(!basicDataStructure[type][browser]){
                                basicDataStructure[type][browser] = {};
                            }
                            if(!basicDataStructure[type][browser][key]){
                                basicDataStructure[type][browser][key] = {};
                            }
                            if(!basicDataStructure[type][browser][key][name]){
                                basicDataStructure[type][browser][key][name] = {};
                            }
                            basicDataStructure[type][browser][key][name][testType] = testResults.objectCount;
                        }
                        /*
                        columnLabels = columnLabels.concat( key );
                        row = row.concat( vals );
                        */
                    });
                });
                /*
                dataStructure[combo][2] = columnLabels;
                dataStructure[combo].push( row );
                */
                fileCount -= 1;
                if( fileCount === 0 ){
                    var spreadsheet = createSpreadsheet();
                    
                    saveSpreadsheet( spreadsheet );
//                    foo();
//                    bar();
                }
            });
        });
    });

    function createSpreadsheet(){
        var spreadsheet = [];
        
        dataStructure = Object.keys(basicDataStructure).map( function(type){
            // type is something like Phone / Tablet / Laptop
            
            spreadsheet.push( [type] );
            
            Object.keys( basicDataStructure[type] ).map( function(browser){
                // browser is something like Safari or Firefox
                
                spreadsheet.push( [,browser] );
                
                var summaryOfTechniques = [];
                var allEncounteredDevices = [];
                
                Object.keys( basicDataStructure[type][browser] ).map( function(renderingTechnique){
                    // renderingTechnique is something like css2dImg
                    
                    spreadsheet.push( [,,renderingTechnique] );
                    spreadsheet.push( [,,,"Device","translate","scale","rotate"] );
                    
                    var summaryOfTechnique = {"technique":renderingTechnique, "deviceScores":{}};
                    
                    Object.keys( basicDataStructure[type][browser][renderingTechnique] ).map( function(device){
                        // Device is something like the iPhone 4S
                        
                        if( allEncounteredDevices.indexOf( device ) === -1 ){
                            allEncounteredDevices.push( device );
                        }
                        
                        var dataRow = [,,,device];
                        var sumOfTestTypes = undefined;
                        
                        // Object.keys( basicDataStructure[type][browser][renderingTechnique][device] )
                        ['translate', 'scale', 'rotate'].map( function(testType){
                            // testType is something like 'translate', 'scale', or 'rotate;
                            var dataPoint = basicDataStructure[type][browser][renderingTechnique][device][testType];
                            dataRow.push( dataPoint );
                            
                            sumOfTestTypes = sumOfTestTypes || 0;
                            sumOfTestTypes += dataPoint;
                        });
                        
                        spreadsheet.push( dataRow );
                        
                        summaryOfTechnique.deviceScores[ device ] = sumOfTestTypes;
                    });
                    
                    spreadsheet.push( [] );
                    
                    summaryOfTechniques.push( summaryOfTechnique );
                });
                
                spreadsheet.push( [,"Summary"] );
                spreadsheet.push( [,].concat(allEncounteredDevices) );
                for( var i = 0; i < summaryOfTechniques.length; i++ ){
                    var summary = summaryOfTechniques[i];
                    var summaryRow = [summary.technique];
                    for( var j = 0; j < allEncounteredDevices.length; j++ ){
                        var encounteredDevice = allEncounteredDevices[j];
                        summaryRow.push( summary.deviceScores[encounteredDevice] )
                    }
                    spreadsheet.push( summaryRow );
                }
                spreadsheet.push( [] );
            })
        });
        
        return spreadsheet;
    }
    
    function saveSpreadsheet(spreadsheet){
        fs.writeFile( path.join(OUTPUT_DIR, THING + "-" + PARAM + "-RAW.csv"), report.csvByTable(spreadsheet), 'utf8', function(err){
            if(err) throw err;
            console.log("saved RAW!");
        } );
    }
    

    function foo(){
        var toSave = [];
        var roundUp = [[], ["","best of best device","average of each device's best","best of worst device"]];
        Object.keys(dataStructure).forEach( function(combo){
            var rows = dataStructure[combo];
            var averages = ['averages'];
            for( var i = 1; i < rows[2].length; i++ ){
                var sum = 0;
                var legitCount = 0;
                for( var j = 3; j < rows.length; j++ ){
                    var temp = rows[j][i];
                    if( temp !== null ){
                        legitCount += 1;
                        sum += temp;
                    }
                }
                var avg = sum / legitCount;
                averages[i] = avg;
            }
            
            var bestTheWorstPhoneCanDo = Infinity;
            var bestTheBestPhoneCanDo = -Infinity;
            var averageOfEachBestTechnique = 0;
            var phoneCount = 0;
            for( var j = 3; j < rows.length; j++ ){
                phoneCount += 1;
                var maxForPhone = -Infinity;
                for( var i = 1; i < rows[2].length; i++ ){
                    var temp = rows[j][i];
                    if( temp !== null ){
                        maxForPhone = Math.max( maxForPhone, temp );
                    }
                }
                
                bestTheWorstPhoneCanDo = Math.min( bestTheWorstPhoneCanDo, maxForPhone );
                bestTheBestPhoneCanDo = Math.max( bestTheBestPhoneCanDo, maxForPhone );
                averageOfEachBestTechnique += maxForPhone;
            }
            averageOfEachBestTechnique = (averageOfEachBestTechnique / phoneCount);
            
            toSave = toSave.concat( rows, [averages] );
            roundUp.push( [combo, bestTheBestPhoneCanDo, averageOfEachBestTechnique, bestTheWorstPhoneCanDo] );
        });
        console.log( roundUp );
        toSave = toSave.concat( roundUp );
        
        fs.writeFile( path.join(OUTPUT_DIR, THING + "-" + PARAM + ".csv"), report.csvByTable(toSave), 'utf8', function(err){
            if(err) throw err;
            console.log("saved!");
        } );
    }
    
    function bar(){
        var output = [];
        Object.keys( basicDataStructure ).sort().forEach( function(type){
            output.push( [type] );
            Object.keys( basicDataStructure[type] ).sort().forEach( function(browser){
                output.push( ["", browser] );
                Object.keys( basicDataStructure[type][browser] ).sort().forEach( function(techniqueName){
                    output.push( ["", "", techniqueName] );
                    
                    var uniqueXValues = [];
                    
                    Object.keys( basicDataStructure[type][browser][techniqueName] ).sort().forEach( function(deviceName){
                        var data = basicDataStructure[type][browser][techniqueName][deviceName];
                        
                        data.forEach( function(tuple){
                            var objectCount = tuple[0];
                            if( uniqueXValues.indexOf(objectCount) === -1 ){
                                uniqueXValues.push( objectCount );
                            }
                        });
                    });
                    
                    uniqueXValues.sort( NUMERIC_SORT );
                    
                    output.push( ["", "", "", ""].concat( uniqueXValues ) );
                    
                    Object.keys( basicDataStructure[type][browser][techniqueName] ).sort().forEach( function(deviceName){
                        var data = basicDataStructure[type][browser][techniqueName][deviceName];
                        
                        var yValues = uniqueXValues.map( function(numObjects){
                            var fps = fpsForObjectCount(data, numObjects);
                            if( fps === null ){
                                return '';
                            }else{
                                return fps;
                            }
                        });
                        
                        output.push( ["", "", "", deviceName].concat( yValues ) );
                    });
                });
            });
        });
        
        
        fs.writeFile( path.join(OUTPUT_DIR, THING + "-" + PARAM + "-RAW.csv"), report.csvByTable(output), 'utf8', function(err){
            if(err) throw err;
            console.log("saved RAW!");
        } );    
    }
});

function fpsForObjectCount( data, objectCount ){
    for( var i = 0; i < data.length; i++ ){
        if( data[i][0] === objectCount ){
            return data[i][2];
        }
    }
    return null;
}

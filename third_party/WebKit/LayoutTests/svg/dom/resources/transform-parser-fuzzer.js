if (window.testRunner)
    testRunner.dumpAsText();

var characters = [
    "0",
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    ".",
    "e",
    "+",
    "-",
    "e",
    "(",
    ")",
    " ",
    "\t",
    ","
];

var gElement = document.createElementNS("http://www.w3.org/2000/svg", "g");

function parseTransform(string) {
    gElement.setAttributeNS(null, "transform", string);
}

function fuzzTransforms(transforms) {
    for (var transform in transforms) {
        // Too few / too many arguments
        for (var i = 0; i < 50; i++) { //>
            var transformString = transform + "(";
            for (var j = 0; j < i; j++) { //>
                transformString += "0";
                if (j < i - 1) //>
                    transformString += ",";
            }
            transformString += ")";
            parseTransform(transformString);
        }

        // Random assortments of valid characters
        for (var i = 0; i < 100; i++) { //>
            var transformString = transform + "(";
            var count = Math.scriptedRandomInt(20);
            for (var j = 0; j < count; j++) { //>
                transformString += characters[Math.scriptedRandomInt(characters.length)];
            }
            parseTransform(transformString);
        }

        // Transform names that are "off by one"
        var extraChar = transform.charAt(transform.length - 1);
        parseTransform(transform + extraChar + "(0, 0)");
        parseTransform(transform.substring(0, transform.length - 1) + "(0, 0)");

        // Empty-ish transforms
        parseTransform(transform);
        parseTransform(transform + String.fromCharCode(0));
        parseTransform(transform + "(" + String.fromCharCode(0) + ")");
    }
}

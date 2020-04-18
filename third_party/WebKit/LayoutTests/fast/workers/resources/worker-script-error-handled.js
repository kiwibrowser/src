onerror = function(message, url, lineno, column)
{
    postMessage("onerror invoked for a script that has script error '" + message + "' at line " + lineno + ":" + column);
    return true;
}

if (true) foo.bar = 0;

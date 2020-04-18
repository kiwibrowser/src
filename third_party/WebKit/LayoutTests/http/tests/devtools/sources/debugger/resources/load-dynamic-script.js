function loadDynamicScript()
{
    var scriptElement = document.createElement("script");
    scriptElement.textContent = "function bar() { } \nconsole.log(\"DYNAMIC <script>\");";
    document.head.appendChild(scriptElement);
}

document.write("<scrip" + "t>function foo() { } \nconsole.log(\"DYNAMIC document.write()\");</sc" + "ript>");
loadDynamicScript();
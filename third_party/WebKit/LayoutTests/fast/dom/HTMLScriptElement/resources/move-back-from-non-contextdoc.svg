<?xml version="1.0"?>
<svg xmlns="http://www.w3.org/2000/svg" width="400" height="400">
<script>
function init()
{
    var script = document.createElementNS("http://www.w3.org/2000/svg", "script");
    script.setAttributeNS("http://www.w3.org/1999/xlink", "href", "does-not-exist.js");
    document.documentElement.appendChild(script);
    // otherDoc's contextDocument is document.
    var otherDoc = document.implementation.createDocument("", null);
    otherDoc.adoptNode(script);
    document.documentElement.appendChild(script);
    window.top.done();
}

init();
</script>
</svg>

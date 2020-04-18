function removeScriptElement() {
    var s = document.getElementById('theScript');
    s.parentNode.removeChild(s);
}

removeScriptElement();
gc();

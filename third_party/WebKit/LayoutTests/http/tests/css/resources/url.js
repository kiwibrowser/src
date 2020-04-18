function baseURL() {
    return window.location.protocol + "//" + window.location.host + window.location.pathname.substr(0, window.location.pathname.lastIndexOf("/") + 1);
}

function absoluteURL(url) {
    for ( ; ; ) {
        // Not support "./".
        var match = url.match(/^(.*\/)[^.][^/]*\/\.\.\/(.*)$/);
        if (match == null)
           return url;
        url = match[1] + match[2];
    }
    return url;
}

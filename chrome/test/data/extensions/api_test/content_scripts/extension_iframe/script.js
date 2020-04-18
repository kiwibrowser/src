var ifr = document.createElement("iframe");
ifr.src = chrome.extension.getURL("iframe.html");
document.body.appendChild(ifr);

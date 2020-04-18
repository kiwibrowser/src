/* Do not copy this file. Instead, do something like this in your
   own code.

  if (!window.WebGL2RenderingContext) {
     // Browser has no idea what WebGL is. Suggest they
     // get a new browser by presenting the user with link to
     // http://get.webgl.org/webgl2
     return;
  }

  gl = canvas.getContext("webgl2");
  if (!gl) {
    // Browser could not initialize WebGL. User probably needs to
    // update their drivers or get a new browser. Present a link to
    // http://get.webgl.org/webgl2/troubleshooting
    return;
  }

*/

var BrowserDetect = {
  init: function () {
    var info = this.searchString(this.dataBrowser) || {identity:"unknown"}
    this.browser = info.identity;
    this.version = this.searchVersion(navigator.userAgent)
        || this.searchVersion(navigator.appVersion)
        || "an unknown version";
    this.platformInfo = this.searchString(this.dataPlatform) || this.dataPlatform["unknown"];
    this.platform = this.platformInfo.identity;
    var browserInfo = this.urls[this.browser];
    if (!browserInfo) {
      browserInfo = this.urls["unknown"];
    } else if (browserInfo.platforms) {
      var info = browserInfo.platforms[this.platform];
      if (info) {
        browserInfo = info;
      }
    }
    this.urls = browserInfo;
  },
  searchString: function (data) {
    for (var i = 0; i < data.length; i++){
      var info = data[i];
      var dataString = info.string;
      var dataProp = info.prop;
      this.versionSearchString = info.versionSearch || info.identity;
      if (dataString) {
        if (dataString.indexOf(info.subString) != -1) {
          var shouldExclude = false;
          if (info.excludeSubstrings) {
            for (var ii = 0; ii < info.excludeSubstrings.length; ++ii) {
              if (dataString.indexOf(info.excludeSubstrings[ii]) != -1) {
                shouldExclude = true;
                break;
              }
            }
          }
          if (!shouldExclude)
            return info;
        }
      } else if (dataProp) {
        return info;
      }
    }
  },
  searchVersion: function (dataString) {
    var index = dataString.indexOf(this.versionSearchString);
    if (index == -1) {
      return;
    }
    return parseFloat(dataString.substring(
        index + this.versionSearchString.length + 1));
  },
  dataBrowser: [
  { string: navigator.userAgent,
    subString: "Chrome",
    excludeSubstrings: ["OPR/", "Edge/"],
    identity: "Chrome"
  },
  { string: navigator.userAgent,
    subString: "OmniWeb",
    versionSearch: "OmniWeb/",
    identity: "OmniWeb"
  },
  { string: navigator.vendor,
    subString: "Apple",
    identity: "Safari",
    versionSearch: "Version"
  },
  { string: navigator.vendor,
    subString: "Opera",
    identity: "Opera"
  },
  { string: navigator.userAgent,
    subString: "Android",
    identity: "Android"
  },
  { string: navigator.vendor,
    subString: "iCab",
    identity: "iCab"
  },
  { string: navigator.vendor,
    subString: "KDE",
    identity: "Konqueror"
  },
  { string: navigator.userAgent,
    subString: "Firefox",
    identity: "Firefox"
  },
  { string: navigator.vendor,
    subString: "Camino",
    identity: "Camino"
  },
  {// for newer Netscapes (6+)
    string: navigator.userAgent,
    subString: "Netscape",
    identity: "Netscape"
  },
  { string: navigator.userAgent,
    subString: "Edge/",
    identity: "Edge"
  },
  { string: navigator.userAgent,
    subString: "MSIE",
    identity: "Explorer",
    versionSearch: "MSIE"
  },
  { // for IE11+
    string: navigator.userAgent,
    subString: "Trident",
    identity: "Explorer",
    versionSearch: "rv"
  },
  { string: navigator.userAgent,
    subString: "Gecko",
    identity: "Mozilla",
    versionSearch: "rv"
  },
  { // for older Netscapes (4-)
    string: navigator.userAgent,
    subString: "Mozilla",
    identity: "Netscape",
    versionSearch: "Mozilla"
  }
  ],
  dataPlatform: [
  { string: navigator.platform,
    subString: "Win",
    identity: "Windows",
    browsers: [
      {url: "http://www.mozilla.com/en-US/firefox/new/", name: "Mozilla Firefox"},
      {url: "http://www.google.com/chrome/", name: "Google Chrome"},
    ]
  },
  { string: navigator.platform,
    subString: "Mac",
    identity: "Mac",
    browsers: [
      {url: "http://www.mozilla.com/en-US/firefox/new/", name: "Mozilla Firefox"},
      {url: "http://www.google.com/chrome/", name: "Google Chrome"},
    ]
  },
  { string: navigator.userAgent,
    subString: "iPhone",
    identity: "iPhone/iPod",
    browsers: [
      //{url: "http://www.mozilla.com/en-US/firefox/new/", name: "Mozilla Firefox"}
    ]
  },
  { string: navigator.platform,
    subString: "iPad",
    identity: "iPad",
    browsers: [
      //{url: "http://www.mozilla.com/en-US/firefox/new/", name: "Mozilla Firefox"}
    ]
  },
  { string: navigator.userAgent,
    subString: "Android",
    identity: "Android",
    browsers: [
      {url: "https://play.google.com/store/apps/details?id=org.mozilla.firefox", name: "Mozilla Firefox"},
      {url: "https://play.google.com/store/apps/details?id=com.android.chrome", name: "Google Chrome"}
    ]
  },
  { string: navigator.platform,
    subString: "Linux",
    identity: "Linux",
    browsers: [
      {url: "http://www.mozilla.com/en-US/firefox/new/", name: "Mozilla Firefox"},
      {url: "http://www.google.com/chrome/", name: "Google Chrome"},
    ]
  },
  { string: "unknown",
    subString: "unknown",
    identity: "unknown",
    browsers: [
      {url: "http://www.mozilla.com/en-US/firefox/new/", name: "Mozilla Firefox"},
      {url: "http://www.google.com/chrome/", name: "Google Chrome"},
    ]
  }
  ],
  /*
  upgradeUrl:         Tell the user how to upgrade their browser.
  troubleshootingUrl: Help the user.
  platforms:          Urls by platform. See dataPlatform.identity for valid platform names.
  */
  urls: {
    "Chrome": {
      upgradeUrl: "http://get.webgl.org/webgl2/enable.html#chrome",
      //upgradeUrl: "http://www.google.com/support/chrome/bin/answer.py?answer=95346",
      troubleshootingUrl: "http://www.google.com/support/chrome/bin/answer.py?answer=1220892"
    },
    "Firefox": {
      upgradeUrl: "http://get.webgl.org/webgl2/enable.html#firefox",
      //upgradeUrl: "http://www.mozilla.com/en-US/firefox/new/",
      troubleshootingUrl: "https://support.mozilla.com/en-US/kb/how-do-i-upgrade-my-graphics-drivers"
    },
    "Opera": {
      platforms: {
        "Android": {
          upgradeUrl: "https://market.android.com/details?id=com.opera.browser",
          troubleshootingUrl: "http://www.opera.com/support/"
        }
      },
      upgradeUrl: "http://www.opera.com/",
      troubleshootingUrl: "http://www.opera.com/support/"
    },
    "Android": {
      upgradeUrl: null,
      troubleshootingUrl: null
    },
    "Safari": {
      platforms: {
        "iPhone/iPod": {
          upgradeUrl: "http://www.apple.com/ios/",
          troubleshootingUrl: "http://www.apple.com/support/iphone/"
        },
        "iPad": {
          upgradeUrl: "http://www.apple.com/ios/",
          troubleshootingUrl: "http://www.apple.com/support/ipad/"
        },
        "Mac": {
          upgradeUrl: "http://www.webkit.org/",
          troubleshootingUrl: "https://support.apple.com/kb/PH21426"
        }
      },
      upgradeUrl: "http://www.webkit.org/",
      troubleshootingUrl: "https://support.apple.com/kb/PH21426"
    },
    "Explorer": {
      upgradeUrl: "http://www.microsoft.com/ie",
      troubleshootingUrl: "http://msdn.microsoft.com/en-us/library/ie/bg182648(v=vs.85).aspx"
    },
    "Edge": {
      upgradeUrl: "http://www.microsoft.com/en-us/windows/windows-10-upgrade",
      troubleshootingUrl: "http://msdn.microsoft.com/en-us/library/ie/bg182648(v=vs.85).aspx"
    },
    "unknown": {
      upgradeUrl: null,
      troubleshootingUrl: null
    }
  }
};




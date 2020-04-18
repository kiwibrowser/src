# Opening links in Chrome for iOS

The easiest way to have your iOS app open links in Chrome is to use the
[OpenInChromeController](https://github.com/GoogleChrome/OpenInChrome) class.
This API is described here along with the URI schemes it supports.

## Using OpenInChromeController to open links

The **OpenInChromeController** class provides methods that
encapsulate the URI schemes and the scheme replacement process also described
in this document. Use this class to check if Chrome is installed or to specify
the URL to open.

### Methods

* `isChromeInstalled`: returns YES if Chrome is installed
* `openInChrome`: opens a given URL in Chrome

For example, use the OpenInChromeController class as follows:

```
if ([openInController_ isChromeInstalled]) {
    [openInController_ openInChrome:urlToOpen];
}
```

## Downloading the class file

The OpenInChromeController class file is available
[here](https://github.com/GoogleChrome/OpenInChrome). Copy it into
your Xcode installation.

The rest of this document describes the underpinnings of this API.

## URI schemes

Chrome for iOS handles the following URI Schemes:

* `googlechrome` for http
* `googlechromes` for https

To check if Chrome is installed, an app can simply check if either of these URI schemes is available:

```
[[UIApplication sharedApplication] canOpenURL:
    [NSURL URLWithString:@"googlechrome://"]];
```

This step is useful in case an app would like to change the UI depending
on if Chrome is installed or not. For instance the app could add an
option to open URLs in Chrome in a share menu or action sheet.

To actually open a URL in Chrome, the URI scheme provided in the URL
must be changed from `http` or `https` to the Google Chrome equivalent of
`googlechrome` or `googlechromes` respectively.
The following sample code opens a URL in Chrome:

```
NSURL *inputURL = <the URL to open>;
NSString *scheme = inputURL.scheme;

// Replace the URL Scheme with the Chrome equivalent.
NSString *chromeScheme = nil;
if ([scheme isEqualToString:@"http"]) {
  chromeScheme = @"googlechrome";
} else if ([scheme isEqualToString:@"https"]) {
  chromeScheme = @"googlechromes";
}

// Proceed only if a valid Google Chrome URI Scheme is available.
if (chromeScheme) {
  NSString *absoluteString = [inputURL absoluteString];
  NSRange rangeForScheme = [absoluteString rangeOfString:@":"];
  NSString *urlNoScheme =
      [absoluteString substringFromIndex:rangeForScheme.location];
  NSString *chromeURLString =
      [chromeScheme stringByAppendingString:urlNoScheme];
  NSURL *chromeURL = [NSURL URLWithString:chromeURLString];

  // Open the URL with Chrome.
  [[UIApplication sharedApplication] openURL:chromeURL];
}
```

If Chrome is installed, the above code converts the URI scheme found in
the URL to the Google Chrome equivalent. When Google Chrome opens, the
URL passed as a parameter will be opened in a new tab.

If Chrome is not installed the user can be prompted to download it from the App Store.
If the user agrees, the app can open the App Store download page using the following:

```
[[UIApplication sharedApplication] openURL:[NSURL URLWithString:
    @"itms-apps://itunes.apple.com/us/app/chrome/id535886823"]];
```

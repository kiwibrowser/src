# Using Custom Tabs

## Summary

This presents a few example applications using Custom Tabs, and a possible usage
of the APIs, both intent-based and the the background service. It covers UI
customization, setting up callbacks, pre-warming, pre-fetching, and lifecycle
management. Here we assume that Chrome's implementation of Custom Tabs is used.
Note that this feature is in no way specific to Chrome, but slight differences
may exist with other implementations.

### Overview

In particular, this covers:

* UI customization:
  * Toolbar color
  * Action button
  * Custom menu items
  * Custom in/out animations
* Navigation awareness: the browser delivers callbacks to the application for
  navigations in the Custom Tab.
* Performance optimizations:
  * Pre-warming of the Browser in the background, without stealing resources
    from the application
  * Providing a likely URL in advance to the browser, which may perform
    speculative work, speeding up page load time.

These features are enabled through two mechanisms:

* Adding extras to the `ACTION_VIEW` intent sent to the browser.
* Connecting to a bound service in the target browser.

### Code Organization

To get full benefits of the Custom Tabs APIs, it is recommended to use the
[Android Support Library](https://developer.android.com/tools/support-library/index.html).

The code in this repository is organised in four parts:

* `demos/`: This module contains sample implementations for Chrome Custom Tabs using the Android
  Support Library. Feel free to re-use the classes withing this module.
* `shared/`: Shared code between the `Application` and `demos` modules. Feel free to
  re-use the classes within this directory, which are only provided as a convenience.
  In particular,`CustomTabsHelper` can be re-used. This code is not required to use Custom Tabs.
* `customtabs/`: Code within this directory is in the package
  `android.support.customtabs`. This contains code analog to the Android Support library, but with
   the latest version of Chrome Custom Tabs, enabling features that may still not be available on
   the Android Support Library. API is subject to changes and this code should only be used if you
   want to test the latest features. It is recommended to copy the code as-is to your project and
   remove the Android Support Library for Chrome Custom Tabs from the `build.gradle` file.
* `Application/`: Example application code, in the package
  `org.chromium.customtabsclient`. This code uses the latest version of the Chrome Custom Tabs,
   contained in the module `customtabs`.

## UI Customization

UI customization is done through the methods exposed by
`CustomTabsIntent.Builder`.

**Example:**
```java
CustomTabsIntent.Builder builder = new CustomTabsIntent.Builder();
builder.setSession(session);
builder.setToolbarColor(Color.BLUE);
// Application exit animation, Chrome enter animation.
builder.setStartAnimations(this, R.anim.slide_in_right, R.anim.slide_out_left);
// vice versa
builder.setExitAnimations(this, R.anim.slide_in_left, R.anim.slide_out_right);

CustomTabsIntent customTabsIntent = builder.build();
customTabsIntent.launchUrl(this, packageName, url);
```

In this example, no UI customization is done, aside from the animations and the
toolbar color. The general usage is:

1. Create an instance of `CustomTabsIntent.Builder`
2. Build the UI using the methods of `CustomTabsIntent.Builder`
3. Call `CustomTabsIntent.Builder.build()`
4. Call `CustomTabsIntent.launchUrl()`

The communication between the custom tab activity and the application is done
via pending intents. For each interaction leading back to the application (menu
items and action button), a
[`PendingIntent`](http://developer.android.com/reference/android/app/PendingIntent.html)
must be provided, and will be delivered upon activation of the corresponding UI
element.

## Navigation

The hosting application can elect to get notifications about navigations in a
Custom Tab. This is done using a callback extending
`android.support.customtabs.CustomTabsCallback`, that is:

```java
void onNavigationEvent(int navigationEvent, Bundle extras);
```

This callback is set when a `CustomTabsSession` object is created, through
`CustomTabsSession.newSession()`. It thus has to be set:

* After binding to the background service
* Before launching a URL in a custom tab

The two events are analogous to `WebViewClient.onPageStarted()` and
`WebViewClient.onPageFinished()`, respectively (see
[WebViewClient](http://developer.android.com/reference/android/webkit/WebViewClient.html)).

## Optimization

**WARNING:** The browser treats the calls described in this section only as
  advice. Actual behavior may depend on connectivity, available memory and other
  resources.

The application can communicate its intention to the browser, that is:
* Warming up the browser
* Indicating a likely navigation to a given URL

In both cases, communication with the browser is done through a bound background
service. This binding is done by
`CustomTabClient.bindCustomTabsService()`. After the service is connected, the
client has access to a `CustomTabsClient` object, valid until the service gets
disconnected. This client can be used in these two cases:

* **Warmup**: Warms up the browser to make navigation faster. This is expected
  to create some CPU and IO activity, and to have a duration comparable to a
  normal Chrome startup. Once started, Chrome will not use additional
  resources. This is triggered by `CustomTabsClient.warmup()`.
* **Hint about a likely future navigation:** Indicates that a given URL may be
  loaded in the future. Chrome may perform speculative work to speed up page
  load time. The application must call `CustomTabsClient.warmup()` first. This
  is triggered by `CustomTabsSession.mayLaunchUrl()`.

**Example:**
```java
// Binds to the service.
CustomTabsClient.bindCustomTabsService(context, packageName, new CustomTabsServiceConnection() {
    @Override
    public void onCustomTabsServiceConnected(ComponentName name, CustomTabsClient client) {
        // mClient is now valid.
        mClient = client;
    }

    @Override
    public void onServiceDisconnected(ComponentName name) {
        // mClient is no longer valid. This also invalidates sessions.
        mClient = null;
    }
});

// With a valid mClient.
mClient.warmup(0);

// With a valid mClient.
CustomTabsSession session = mClient.newSession(new CustomTabsCallback());
session.mayLaunchUrl(Uri.parse("https://www.google.com"), null, null);

// Shows the Custom Tab
builder.build().launchUrl(context, packageName, Uri.parse("https://www.google.com"));
```

**Tips**

* If possible, issue the `warmup` call in advance to reduce waiting when the
  custom tab activity is started.
* If possible, advise Chrome about the likely target URL in advance, as the
  loading optimization can take time (requiring network traffic, for instance).

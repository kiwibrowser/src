# Kiosk Mode

If you have a real world kiosk application that you want to run on Google
Chrome, then below are the steps to take to simulate kiosk mode.

## Steps to Simulate Kiosk Mode

### Step 1

Compile the following Java code:

```java
import java.awt.*;
import java.applet.*;
import java.security.*;
import java.awt.event.*;

public class FullScreen extends Applet
{
   public void fullScreen()
   {
      AccessController.doPrivileged
      (
         new PrivilegedAction()
         {
            public Object run()
            {
               try
               {
                  Robot robot = new Robot();
                  robot.keyPress(KeyEvent.VK_F11);
               }
               catch (AWTException e)
               {
                  e.printStackTrace();
               }
               return null;
            }
         }
      );
   }
}
```

### Step 2

Include it in an applet on your kiosk application's home page:

```html
<applet name="appletFullScreen"
        code="FullScreen.class"
        width="1"
        height="1"></applet>
```

### Step 3

Add the following to the kiosk computer's java.policy file:

```
grant codeBase "http://yourservername/*"
{
   permission java.security.AllPermission;
};
```

### Step 4

Include the following JavaScript and assign the `doLoad` function to the
`onload` event:

```javascript
var _appletFullScreen;

function doLoad()
{
   _appletFullScreen = document.applets[0];
   doFullScreen();
}

function doFullScreen()
{
   if (_appletFullScreen && _appletFullScreen.fullScreen)
   {
      // Add an if statement to check whether document.body.clientHeight is not
      // indicative of full screen mode
      _appletFullScreen.fullScreen();
   }
}
```

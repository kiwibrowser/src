# Description
This script is to run any version of Khronos WebGL conformance test on various OSes (like Android, ChromeOS, Linux, MacOS and Windows) with various browsers (like Chrome, Edge, FireFox, Safari, etc.). Results will be
compared with expectations and a final report will be generated.  
WebDriver backs the main logic of test automation in this script.

# Supported Configurations
Target OS means the OS you run test on, while host OS is the place you run this script. They are same most of time, while Android is the only known exception now.
<table>
  <tr align=center>
    <td><strong>Target OS</td>
    <td><strong>Host OS</td>
    <td><strong>Browser</td>
  </tr>
  <tr align=left>
    <td>Android</td>
    <td>Linux</td>
    <td>Chrome: Stable, Beta, Dev, Canary, Public<br>FireFox: Stable, Beta, Aurora,  Nightly</td>
  </tr>
  <tr align=left>
    <td>Android</td>
    <td>MacOS</td>
    <td>Chrome: Stable, Beta, Dev, Canary, Public<br>FireFox: Stable, Beta, Aurora,  Nightly</td>
  </tr>
  <tr align=left>
    <td>Android</td>
    <td>Windows</td>
    <td>Chrome: Stable[1], Beta, Dev, Canary, Public<br>FireFox: Stable, Beta, Aurora,  Nightly</td>
  </tr>
  <tr align=left>
    <td colspan=2 align=center>ChromeOS</td>
    <td>Chrome[1]</td>
  </tr>
  <tr align=left>
    <td colspan=2 align=center>Linux</td>
    <td>Chrome: Stable[1], Beta, Dev<br>FireFox: Stable, Beta, Dev, Nightly</td>
  </tr>  
  <tr align=left>
    <td colspan=2 align=center>MacOS</td>
    <td>Chrome: Stable, Beta, Dev, Canary[1]<br>FireFox: Stable, Beta, Dev, Nightly<br>Safari</td>
  </tr>  
  <tr align=left>
    <td colspan=2 align=center>Windows</td>
    <td>Chrome: Stable[1], Beta, Dev, Canary<br>FireFox: Stable[1], Beta, Dev, Nightly[1]<br>IE<br>Edge[1]</td>
  </tr>
</table>

[1] means the configuration has been tested.

# Setup
## Android, Linux, MacOS and Windows
* Install Python<br>
Both Python 2 and 3 are supported, and you may download it from https://www.python.org/downloads/.  
* Install Python Selenium package<br>
pip install selenium  
Note on Windows, pip resides in &lt;python_dir>/Scripts.
* Download Android platform tools (Android only)<br>
You may download tools as below according to your system, and please be sure to put them in your PATH environment.<br>
https://dl.google.com/android/repository/platform-tools-latest-darwin.zip<br>
https://dl.google.com/android/repository/platform-tools-latest-linux.zip<br>
https://dl.google.com/android/repository/platform-tools-latest-windows.zip<br>
* Download this script<br>
Put it in &lt;work_dir><br>
* Download webdriver<br>
You may put them under &lt;work_dir>/webdriver/&lt;os_name>, which is the default place for them. Otherwise, you have to designate the path of webdriver with option --webdriver-path.  
&lt;os_name>: android, linux, mac or win.  
Webdriver download links:  
Chrome (chromedriver(.exe)): https://sites.google.com/a/chromium.org/chromedriver  
Edge (MicrosoftWebDriver.exe): https://developer.microsoft.com/en-us/microsoft-edge/tools/webdriver  
FireFox (geckodriver(.exe)): https://github.com/mozilla/geckodriver/releases  
Safari (safaridriver): already included as /usr/bin/safaridriver
* Execute script<br>
python conformance.py  
Type --help for more information
* Check report<br>
After test, you may find report in &lt;work_dir>/result/&lt;timestamp>.html  
&lt;timestamp>: The datetime you run the test in format %Y%m%d%H%M%S, e.g., 20170403235901

## ChromeOS
First, a test image is required as the script relies on telemetry. Then you just need to copy the script to your ChromeOS and execute it as others, including Python, webdriver binary, etc., just work out of the box.


# Supported Features
* Multiple Android devices<br>
You may connect multiple Android devices with your host machine, and use --android-device-id to designate the exact device you will test on.
* Multiple GPUs<br>
Multiple GPUs can be installed on same device. Typically, you may have one discrete GPU and one integrated GPU in this scenario. The choice among them can be quite flexible. For example, on MacOS, you may run one application with discrete GPU, while running another application with integrated GPU at the same time. The script will try to check some info from browser at runtime to see which GPU it uses actually.
The info of GPU in usage can be very important for the tests. For example, it's important to know how many of the expectations can be applied in current tests.
* Crash handling<br>
It's often to see some GPU driver issues crash the browser. To run the whole test suite in a batch, the capability to recover from crash is critical. However, the crash handling can be very complex, due to different browsers under very different situations.   
Currently, some simple but effective crash handling was added, which was verified to be very useful for tests at least with Chrome.
* Resume from last tests<br>
We can't always guarantee the tests to be finished smoothly, especially when many crashes are unexpected. The script will record the progress (&lt;work_dir>/log/resume) in details so that you may resume from it next time.
* Automatic retry<br>
Sometimes, a test case can be flaky under an abnormal context, and a clean retest can mute this false alarm. A simple retry mechanism is brought for this sake.
* Expectation as the baseline<br>
Sophiscated expectations regarding to OS, GPU and browser can be set so that you can always have a clear idea on improvements and regressions.
* Test with only a subset of all cases<br>
You may designate a folder or a specific case for testing using option --suite.
* Extra browser options<br>
You may pass extra browser options to test script. An intuitive usage of this is to live behind the proxy.
* Top time consuming cases<br>
Top time consuming cases will also be listed in final report, which can help to find some performance issue.
* OpenGL ES<br>
Sometimes, you want to test against OpenGL ES instead of OpenGL on Linux, and option --gles is your friend here.  
* Self-build Mesa driver<br>
On Linux, Mesa driver can be used on the fly, which means you may run the system with system graphics stack, while running browser solely with your self-build Mesa driver. Option --mesa-dir can be used for this sake.

# TODO Features
* Python 3 support<br>
* More support of host_os, target_os and browser combinations, especially for Safari
* The design of expectations
* Get more GPU, OS, browser info
* log_path of geckodriver
* Run with multiple frames (?frame=x in url)<br>
This might not be an important feature.

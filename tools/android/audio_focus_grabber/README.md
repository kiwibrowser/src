## AudioFocusGrabber: a Tool for Testing Audio Focus Handling in Apps

A simple app used to test audio focus handling in apps, especially
MediaSession in Chrome. You can perform audio gain/abandon actions, in
order to simulate a short ping from an SMS or email, or permanent
audio focus gain from other media player apps.

### Setup

#### 1: Build and install the AudioFocusGrabber app

	ninja -C out/Debug audio_focus_grabber_apk
	adb install -r out/Debug/apks/AudioFocusGrabber.apk

#### 2: Simulate audio focus actions

You can simulate audio focus actions using the UI, the notification
bar or through `adb` shell. There are three kinds of audio focus
actions, corresponding to:

* `AudioManager.AUDIOFOCUS_GAIN`
* `AudioManager.AUDIOFOCUS_TRANSIENT`
* `AudioManager.AUDIOFOCUS_TRANSIENT_MAY_DUCK`

##### 2.1: Controlling from the UI

From the UI, there are three buttons for the three actions. Just click
it, and AudioFocusGrabber will perform the audio focus action, and play a ping
sound.

However in this way, the app must be in background.

##### 2.2: Controlling from the notification

You can also start a notification from the UI, and then you can make
controls from the notification.

In this way, the app must be in background or losed window focus.

##### 2.3 Controlling from the `adb` shell

From the `adb` shell, which you can do it even if the AudioFocusGrabber is not
in foreground. You may use the following three commands:

	adb shell am startservice -a AUDIO_FOCUS_GRABBER_GAIN
	adb shell am startservice -a AUDIO_FOCUS_GRABBER_TRANSIENT_PAUSE
	adb shell am startservice -a AUDIO_FOCUS_GRABBER_TRANSIENT_DUCK

In this way, the app may be in the foreground, in the background or
losed window focus.

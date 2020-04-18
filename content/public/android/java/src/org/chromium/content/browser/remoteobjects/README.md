# Remote Objects

This is an implementation of the Blink mojo interfaces which allow objects
hosted out of process to be exposed to script. It is intended to ultimately
migrate the Gin/Java bridge used to implement
[`addJavascriptInterface`](https://developer.android.com/reference/android/webkit/WebView.html#addJavascriptInterface%28java.lang.Object,%20java.lang.String%29)
in Android WebView.

See also the [design doc](https://docs.google.com/document/d/1T8Zj_gZK7jHsy80Etk-Rw4hXMIW4QeaTtXjy5ZKP3X0/edit).

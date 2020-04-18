// Creates a frame and invokes f when the frame is ready.
window.withFrame = function (opt_url, f) {
    var url = typeof opt_url == 'string' ? opt_url : 'about:blank';
    f = f || opt_url;

    var frame = document.createElement('iframe');
    frame.onload = function () {
        f(frame);
    };
    frame.src = url;
    document.body.appendChild(frame);
};

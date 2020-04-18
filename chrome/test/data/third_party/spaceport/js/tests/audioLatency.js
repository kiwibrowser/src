define([ 'util/ensureCallback', 'util/cacheBust', 'util/timeout' ], function (ensureCallback, cacheBust, timeout) {
    var MAX_LATENCY = 20;

    function getPlayLatency(audio, callback) {
        callback = ensureCallback(callback);

        var startTime;

        function update() {
            var endTime = Date.now();

            audio.removeEventListener('play', update, false);
            audio.removeEventListener('timeupdate', update, false);
            audio.pause();

            callback(null, endTime - startTime);
        }

        audio.addEventListener('play', update, false);
        audio.addEventListener('timeupdate', update, false);

        startTime = Date.now();
        audio.play();
    }

    function audioLatency(callback) {
        callback = ensureCallback(callback);

        if (!window.Audio) {
            callback(new Error('Audio not supported'));
            return;
        }

        var audio = new window.Audio();

        function onCanPlayThrough() {
            audio.removeEventListener('canplaythrough', onCanPlayThrough, false);

            // Run the test twice: once for "cold" time, once for "warm" time.
            getPlayLatency(audio, function (err, coldTime) {
                if (err) {
                    callback(err);
                    return;
                }

                getPlayLatency(audio, function (err, warmTime) {
                    if (err) {
                        callback(err);
                        return;
                    }

                    callback(null, {
                        pass: coldTime <= MAX_LATENCY && warmTime <= MAX_LATENCY,
                        coldLatency: coldTime,
                        warmLatency: warmTime
                    });
                });
            });
        }

        function onError() {
            callback(new Error('Failed to load audio'));
        }

        var source = document.createElement('source');
        source.src = cacheBust.url('assets/silence.wav');
        source.addEventListener('error', onError, false);

        audio.addEventListener('canplaythrough', onCanPlayThrough, false);
        audio.addEventListener('error', onError, false);
        audio.appendChild(source);
        audio.play();

        // Work around Webkit bug (present in Chrome <= 15, Safari <= 5, at
        // time of writing) where the browser will decide it doesn't /need/
        // to download all these pesky audio files.
        window['audio__' + Math.random()] = audio;
    }

    return function (callback) {
        callback = ensureCallback(callback);

        timeout(5000, audioLatency, callback);
    };
});

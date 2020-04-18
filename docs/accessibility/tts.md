# Text to Speech in Chrome and Chrome OS

Chrome and Chrome OS allow developers to produce synthesized speech. This
document is an overview of the relevant code and code structure around
synthesized speech.

## Code structure

A brief outline of the flow from speech request to the resulting speech on any
platform.

### Input

- chrome.tts extension API

    - The [chrome.tts extension API](https://developer.chrome.com/apps/tts)
    allows extensions to request speech across Windows, Mac or Chrome OS, using
    native speech synthesis.

    - Input to the extension is first processed in the
    [TtsExtensionApi](https://cs.chromium.org/chromium/src/chrome/browser/speech/extension_api/tts_extension_api.h).

    - The extension is passed an [Options object](https://developer.chrome.com/apps/tts#method-speak)
    in chrome.tts.speak, which is translated into a
    [tts_controller Utterance](https://cs.chromium.org/chromium/src/chrome/browser/speech/tts_controller.h?dr=CSs&l=130).

- Web Speech API

    - Chrome implements
    [Window.SpeechSynthesis](https://developer.mozilla.org/en-US/docs/Web/API/Window/speechSynthesis)
    from the [Web Speech API](https://developer.mozilla.org/en-US/docs/Web/API/Web_Speech_API).
    This allows web apps to do text-to-speech via the device's speech
    synthesizer.

    - A [WebSpeechSynthesisUtterance](https://cs.chromium.org/chromium/src/third_party/blink/public/platform/web_speech_synthesis_utterance.h)
    is created by window.SpeechSynthesis

### Processing

- The [TtsController](https://cs.chromium.org/chromium/src/chrome/browser/speech/tts_controller.h) processes utterances and sends them to the correct output engine

### Output

- May differ by system, including Mac, Wind, Android, Arc++, and Chrome OS

- In Chrome OS:

    - [TtsEngineExtensionAPI](https://cs.chromium.org/chromium/src/chrome/browser/speech/extension_api/tts_engine_extension_api.h)
    forwards speech events to PATTS, or the network speech engine, or,
    coming soon, third-party speech engines.

    - [PATTS](patts.md) is the built-in Chrome OS text-to-speech engine.

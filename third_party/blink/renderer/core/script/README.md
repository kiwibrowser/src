# Scripting

This directory contains classes which manage the `<script>` elements and
loading and execution of scripts in Blink.

The scripts are loaded by `core/loader` and executed by V8 via `bindings/core/v8`.
The interaction between `<script>` elements and these components is controlled by `core/script`.

This directory implements the following scripting concepts in the HTML spec:

*   [`<script>` elements](https://html.spec.whatwg.org/multipage/scripting.html#the-script-element)
*   Interactions between scripts and HTML/XML documents/parsers
*   A part of [Scripting](https://html.spec.whatwg.org/multipage/webappapis.html#scripting)
    * [scripts](https://html.spec.whatwg.org/multipage/webappapis.html#definitions-2)
        * [classic scripts](https://html.spec.whatwg.org/multipage/webappapis.html#classic-script)
        * [module scripts](https://html.spec.whatwg.org/multipage/webappapis.html#module-script)
    * [Fetching](https://html.spec.whatwg.org/multipage/webappapis.html#fetching-scripts) scripts
    * [creating](https://html.spec.whatwg.org/multipage/webappapis.html#creating-scripts) scripts
    * [calling](https://html.spec.whatwg.org/multipage/webappapis.html#calling-scripts) scripts
    * [Integration with the JavaScript module system](https://html.spec.whatwg.org/multipage/webappapis.html#integration-with-the-javascript-module-system)

## See Also

[Slides](https://docs.google.com/presentation/d/1H-1U9LmCghOmviw0nYE_SP_r49-bU42SkViBn539-vg/edit?usp=sharing)

if (document.currentScript.getAttribute("nonce") == null)
    throw "Should be included by nonce-annotated element!";
document.externalScriptWithNonceHasRun = true;
eval("document.evalFromExternalWithNonceHasRun = true;");

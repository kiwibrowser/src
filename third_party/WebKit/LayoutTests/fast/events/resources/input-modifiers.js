function forEachModifier(callback) {
    var allKeyModifiers = ["Control", "Shift", "Meta", "Alt", "AltGraph", "Fn", "CapsLock", "ScrollLock", "NumLock", "Symbol"];
    var allEventSenderNames = ["ctrlKey", "shiftKey", "metaKey", "altKey", "altGraphKey", "fnKey", "capsLockOn", "scrollLockOn", "numLockOn", "symbolKey"];
    ["ctrlKey", "shiftKey", "metaKey", "altKey", "modifierAltGraph", "modifierFn","modifierCapsLock", "modifierScrollLock", "modifierNumLock", "modifierSymbol"].forEach(function (attr, index) {
        callback(attr, allKeyModifiers[index], allEventSenderNames[index])
    });
}

function forEachLegacyModifier(callback) {
    var allKeyModifiers = ["Control", "Shift", "Meta", "Alt"];
    ["ctrlKey", "shiftKey", "metaKey", "altKey"].forEach(function (attr, index) {
        callback(attr, allKeyModifiers[index])
    });
}

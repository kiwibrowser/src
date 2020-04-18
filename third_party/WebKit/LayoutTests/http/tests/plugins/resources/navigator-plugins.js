function pluginDataAsString() {
    var pluginNames = Array.prototype.map.call(navigator.plugins, function (plugin) { return plugin.name; });
    var mimeTypes = Array.prototype.map.call(navigator.mimeTypes, function (mimeType) { return mimeType.type; });
    return JSON.stringify({"pluginNames": pluginNames, "mimeTypes": mimeTypes});
}

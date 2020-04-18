var imageFailed = function(){
    var err = document.createElement("div");
    err.innerHTML = "ERROR: failed to load image";
    document.body.appendChild(err);
}

document.addEventListener("DOMContentLoaded", function() {
    var images = document.querySelectorAll("img");
    var imagesLen = images.length;
    for (var i = 0; i < imagesLen; ++i)
        images[i].onerror = imageFailed;
});

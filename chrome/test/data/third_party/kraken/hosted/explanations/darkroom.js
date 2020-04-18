/* -*- Mode: js2; js2-basic-offset: 2; indent-tabs-mode: nil; tab-width: 40; -*- */

// The if (0) block of function definitions here tries to use
// faster math primitives, based on being able to reinterpret
// floats as ints and vice versa.  We do that using the
// WebGL arrays.

if (0) {

var gConversionBuffer = new ArrayBuffer(4);
var gFloatConversion = new WebGLFloatArray(gConversionBuffer);
var gIntConversion = new WebGLIntArray(gConversionBuffer);

function AsFloat(i) {
  gIntConversion[0] = i;
  return gFloatConversion[0];
}

function AsInt(f) {
  gFloatConversion[0] = f;
  return gIntConversion[0];
}

// magic constants used for various floating point manipulations
var kMagicFloatToInt = (1 << 23);
var kOneAsInt = 0x3F800000;
var kScaleUp = AsFloat(0x00800000);
var kScaleDown = 1.0 / kScaleUp;

function ToInt(f) {
  // force integer part into lower bits of mantissa
  var i = ReinterpretFloatAsInt(f + kMagicFloatToInt);
  // return lower bits of mantissa
  return i & 0x3FFFFF;
}

function FastLog2(x) {
  return (AsInt(x) - kOneAsInt) * kScaleDown;
}

function FastPower(x, p) {
  return AsFloat(p * AsInt(x) + (1.0 - p) * kOneAsInt);
}

var LOG2_HALF = FastLog2(0.5);

function FastBias(b, x) {
  return FastPower(x, FastLog2(b) / LOG2_HALF);
}

} else {

function FastLog2(x) {
  return Math.log(x) / Math.LN2;
}

var LOG2_HALF = FastLog2(0.5);

function FastBias(b, x) {
  return Math.pow(x, FastLog2(b) / LOG2_HALF);
}

}

function FastGain(g, x) {
  return (x < 0.5) ?
    FastBias(1.0 - g, 2.0 * x) * 0.5 :
    1.0 - FastBias(1.0 - g, 2.0 - 2.0 * x) * 0.5;
}

function Clamp(x) {
  return (x < 0.0) ? 0.0 : ((x > 1.0) ? 1.0 : x);
}

function ProcessImageData(imageData, params) {
  var saturation = params.saturation;
  var contrast = params.contrast;
  var brightness = params.brightness;
  var blackPoint = params.blackPoint;
  var fill = params.fill;
  var temperature = params.temperature;
  var shadowsHue = params.shadowsHue;
  var shadowsSaturation = params.shadowsSaturation;
  var highlightsHue = params.highlightsHue;
  var highlightsSaturation = params.highlightsSaturation;
  var splitPoint = params.splitPoint;

  var brightness_a, brightness_b;
  var oo255 = 1.0 / 255.0;

  // do some adjustments
  fill *= 0.2;
  brightness = (brightness - 1.0) * 0.75 + 1.0;
  if (brightness < 1.0) {
    brightness_a = brightness;
    brightness_b = 0.0;
  } else {
    brightness_b = brightness - 1.0;
    brightness_a = 1.0 - brightness_b;
  }
  contrast = contrast * 0.5;
  contrast = (contrast - 0.5) * 0.75 + 0.5;
  temperature = (temperature / 2000.0) * 0.1;
  if (temperature > 0.0) temperature *= 2.0;
  splitPoint = ((splitPoint + 1.0) * 0.5);

  // apply to pixels
  var sz = imageData.width * imageData.height;
  var data = imageData.data;
  for (var j = 0; j < sz; j++) {
    var r = data[j*4+0] * oo255;
    var g = data[j*4+1] * oo255;
    var b = data[j*4+2] * oo255;
    // convert RGB to YIQ
    // this is a less than ideal colorspace;
    // HSL would probably be better, but more expensive
    var y = 0.299 * r + 0.587 * g + 0.114 * b;
    var i = 0.596 * r - 0.275 * g - 0.321 * b;
    var q = 0.212 * r - 0.523 * g + 0.311 * b;
    i = i + temperature;
    q = q - temperature;
    i = i * saturation;
    q = q * saturation;
    y = (1.0 + blackPoint) * y - blackPoint;
    y = y + fill;
    y = y * brightness_a + brightness_b;
    y = FastGain(contrast, Clamp(y));

    if (y < splitPoint) {
      q = q + (shadowsHue * shadowsSaturation) * (splitPoint - y);
    } else {
      i = i + (highlightsHue * highlightsSaturation) * (y - splitPoint);
    }

    // convert back to RGB for display
    r = y + 0.956 * i + 0.621 * q;
    g = y - 0.272 * i - 0.647 * q;
    b = y - 1.105 * i + 1.702 * q;

    // clamping is "free" as part of the ImageData object
    data[j*4+0] = r * 255.0;
    data[j*4+1] = g * 255.0;
    data[j*4+2] = b * 255.0;
  }
}

//
// UI code
//

var gFullCanvas = null;
var gFullContext = null;
var gFullImage = null;
var gDisplayCanvas = null;
var gDisplayContext = null;
var gZoomPoint = null;
var gDisplaySize = null;
var gZoomSize = [600, 600];
var gMouseStart = null;
var gMouseOrig = [0, 0];
var gDirty = true;

// If true, apply image correction to the original
// source image before scaling down; if false,
// scale down first.
var gCorrectBefore = false;

var gParams = null;
var gIgnoreChanges = true;

function OnSliderChanged() {
  if (gIgnoreChanges)
    return;

  gDirty = true;

  gParams = {};

  // The values will come in as 0.0 .. 1.0; some params want
  // a different range.
  var ranges = {
    "saturation": [0, 2],
    "contrast": [0, 2],
    "brightness": [0, 2],
    "temperature": [-2000, 2000],
    "splitPoint": [-1, 1]
  };

  $(".slider").each(function(index, e) {
                      var val = Math.floor($(e).slider("value")) / 1000.0;
                      var id = e.getAttribute("id");
                      if (id in ranges)
                        val = val * (ranges[id][1] - ranges[id][0]) + ranges[id][0];
                      gParams[id] = val;
                    });

  Redisplay();
}

function ClampZoomPointToTranslation() {
  var tx = gZoomPoint[0] - gZoomSize[0]/2;
  var ty = gZoomPoint[1] - gZoomSize[1]/2;
  tx = Math.max(0, tx);
  ty = Math.max(0, ty);

  if (tx + gZoomSize[0] > gFullImage.width)
    tx = gFullImage.width - gZoomSize[0];
  if (ty + gZoomSize[1] > gFullImage.height)
    ty = gFullImage.height - gZoomSize[1];
  return [tx, ty];
}

function Redisplay() {
  if (!gParams)
    return;

  var angle =
    (gParams.angle*2.0 - 1.0) * 90.0 +
    (gParams.fineangle*2.0 - 1.0) * 2.0;

  angle = Math.max(-90, Math.min(90, angle));
  angle = (angle * Math.PI) / 180.0;

  var processTime;
  var processWidth, processHeight;

  var t0 = (new Date()).getTime();

  // Render the image with rotation; we only need to render
  // if we're either correcting just the portion that's visible,
  // or if we're correcting the full thing and the sliders have been
  // changed.  Otherwise, what's in the full canvas is already corrected
  // and correct.
  if ((gCorrectBefore && gDirty) ||
      !gCorrectBefore)
  {
    gFullContext.save();
    gFullContext.translate(Math.floor(gFullImage.width / 2), Math.floor(gFullImage.height / 2));
    gFullContext.rotate(angle);
    gFullContext.globalCompositeOperation = "copy";
    gFullContext.drawImage(gFullImage,
                           -Math.floor(gFullImage.width / 2),
                           -Math.floor(gFullImage.height / 2));
    gFullContext.restore();
  }

  function FullToDisplay() {
    gDisplayContext.save();
    if (gZoomPoint) {
      var pt = ClampZoomPointToTranslation();

      gDisplayContext.translate(-pt[0], -pt[1]);
    } else {
      gDisplayContext.translate(0, 0);
      var ratio = gDisplaySize[0] / gFullCanvas.width;
      gDisplayContext.scale(ratio, ratio);
    }

    gDisplayContext.globalCompositeOperation = "copy";
    gDisplayContext.drawImage(gFullCanvas, 0, 0);
    gDisplayContext.restore();
  }

  function ProcessCanvas(cx, canvas) {
    var ts = (new Date()).getTime();

    var data = cx.getImageData(0, 0, canvas.width, canvas.height);
    ProcessImageData(data, gParams);
    cx.putImageData(data, 0, 0);

    processWidth = canvas.width;
    processHeight = canvas.height;

    processTime = (new Date()).getTime() - ts;
  }

  if (gCorrectBefore) {
    if (gDirty) {
      ProcessCanvas(gFullContext, gFullCanvas);
    } else {
      processTime = -1;
    }
    gDirty = false;
    FullToDisplay();
  } else {
    FullToDisplay();
    ProcessCanvas(gDisplayContext, gDisplayCanvas);
  }

  var t3 = (new Date()).getTime();

  if (processTime != -1) {
    $("#log")[0].innerHTML = "<p>" +
      "Size: " + processWidth + "x" + processHeight + " (" + (processWidth*processHeight) + " pixels)<br>" +
      "Process: " + processTime + "ms" + " Total: " + (t3-t0) + "ms<br>" + 
      "Throughput: " + Math.floor((processWidth*processHeight) / (processTime / 1000.0)) + " pixels per second<br>" + 
      "FPS: " + (Math.floor((1000.0 / (t3-t0)) * 100) / 100) + "<br>" +
    "</p>";
  } else {
    $("#log")[0].innerHTML = "<p>(No stats when zoomed and no processing done)</p>";
  }
}

function ZoomToPoint(x, y) {
  if (gZoomSize[0] > gFullImage.width ||
      gZoomSize[1] > gFullImage.height)
    return;

  var r = gDisplaySize[0] / gFullCanvas.width;

  gDisplayCanvas.width = gZoomSize[0];
  gDisplayCanvas.height = gZoomSize[1];
  gZoomPoint = [x/r, y/r];
  $("#canvas").removeClass("canzoomin").addClass("cangrab");
  Redisplay();  
}

function ZoomReset() {
  gDisplayCanvas.width = gDisplaySize[0];
  gDisplayCanvas.height = gDisplaySize[1];  
  gZoomPoint = null;
  $("#canvas").removeClass("canzoomout cangrab isgrabbing").addClass("canzoomin");
  Redisplay();
}

function LoadImage(url) {
  if (!gFullCanvas)
    gFullCanvas = document.createElementNS("http://www.w3.org/1999/xhtml", "canvas");
  if (!gDisplayCanvas)
    gDisplayCanvas = $("#canvas")[0];

  var img = new Image();
  img.onload = function() {
    var w = img.width;
    var h = img.height;

    gFullImage = img;

    gFullCanvas.width = w;
    gFullCanvas.height = h;
    gFullContext = gFullCanvas.getContext("2d");

    // XXX use the actual size of the visible region, so that
    // we rescale along with the window
    var dim = 600;
    if (Math.max(w,h) > dim) {
      var scale = dim / Math.max(w,h);
      w *= scale;
      h *= scale;
    }

    gDisplayCanvas.width = Math.floor(w);
    gDisplayCanvas.height = Math.floor(h);
    gDisplaySize = [ Math.floor(w), Math.floor(h) ];
    gDisplayContext = gDisplayCanvas.getContext("2d");

    $("#canvas").removeClass("canzoomin canzoomout cangrab isgrabbing");

    if (gZoomSize[0] <= gFullImage.width &&
        gZoomSize[1] <= gFullImage.height)
    {
      $("#canvas").addClass("canzoomin");
    }

    OnSliderChanged();
  };
  //img.src = "foo.jpg";
  //img.src = "Nina6.jpg";
  img.src = url ? url : "sunspots.jpg";
}

function SetupDnD() {
  $("#imagedisplay").bind({
                            dragenter: function(e) {
                              $("#imagedisplay").addClass("indrag");
                              return false;
                            },

                            dragover: function(e) {
                              return false;
                            },

                            dragleave: function(e) {
                              $("#imagedisplay").removeClass("indrag");
                              return false;
                            },

                            drop: function(e) {
                              e = e.originalEvent;
                              var dt = e.dataTransfer;
                              var files = dt.files;

                              if (files.length > 0) {
                                var file = files[0];
                                var reader = new FileReader();
                                reader.onload = function(e) { LoadImage(e.target.result); };
                                reader.readAsDataURL(file);
                              }

                              $("#imagedisplay").removeClass("indrag");
                              return false;
                            }
                          });
}

function SetupZoomClick() {
  $("#canvas").bind({
                      click: function(e) {
                        if (gZoomPoint)
                          return true;

                        var bounds = $("#canvas")[0].getBoundingClientRect();
                        var x = e.clientX - bounds.left;
                        var y = e.clientY - bounds.top;

                        ZoomToPoint(x, y);
                        return false;
                      },

                      mousedown: function(e) {
                        if (!gZoomPoint)
                          return true;

                        $("#canvas").addClass("isgrabbing");

                        gMouseOrig[0] = gZoomPoint[0];
                        gMouseOrig[1] = gZoomPoint[1];
                        gMouseStart = [ e.clientX, e.clientY ];

                        return false;
                      },

                      mouseup: function(e) {
                        if (!gZoomPoint || !gMouseStart)
                          return true;
                        $("#canvas").removeClass("isgrabbing");

                        gZoomPoint = ClampZoomPointToTranslation();

                        gZoomPoint[0] += gZoomSize[0]/2;
                        gZoomPoint[1] += gZoomSize[1]/2;

                        gMouseStart = null;
                        return false;
                      },

                      mousemove: function(e) {
                        if (!gZoomPoint || !gMouseStart)
                          return true;

                        gZoomPoint[0] = gMouseOrig[0] + (gMouseStart[0] - e.clientX);
                        gZoomPoint[1] = gMouseOrig[1] + (gMouseStart[1] - e.clientY);
                        Redisplay();

                        return false;
                      }
                    });
  
}

function CheckboxToggled(skipRedisplay) {
  gCorrectBefore = $("#correct_before")[0].checked ? true : false;

  if (!skipRedisplay)
    Redisplay();
}

function ResetSliders() {
  gIgnoreChanges = true;

  $(".slider").each(function(index, e) { $(e).slider("value", 500); });
  $("#blackPoint").slider("value", 0);
  $("#fill").slider("value", 0);
  $("#shadowsSaturation").slider("value", 0);
  $("#highlightsSaturation").slider("value", 0);

  gIgnoreChanges = false;
}

function DoReset() {
  ResetSliders();  
  ZoomReset();
  OnSliderChanged();
}

function DoRedisplay() {
  Redisplay();
}

// Speed test: run 10 processings, report in thousands-of-pixels-per-second
function Benchmark() {
  var times = [];

  var width = gFullCanvas.width;
  var height = gFullCanvas.height;

  $("#benchmark-status")[0].innerHTML = "Resetting...";

  ResetSliders();

  setTimeout(RunOneTiming, 0);

  function RunOneTiming() {

    $("#benchmark-status")[0].innerHTML = "Running... " + (times.length + 1);

    // reset to original image
    gFullContext.save();
    gFullContext.translate(Math.floor(gFullImage.width / 2), Math.floor(gFullImage.height / 2));
    gFullContext.globalCompositeOperation = "copy";
    gFullContext.drawImage(gFullImage,
                           -Math.floor(gFullImage.width / 2),
                           -Math.floor(gFullImage.height / 2));
    gFullContext.restore();

    // time the processing
    var start = (new Date()).getTime();
    var data = gFullContext.getImageData(0, 0, width, height);
    ProcessImageData(data, gParams);
    gFullContext.putImageData(data, 0, 0);
    var end = (new Date()).getTime();
    times.push(end - start);

    if (times.length < 5) {
      setTimeout(RunOneTiming, 0);
    } else {
      displayResults();
    }

  }

  function displayResults() {
    var totalTime = times.reduce(function(p, c) { return p + c; });
    var totalPixels = height * width * times.length;
    var MPixelsPerSec = totalPixels / totalTime / 1000;
    $("#benchmark-status")[0].innerHTML = "Complete: " + MPixelsPerSec.toFixed(2) + " megapixels/sec";
    $("#benchmark-ua")[0].innerHTML = navigator.userAgent;
  }
}

function SetBackground(n) {
  $("body").removeClass("blackbg whitebg graybg");

  switch (n) {
  case 0: // black
    $("body").addClass("blackbg");
    break;
  case 1: // gray
    $("body").addClass("graybg");
    break;
  case 2: // white
    $("body").addClass("whitebg");
    break;
  }
}

$(function() {
    $(".slider").slider({
                          orientation: 'horizontal',
                          range: "min",
                          max: 1000,
                          value: 500,
                          slide: OnSliderChanged,
                          change: OnSliderChanged
                        });
    ResetSliders();
    SetupDnD();
    SetupZoomClick();
    CheckboxToggled(true);
    LoadImage();
  });

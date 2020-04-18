<?php
// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This test tests that the JavaScript exposure of feature policy in iframes
// works via the following methods:
//     allowsFeature(feature)
//         -- if |feature| is allowed on the src origin of the iframe.
//     allowsFeature(feature, origin)
//         -- if |feature| is allowed on the given origin in the iframe.
//     allowedFeatures()
//         -- a list of features that are enabled on the src origin of the
//            iframe.
//     getAllowlistForFeatureForFeature(feature)
//         -- a list of explicitly named origins where the given feature is
//            enabled, or
//            ['*'] if the feature is enabled on all origins.

Header("Feature-Policy: fullscreen *; payment 'self'; midi 'none'; camera 'self' http://www.example.com https://www.example.net http://localhost:8000");
?>

<!DOCTYPE html>
<script src="../../resources/testharness.js"></script>
<script src="../../resources/testharnessreport.js"></script>
<iframe id="f1" src="../resources/dummy.html"></iframe>
<iframe id="f2" src="http://localhost:8000/../resources/dummy.html"></iframe>
<script>
var local_iframe_policy = document.getElementById("f1").policy;
var remote_iframe_policy = document.getElementById("f2").policy;
var local_src = "http://127.0.0.1:8000";
var remote_src = "http://localhost:8000";
// Tests for policy.allowsFeature().
// fullscreen should be allowed in both iframes on any origin.
test(function() {
  assert_true(local_iframe_policy.allowsFeature("fullscreen"));
  assert_true(local_iframe_policy.allowsFeature("fullscreen", local_src));
  assert_false(local_iframe_policy.allowsFeature("fullscreen", remote_src));
  assert_true(remote_iframe_policy.allowsFeature("fullscreen"));
  assert_true(remote_iframe_policy.allowsFeature("fullscreen", remote_src));
  assert_false(remote_iframe_policy.allowsFeature("fullscreen", local_src));
  assert_false(local_iframe_policy.allowsFeature("fullscreen", "http://www.example.com"));
  assert_false(remote_iframe_policy.allowsFeature("fullscreen", "http://www.example.com"));
}, 'Test policy.allowsFeature() on fullscreen');

// Camera should be allowed in both iframes on src origin but no
test(function() {
  assert_true(local_iframe_policy.allowsFeature("camera"));
  assert_true(local_iframe_policy.allowsFeature("camera", local_src));
  assert_false(local_iframe_policy.allowsFeature("camera", remote_src));
  assert_true(remote_iframe_policy.allowsFeature("camera"));
  assert_true(remote_iframe_policy.allowsFeature("camera", remote_src));
  assert_false(remote_iframe_policy.allowsFeature("camera", local_src));
  assert_false(local_iframe_policy.allowsFeature("camera", "http://www.example.com"));
  assert_false(remote_iframe_policy.allowsFeature("camera", "http://www.example.com"));
}, 'Test policy.allowsFeature() on camera');
// payment should only be allowed in the local iframe on src origin:
test(function() {
  assert_true(local_iframe_policy.allowsFeature("payment"));
  assert_true(local_iframe_policy.allowsFeature("payment", local_src));
  assert_false(local_iframe_policy.allowsFeature("payment", remote_src));
  assert_false(remote_iframe_policy.allowsFeature("payment"));
  assert_false(remote_iframe_policy.allowsFeature("payment", local_src));
  assert_false(remote_iframe_policy.allowsFeature("payment", remote_src));
}, 'Test policy.allowsFeature() on locally allowed feature payment');
// badfeature and midi should be disallowed in both iframes:
for (var feature of ["badfeature", "midi"]) {
  test(function() {
    assert_false(local_iframe_policy.allowsFeature(feature));
    assert_false(local_iframe_policy.allowsFeature(feature, local_src));
    assert_false(local_iframe_policy.allowsFeature(feature, remote_src));
    assert_false(remote_iframe_policy.allowsFeature(feature));
    assert_false(remote_iframe_policy.allowsFeature(feature, local_src));
    assert_false(remote_iframe_policy.allowsFeature(feature, remote_src));
  }, 'Test policy.allowsFeature() on disallowed feature ' + feature);
}

// Tests for policy.allowedFeatures().
var allowed_local_iframe_features = local_iframe_policy.allowedFeatures();
var allowed_remote_iframe_features = remote_iframe_policy.allowedFeatures();
for (var feature of ["fullscreen", "camera"]) {
  test(function() {
    assert_true(allowed_local_iframe_features.includes(feature));
    assert_true(allowed_remote_iframe_features.includes(feature));
  }, 'Test policy.allowedFeatures() include feature ' + feature);
}
for (var feature of ["badfeature", "midi"]) {
  test(function() {
    assert_false(allowed_local_iframe_features.includes(feature));
    assert_false(allowed_remote_iframe_features.includes(feature));
  }, 'Test policy.allowedFeatures() does not include disallowed feature ' +
    feature);
}
for (var feature of ["payment", "geolocation"]) {
test(function() {
  assert_true(allowed_local_iframe_features.includes(feature));
  assert_false(allowed_remote_iframe_features.includes(feature));
}, 'Test policy.allowedFeatures() locally include feature ' + feature +
  '  but not remotely ');
}

// Tests for policy.getAllowlistForFeature().
test(function() {
  assert_array_equals(
    local_iframe_policy.getAllowlistForFeature("fullscreen"), [local_src]);
  assert_array_equals(
    remote_iframe_policy.getAllowlistForFeature("fullscreen"), [remote_src]);
}, 'policy.getAllowlistForFeature(): fullscreen is allowed in both iframes');
test(function() {
  assert_array_equals(
    local_iframe_policy.getAllowlistForFeature("payment"), [local_src]);
  assert_array_equals(
    remote_iframe_policy.getAllowlistForFeature("payment"), []);
}, 'policy.getAllowlistForFeature(): payment is allowed only in local iframe');
test(function() {
  assert_array_equals(
    local_iframe_policy.getAllowlistForFeature("geolocation"), [local_src]);
  assert_array_equals(
    remote_iframe_policy.getAllowlistForFeature("geolocation"), []);
}, 'policy.getAllowlistForFeature(): geolocation is allowed only in local iframe');
test(function() {
  assert_array_equals(
    local_iframe_policy.getAllowlistForFeature("midi"), []);
  assert_array_equals(
    remote_iframe_policy.getAllowlistForFeature("midi"), []);
}, 'policy.getAllowlistForFeature(): midi is disallowed in both iframe');

// Dynamically update iframes policy.
document.getElementById("f1").allow = "fullscreen 'none'; payment 'src'; midi 'src'; geolocation 'none'; camera 'src' 'self' https://www.example.com https://www.example.net";
document.getElementById("f2").allow = "fullscreen 'none'; payment 'src'; midi 'src'; geolocation 'none'; camera 'src' 'self' https://www.example.com https://www.example.net";
test(function() {
  assert_array_equals(
    local_iframe_policy.getAllowlistForFeature("fullscreen"), []);
  assert_array_equals(
    document.getElementById("f1").policy.getAllowlistForFeature("fullscreen"),
    []);
  assert_array_equals(
    remote_iframe_policy.getAllowlistForFeature("fullscreen"), []);
  assert_array_equals(
    document.getElementById("f2").policy.getAllowlistForFeature("fullscreen"),
    []);
}, 'Dynamically redefine allow: fullscreen is disallowed in both iframes');

test(function() {
  assert_array_equals(
    local_iframe_policy.getAllowlistForFeature("payment"), [local_src]);
  assert_array_equals(
    document.getElementById("f1").policy.getAllowlistForFeature("payment"),
    [local_src]);
  assert_array_equals(
    remote_iframe_policy.getAllowlistForFeature("payment"), [remote_src]);
  assert_array_equals(
    document.getElementById("f2").policy.getAllowlistForFeature("payment"),
    [remote_src]);
}, 'Dynamically redefine allow: payment is allowed in both iframes');

test(function() {
  assert_array_equals(
    local_iframe_policy.getAllowlistForFeature("geolocation"), []);
  assert_array_equals(
    remote_iframe_policy.getAllowlistForFeature("geolocation"), []);
}, 'Dynamically redefine allow: geolocation is disallowed in both iframes');

test(function() {
  assert_array_equals(
    local_iframe_policy.getAllowlistForFeature("camera"), [local_src]);
  assert_array_equals(
    remote_iframe_policy.getAllowlistForFeature("camera"), [remote_src]);
}, 'Dynamically redefine allow: camera is allowed in both iframes');

test(function() {
  assert_array_equals(
    local_iframe_policy.getAllowlistForFeature("midi"), []);
  assert_array_equals(
    remote_iframe_policy.getAllowlistForFeature("midi"), []);
}, 'Dynamically redefine allow: midi is disallowed in both iframe');
</script>

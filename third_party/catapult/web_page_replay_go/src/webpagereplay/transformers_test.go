// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package webpagereplay

import (
	"bytes"
	"compress/gzip"
	"fmt"
	"io"
	"io/ioutil"
	"net/http"
	"strconv"
	"testing"
	"time"
)

func TestReplaceTimeStamp(t *testing.T) {
	time_stamp_ms := time.Date(2017, time.June, 1, 23, 0, 0, 0, time.UTC).Unix() * 1000
	replacements := map[string]string{"{{WPR_TIME_SEED_TIMESTAMP}}": strconv.FormatInt(time_stamp_ms, 10)}
	script := []byte("var time_seed = {{WPR_TIME_SEED_TIMESTAMP}};")
	transformer := NewScriptInjector(script, replacements)
	req := http.Request{}
	responseHeader := http.Header{
		"Content-Type": []string{"text/html"}}
	resp := http.Response{
		StatusCode: 200,
		Header:     responseHeader,
		Body:       ioutil.NopCloser(bytes.NewReader([]byte("<html></html>")))}
	transformer.Transform(&req, &resp)
	body, err := ioutil.ReadAll(resp.Body)
	resp.Body.Close()
	if err != nil {
		t.Fatal(err)
	}
	expectedContent := []byte(fmt.Sprintf("<html><script>var time_seed = %d;</script></html>", time_stamp_ms))
	if !bytes.Equal(expectedContent, body) {
		t.Fatal(fmt.Errorf("expected : %s \n actual: %s \n", expectedContent, body))
	}
}

// Regression test for https://github.com/catapult-project/catapult/issues/3726
func TestInjectScript(t *testing.T) {
	script := []byte("var foo = 1;")
	transformer := NewScriptInjector(script, nil)
	req := http.Request{}
	responseHeader := http.Header{
		"Content-Type": []string{"text/html"}}
	resp := http.Response{
		StatusCode: 200,
		Header:     responseHeader,
		Body:       ioutil.NopCloser(bytes.NewReader([]byte("<html><head><script>document.write('<head></head>');</script></head></html>")))}
	transformer.Transform(&req, &resp)
	body, err := ioutil.ReadAll(resp.Body)
	resp.Body.Close()
	if err != nil {
		t.Fatal(err)
	}
	expectedContent := []byte(fmt.Sprintf("<html><head><script>var foo = 1;</script><script>document.write('<head></head>');</script></head></html>"))
	if !bytes.Equal(expectedContent, body) {
		t.Fatal(fmt.Errorf("expected : %s \n actual: %s \n", expectedContent, body))
	}
}

func TestNoTagFound(t *testing.T) {
	script := []byte("var foo = 1;")
	transformer := NewScriptInjector(script, nil)
	req := http.Request{}
	responseHeader := http.Header{
		"Content-Type": []string{"text/html"}}
	resp := http.Response{
		StatusCode: 200,
		Header:     responseHeader,
		Body:       ioutil.NopCloser(bytes.NewReader([]byte("no tag random content")))}
	resp.Request = &req
	transformer.Transform(&req, &resp)
	body, err := ioutil.ReadAll(resp.Body)
	resp.Body.Close()
	if err != nil {
		t.Fatal(err)
	}
	expectedContent := []byte(fmt.Sprintf("no tag random content"))
	if !bytes.Equal(expectedContent, body) {
		t.Fatal(fmt.Errorf("expected : %s \n actual: %s \n", expectedContent, body))
	}
}

func TestInjectScriptToGzipResponse(t *testing.T) {
	script := []byte("var foo = 1;")
	transformer := NewScriptInjector(script, nil)
	req := http.Request{}
	responseHeader := http.Header{
		"Content-Type":     []string{"text/html"},
		"Content-Encoding": []string{"gzip"}}
	var gzippedBody bytes.Buffer
	gz := gzip.NewWriter(&gzippedBody)
	if _, err := gz.Write([]byte("<html></html>")); err != nil {
		t.Fatal(err)
	}
	if err := gz.Close(); err != nil {
		t.Fatal(err)
	}
	resp := http.Response{
		StatusCode: 200,
		Header:     responseHeader,
		Body:       ioutil.NopCloser(bytes.NewReader(gzippedBody.Bytes()))}
	transformer.Transform(&req, &resp)
	var reader io.ReadCloser
	var err error
	if reader, err = gzip.NewReader(resp.Body); err != nil {
		t.Fatal(err)
	}
	var body []byte
	if body, err = ioutil.ReadAll(reader); err != nil {
		t.Fatal(err)
	}
	reader.Close()
	expectedContent := []byte("<html><script>var foo = 1;</script></html>")
	if !bytes.Equal(expectedContent, body) {
		t.Fatal(fmt.Errorf("expected : %s \n actual: %s \n", expectedContent, body))
	}
}

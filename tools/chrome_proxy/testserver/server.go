// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Test server to facilitate the data reduction proxy Telemetry tests.
//
// The server runs at http://chromeproxy-test.appspot.com/. Please contact
// people in OWNERS for server issues.
//
// For running an AppEngine Go server, see:
// https://developers.google.com/appengine/docs/go/gettingstarted/introduction.
//
// The goal is to keep the test logic on the client side (Telemetry)
// as much as possible. This server will only return a resource
// and/or override the response as specified by the data encoded
// in the request URL queries.
//
// For example, on receiving the query
// /default?respBody=bmV3IGJvZHk=&respHeader=eyJWaWEiOlsiVmlhMSIsIlZpYTIiXX0%3D&respStatus=204
// the server sends back a response with
//	Status code: 204
//	Additional response headers: "Via: Via1" and "Via: Via2"
//	Response body: "new body"
// where the overriding headers and body are base64 encoded in the request query.

package server

import (
	"bytes"
	"encoding/base64"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"net/http"
	"os"
	"strconv"
	"strings"
	"time"
)

func init() {
	http.HandleFunc("/requestHeader", requestHeader)
	http.HandleFunc("/resource", resource)
	http.HandleFunc("/default", defaultResponse)
	http.HandleFunc("/blackhole", blackholeProxy)
}

// requestHander returns request headers in response body as text.
func requestHeader(w http.ResponseWriter, r *http.Request) {
	r.Header.Write(w)
}

// resource returns the content of a data file specified by "r=" query as the response body.
// The response could be overridden by request queries.
// See parseOverrideQuery.
func resource(w http.ResponseWriter, r *http.Request) {
	wroteBody, err := applyOverride(w, r)
	if err != nil || wroteBody {
		return
	}
	path, ok := r.URL.Query()["r"]
	if !ok || len(path) != 1 {
		w.WriteHeader(http.StatusBadRequest)
		w.Write([]byte("no resource in query"))
		return
	}
	if _, err := writeFromFile(w, path[0]); err != nil {
		w.WriteHeader(http.StatusBadRequest)
		w.Write([]byte(fmt.Sprintf("Failed to get %s: %v", path[0], err)))
		return
	}
}

// defaultResponse returns "ok" as response body, if the body is not overridden.
// The response could be overridden by request queries.
// See parseOverrideQuery.
func defaultResponse(w http.ResponseWriter, r *http.Request) {
	wroteBody, err := applyOverride(w, r)
	if err != nil {
		return
	}
	if !wroteBody {
		w.Write([]byte("ok"))
	}
}

// blackholePoxy delays 90 seconds for proxied responses, in order to test if
// the proxy will timeout on the site. Responds immediately to any other request.
func blackholeProxy(w http.ResponseWriter, r *http.Request) {
	if strings.Contains(r.Header.Get("via"), "Chrome-Compression-Proxy") {
		// Causes timeout on proxy, will then send BLOCK_ONCE.
		// Appspot will 502 traffic at 120 seconds with no response.
		time.Sleep(90 * time.Second);
		w.Write([]byte("You are proxy"));
	} else {
		w.Write([]byte("You are direct"));
	}
}

type override struct {
	status int
	header http.Header
	body   io.Reader
}

// parseOverrideQuery parses the queries in r and returns an override.
// It supports the following queries:
//   "respStatus": an integer to override response status code;
//   "respHeader": base64 encoded JSON data to override the response headers;
//   "respBody": base64 encoded JSON data to override the response body.
func parseOverrideQuery(r *http.Request) (*override, error) {
	q := r.URL.Query()
	resp := &override{0, nil, nil}
	if v, ok := q["respStatus"]; ok && len(v) == 1 && len(v[0]) > 0 {
		status, err := strconv.ParseInt(v[0], 10, 0)
		if err != nil {
			return nil, errors.New(fmt.Sprintf("respStatus: %v", err))
		}
		resp.status = int(status)
	}
	if v, ok := q["respHeader"]; ok && len(v) == 1 && len(v[0]) > 0 {
		// Example header after base64 decoding:
		//  {"Via": ["Telemetry Test", "Test2"], "Name": ["XYZ"], "Cache-Control": ["public"]}
		headerValue, err := base64.URLEncoding.DecodeString(v[0])
		if err != nil {
			return nil, errors.New(fmt.Sprintf("Decoding respHeader: %v", err))
		}
		var header http.Header
		err = json.Unmarshal(headerValue, &header)
		if err != nil {
			return nil, errors.New(
				fmt.Sprintf("Unmarlshal (%s) error: %v", string(headerValue), err))
		}
		resp.header = header
	}
	if v, ok := q["respBody"]; ok && len(v) == 1 && len(v[0]) > 0 {
		body, err := base64.URLEncoding.DecodeString(v[0])
		if err != nil {
			return nil, errors.New(
				fmt.Sprintf("Decoding respBody error: %v", err))
		}
		resp.body = bytes.NewBuffer(body)
	}
	return resp, nil
}

// applyOverride applies the override queries in r to w and returns whether the response
// body is overridden.
func applyOverride(w http.ResponseWriter, r *http.Request) (wroteBody bool, err error) {
	resp, err := parseOverrideQuery(r)
	if err != nil {
		w.WriteHeader(http.StatusBadRequest)
		w.Write([]byte(err.Error()))
		return false, err
	}
	headers := w.Header()
	if resp.header != nil {
		for k, v := range resp.header {
			headers[k] = v
		}
	}
	if resp.status > 0 {
		w.WriteHeader(resp.status)
	}
	if resp.body != nil {
		_, err := io.Copy(w, resp.body)
		return true, err
	}
	return false, nil
}

func writeFromFile(w io.Writer, filename string) (int64, error) {
	f, err := os.Open(filename)
	if err != nil {
		return 0, err
	}
	return io.Copy(w, f)
}

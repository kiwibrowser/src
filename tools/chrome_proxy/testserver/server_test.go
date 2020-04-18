// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package server

import (
	"encoding/base64"
	"encoding/json"
	"net/http"
	"net/http/httptest"
	"net/url"
	"reflect"
	"strconv"
	"testing"
)

func composeQuery(path string, code int, headers http.Header, body []byte) (string, error) {
	u, err := url.Parse(path)
	if err != nil {
		return "", err
	}
	q := u.Query()
	if code > 0 {
		q.Set("respStatus", strconv.Itoa(code))
	}
	if headers != nil {
		h, err := json.Marshal(headers)
		if err != nil {
			return "", err
		}
		q.Set("respHeader", base64.URLEncoding.EncodeToString(h))
	}
	if len(body) > 0 {
		q.Set("respBody", base64.URLEncoding.EncodeToString(body))
	}
	u.RawQuery = q.Encode()
	return u.String(), nil
}

func TestResponseOverride(t *testing.T) {
	tests := []struct {
		name    string
		code    int
		headers http.Header
		body    []byte
	}{
		{name: "code", code: 204},
		{name: "body", body: []byte("new body")},
		{
			name: "headers",
			headers: http.Header{
				"Via":          []string{"Via1", "Via2"},
				"Content-Type": []string{"random content"},
			},
		},
		{
			name: "everything",
			code: 204,
			body: []byte("new body"),
			headers: http.Header{
				"Via":          []string{"Via1", "Via2"},
				"Content-Type": []string{"random content"},
			},
		},
	}

	for _, test := range tests {
		u, err := composeQuery("http://test.com/override", test.code, test.headers, test.body)
		if err != nil {
			t.Errorf("%s: composeQuery: %v", test.name, err)
			return
		}
		req, err := http.NewRequest("GET", u, nil)
		if err != nil {
			t.Errorf("%s: http.NewRequest: %v", test.name, err)
			return
		}
		w := httptest.NewRecorder()
		defaultResponse(w, req)
		if test.code > 0 {
			if got, want := w.Code, test.code; got != want {
				t.Errorf("%s: response code: got %d want %d", test.name, got, want)
				return
			}
		}
		if test.headers != nil {
			for k, want := range test.headers {
				got, ok := w.HeaderMap[k]
				if !ok || !reflect.DeepEqual(got, want) {
					t.Errorf("%s: header %s: code: got %v want %v", test.name, k, got, want)
					return
				}
			}
		}
		if test.body != nil {
			if got, want := string(w.Body.Bytes()), string(test.body); got != want {
				t.Errorf("%s: body: got %s want %s", test.name, got, want)
				return
			}
		}
	}
}

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package webpagereplay

import (
	"net/http"
	"net/http/httptest"
	"testing"
)

func createArchivedRequest(t *testing.T, ustr string, header http.Header) *ArchivedRequest {
	req := httptest.NewRequest("GET", ustr, nil)
	req.Header = header
	resp := http.Response{StatusCode: 200}
	archivedRequest, err := serializeRequest(req, &resp)
	if err != nil {
		t.Fatalf("failed serialize request %s: %v", ustr, err)
		return nil
	}
	return archivedRequest
}

func TestFindRequestFuzzyMatching(t *testing.T) {
	a := newArchive()
	const u = "https://example.com/a/b/c/+/query?usegapi=1&foo=bar&c=d"
	const host = "example.com"
	req := createArchivedRequest(t, u, nil)
	a.Requests[host] = make(map[string][]*ArchivedRequest)
	a.Requests[host][u] = []*ArchivedRequest{req}

	const newUrl = "https://example.com/a/b/c/+/query?usegapi=1&foo=yay&c=d&a=y"
	newReq := httptest.NewRequest("GET", newUrl, nil)

	_, foundResp, err := a.FindRequest(newReq, "https")
	if err != nil {
		t.Fatalf("failed to find %s: %v", newUrl, err)
	}
	if got, want := foundResp.StatusCode, 200; got != want {
		t.Fatalf("status codes do not match. Got: %d. Want: %d", got, want)
	}
}

// Regression test for updating bestRatio when matching query params.
func TestFindClosest(t *testing.T) {
	a := newArchive()
	const host = "example.com"
	a.Requests[host] = make(map[string][]*ArchivedRequest)
	// Store three requests. u1 and u2 match equally well. u1 is chosen because
	// u1<u2.
	const u1 = "https://example.com/index.html?a=f&c=e"
	a.Requests[host][u1] = []*ArchivedRequest{createArchivedRequest(t, u1, nil)}

	const u2 = "https://example.com/index.html?a=g&c=e"
	a.Requests[host][u2] = []*ArchivedRequest{createArchivedRequest(t, u2, nil)}

	const u3 = "https://example.com/index.html?a=b&c=d"
	a.Requests[host][u3] = []*ArchivedRequest{createArchivedRequest(t, u3, nil)}

	const newUrl = "https://example.com/index.html?c=e"
	newReq := httptest.NewRequest("GET", newUrl, nil)

	// Check that u1 is returned. FindRequest was previously non-deterministic,
	// due to random map iteration, so run the test several times.
	for i := 0; i < 10; i++ {
		foundReq, foundResp, err := a.FindRequest(newReq, "https")
		if err != nil {
			t.Fatalf("failed to find %s: %v", newUrl, err)
		}
		if got, want := foundResp.StatusCode, 200; got != want {
			t.Fatalf("status codes do not match. Got: %d. Want: %d", got, want)
		}

		query := foundReq.URL.Query()
		if query.Get("a") != "f" || query.Get("c") != "e" {
			t.Fatalf("wrong request is matched\nexpected: %s\nactual: %s", u1, foundReq.URL)
		}
	}
}

// Regression test for https://github.com/catapult-project/catapult/issues/3727
func TestMatchHeaders(t *testing.T) {
	a := newArchive()
	const u = "https://example.com/mail/"
	const host = "example.com"
	headers := http.Header{}
	headers.Set("Accept", "text/html")
	headers.Set("Accept-Language", "en-Us,en;q=0.8")
	headers.Set("Accept-Encoding", "gzip, deflate, br")
	headers.Set("Cookie", "FOO=FOO")
	{
		req := createArchivedRequest(t, u, headers)
		a.Requests[host] = make(map[string][]*ArchivedRequest)
		a.Requests[host][u] = []*ArchivedRequest{req}
	}
	{
		headers.Set("Cookie", "FOO=BAR;SSID=XXhdfdf;LOGIN=HELLO")
		req := createArchivedRequest(t, u, headers)
		a.Requests[host][u] = append(a.Requests[host][u], req)
	}

	newReq := httptest.NewRequest("GET", u, nil)
	newReq.Header = headers

	foundReq, _, err := a.FindRequest(newReq, "https")
	if err != nil {
		t.Fatalf("failed to find %s: %v", u, err)
	}
	if got, want := foundReq.Header.Get("Cookie"), headers.Get("Cookie"); got != want {
		t.Fatalf("expected %s , actual %s\n", want, got)
	}
}

// When no header matches, the archived request with the same url should still be returned.
func TestNoHeadersMatch(t *testing.T) {
	a := newArchive()
	const u = "https://example.com/mail/"
	const host = "example.com"
	headers := http.Header{}
	headers.Set("Accept-Encoding", "gzip, deflate, br")
	req := createArchivedRequest(t, u, headers)
	a.Requests[host] = make(map[string][]*ArchivedRequest)
	a.Requests[host][u] = []*ArchivedRequest{req}

	newReq := httptest.NewRequest("GET", u, nil)
	newReq.Header = headers
	newReq.Header.Set("Accept-Encoding", "gzip, deflate")

	foundReq, _, err := a.FindRequest(newReq, "https")
	if err != nil {
		t.Fatalf("failed to find %s: %v", u, err)
	}
	if got, want := foundReq.Header.Get("Accept-Encoding"), "gzip, deflate, br"; got != want {
		t.Fatalf("expected %s , actual %s\n", want, got)
	}
}

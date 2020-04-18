// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Program httparchive prints information about archives saved by record.
package main

import (
	"bufio"
	"bytes"
	"flag"
	"fmt"
	"io"
	"io/ioutil"
	"net/http"
	"os"
	"os/exec"
	"path/filepath"
	"strings"

	"github.com/catapult-project/catapult/web_page_replay_go/src/webpagereplay"
	"github.com/urfave/cli"
)

const usage = "%s [ls|cat|edit] [options] archive_file [output_file]"

type Config struct {
	method, host, fullPath string
	decodeResponseBody     bool
}

func (cfg *Config) Flags() []cli.Flag {
	return []cli.Flag{
		cli.StringFlag{
			Name:        "command",
			Value:       "",
			Usage:       "Only show URLs matching this HTTP method.",
			Destination: &cfg.method,
		},
		cli.StringFlag{
			Name:        "host",
			Value:       "",
			Usage:       "Only show URLs matching this host.",
			Destination: &cfg.host,
		},
		cli.StringFlag{
			Name:        "full_path",
			Value:       "",
			Usage:       "Only show URLs matching this full path.",
			Destination: &cfg.fullPath,
		},
		cli.BoolFlag{
			Name:        "decode_response_body",
			Usage:       "Decode/encode response body according to Content-Encoding header.",
			Destination: &cfg.decodeResponseBody,
		},
	}
}

func (cfg *Config) requestEnabled(req *http.Request) bool {
	if cfg.method != "" && strings.ToUpper(cfg.method) != req.Method {
		return false
	}
	if cfg.host != "" && cfg.host != req.Host {
		return false
	}
	if cfg.fullPath != "" && cfg.fullPath != req.URL.Path {
		return false
	}
	return true
}

func fail(msg string) {
	fmt.Fprintf(os.Stderr, "Error: %s.\n\n", msg)
	flag.Usage()
	os.Exit(1)
}

func list(cfg *Config, a *webpagereplay.Archive, printFull bool) {
	a.ForEach(func(req *http.Request, resp *http.Response) {
		if !cfg.requestEnabled(req) {
			return
		}
		if printFull {
			fmt.Fprint(os.Stdout, "----------------------------------------\n")
			req.Write(os.Stdout)
			fmt.Fprint(os.Stdout, "\n")
			err := webpagereplay.DecompressResponse(resp)
			if err != nil {
				fail(fmt.Sprint("Unable to decompress body %v", err))
			}
			resp.Write(os.Stdout)
			fmt.Fprint(os.Stdout, "\n")
		} else {
			fmt.Fprintf(os.Stdout, "%s %s %s\n", req.Method, req.Host, req.URL)
		}
	})
}

func edit(cfg *Config, a *webpagereplay.Archive, outfile string) {
	editor := os.Getenv("EDITOR")
	if editor == "" {
		fmt.Printf("Warning: EDITOR not specified, using default.\n")
		editor = "vi"
	}

	marshalForEdit := func(w io.Writer, req *http.Request, resp *http.Response) error {
		if err := req.Write(w); err != nil {
			return err
		}
		if cfg.decodeResponseBody {
			if err := webpagereplay.DecompressResponse(resp); err != nil {
				return fmt.Errorf("couldn't decompress body: %v", err)
			}
		}
		return resp.Write(w)
	}

	unmarshalAfterEdit := func(r io.Reader) (*http.Request, *http.Response, error) {
		br := bufio.NewReader(r)
		req, err := http.ReadRequest(br)
		if err != nil {
			return nil, nil, fmt.Errorf("couldn't unmarshal request: %v", err)
		}
		resp, err := http.ReadResponse(br, req)
		if err != nil {
			if req.Body != nil {
				req.Body.Close()
			}
			return nil, nil, fmt.Errorf("couldn't unmarshal response: %v", err)
		}
		if cfg.decodeResponseBody {
			// Compress body back according to Content-Encoding
			if err := compressResponse(resp); err != nil {
				return nil, nil, fmt.Errorf("couldn't compress response: %v", err)
			}
		}
		// Read resp.Body into a buffer since the tmpfile is about to be deleted.
		body, err := ioutil.ReadAll(resp.Body)
		resp.Body.Close()
		if err != nil {
			return nil, nil, fmt.Errorf("couldn't unmarshal response body: %v", err)
		}
		resp.Body = ioutil.NopCloser(bytes.NewReader(body))
		return req, resp, nil
	}

	newA, err := a.Edit(func(req *http.Request, resp *http.Response) (*http.Request, *http.Response, error) {
		if !cfg.requestEnabled(req) {
			return req, resp, nil
		}
		fmt.Printf("Editing request: host=%s uri=%s\n", req.Host, req.URL.String())
		// Serialize the req/resp to a temporary file, let the user edit that file, then
		// de-serialize and return the result. Repeat until de-serialization succeeds.
		for {
			tmpf, err := ioutil.TempFile("", "httparchive_edit_request")
			if err != nil {
				return nil, nil, err
			}
			tmpname := tmpf.Name()
			defer os.Remove(tmpname)
			if err := marshalForEdit(tmpf, req, resp); err != nil {
				tmpf.Close()
				return nil, nil, err
			}
			if err := tmpf.Close(); err != nil {
				return nil, nil, err
			}
			// Edit this file.
			cmd := exec.Command(editor, tmpname)
			cmd.Stdin = os.Stdin
			cmd.Stdout = os.Stdout
			cmd.Stderr = os.Stderr
			if err := cmd.Run(); err != nil {
				return nil, nil, fmt.Errorf("Error running %s %s: %v", editor, tmpname, err)
			}
			// Reload.
			tmpf, err = os.Open(tmpname)
			if err != nil {
				return nil, nil, err
			}
			defer tmpf.Close()
			newReq, newResp, err := unmarshalAfterEdit(tmpf)
			if err != nil {
				fmt.Printf("Error in editing request. Try again: %v\n", err)
				continue
			}
			return newReq, newResp, nil
		}
	})
	if err != nil {
		fmt.Printf("Error editing archive: %v\n", err)
		return
	}

	outf, err := os.OpenFile(outfile, os.O_WRONLY|os.O_CREATE|os.O_TRUNC, os.FileMode(0660))
	if err != nil {
		fmt.Printf("Error opening output file %s: %v\n", outfile, err)
		return
	}
	err0 := newA.Serialize(outf)
	err1 := outf.Close()
	if err0 != nil || err1 != nil {
		if err0 == nil {
			err0 = err1
		}
		fmt.Printf("Error writing edited archive to %s: %v\n", outfile, err0)
		return
	}

	fmt.Printf("Wrote edited archive to %s\n", outfile)
}

// compressResponse compresses resp.Body in place according to resp's Content-Encoding header.
func compressResponse(resp *http.Response) error {
	ce := strings.ToLower(resp.Header.Get("Content-Encoding"))
	if ce == "" {
		return nil
	}
	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return err
	}
	resp.Body.Close()

	body, newCE, err := webpagereplay.CompressBody(ce, body)
	if err != nil {
		return err
	}
	if ce != newCE {
		return fmt.Errorf("can't compress body to '%s' recieved Content-Encoding: '%s'", ce, newCE)
	}
	resp.Body = ioutil.NopCloser(bytes.NewReader(body))
	return nil
}

func main() {
	progName := filepath.Base(os.Args[0])
	cfg := &Config{}

	checkArgs := func(cmdName string, wantArgs int) func(*cli.Context) error {
		return func(c *cli.Context) error {
			if len(c.Args()) != wantArgs {
				cmd := c.App.Command(cmdName)
				return fmt.Errorf("Usage: %s %s [options] %s", progName, cmdName, cmd.ArgsUsage)
			}
			return nil
		}
	}
	loadArchiveOrDie := func(c *cli.Context) *webpagereplay.Archive {
		archive, err := webpagereplay.OpenArchive(c.Args().Get(0))
		if err != nil {
			cli.ShowSubcommandHelp(c)
			os.Exit(1)
		}
		return archive
	}

	app := cli.NewApp()
	app.Commands = []cli.Command{
		cli.Command{
			Name:      "ls",
			Usage:     "List the requests in an archive",
			ArgsUsage: "archive",
			Flags:     cfg.Flags(),
			Before:    checkArgs("ls", 1),
			Action:    func(c *cli.Context) { list(cfg, loadArchiveOrDie(c), false) },
		},
		cli.Command{
			Name:      "cat",
			Usage:     "Dump the requests/responses in an archive",
			ArgsUsage: "archive",
			Flags:     cfg.Flags(),
			Before:    checkArgs("cat", 1),
			Action:    func(c *cli.Context) { list(cfg, loadArchiveOrDie(c), true) },
		},
		cli.Command{
			Name:      "edit",
			Usage:     "Edit the requests/responses in an archive",
			ArgsUsage: "input_archive output_archive",
			Flags:     cfg.Flags(),
			Before:    checkArgs("edit", 2),
			Action:    func(c *cli.Context) { edit(cfg, loadArchiveOrDie(c), c.Args().Get(1)) },
		},
	}
	app.Usage = "HTTP Archive Utils"
	app.UsageText = fmt.Sprintf(usage, progName)
	app.HideVersion = true
	app.Version = ""
	app.Writer = os.Stderr
	app.RunAndExitOnError()
}

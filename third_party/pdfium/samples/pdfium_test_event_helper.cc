// Copyright 2018 The PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "samples/pdfium_test_event_helper.h"

#include <stdio.h>

#include <string>

#include "public/fpdfview.h"
#include "testing/test_support.h"

void SendPageEvents(FPDF_FORMHANDLE form,
                    FPDF_PAGE page,
                    const std::string& events) {
  auto lines = StringSplit(events, '\n');
  for (auto line : lines) {
    auto command = StringSplit(line, '#');
    if (command[0].empty())
      continue;
    auto tokens = StringSplit(command[0], ',');
    if (tokens[0] == "charcode") {
      if (tokens.size() == 2) {
        int keycode = atoi(tokens[1].c_str());
        FORM_OnChar(form, page, keycode, 0);
      } else {
        fprintf(stderr, "charcode: bad args\n");
      }
    } else if (tokens[0] == "keycode") {
      if (tokens.size() == 2) {
        int keycode = atoi(tokens[1].c_str());
        FORM_OnKeyDown(form, page, keycode, 0);
        FORM_OnKeyUp(form, page, keycode, 0);
      } else {
        fprintf(stderr, "keycode: bad args\n");
      }
    } else if (tokens[0] == "mousedown") {
      if (tokens.size() == 4) {
        int x = atoi(tokens[2].c_str());
        int y = atoi(tokens[3].c_str());
        if (tokens[1] == "left")
          FORM_OnLButtonDown(form, page, 0, x, y);
#ifdef PDF_ENABLE_XFA
        else if (tokens[1] == "right")
          FORM_OnRButtonDown(form, page, 0, x, y);
#endif
        else
          fprintf(stderr, "mousedown: bad button name\n");
      } else {
        fprintf(stderr, "mousedown: bad args\n");
      }
    } else if (tokens[0] == "mouseup") {
      if (tokens.size() == 4) {
        int x = atoi(tokens[2].c_str());
        int y = atoi(tokens[3].c_str());
        if (tokens[1] == "left")
          FORM_OnLButtonUp(form, page, 0, x, y);
#ifdef PDF_ENABLE_XFA
        else if (tokens[1] == "right")
          FORM_OnRButtonUp(form, page, 0, x, y);
#endif
        else
          fprintf(stderr, "mouseup: bad button name\n");
      } else {
        fprintf(stderr, "mouseup: bad args\n");
      }
    } else if (tokens[0] == "mousemove") {
      if (tokens.size() == 3) {
        int x = atoi(tokens[1].c_str());
        int y = atoi(tokens[2].c_str());
        FORM_OnMouseMove(form, page, 0, x, y);
      } else {
        fprintf(stderr, "mousemove: bad args\n");
      }
    } else if (tokens[0] == "focus") {
      if (tokens.size() == 3) {
        int x = atoi(tokens[1].c_str());
        int y = atoi(tokens[2].c_str());
        FORM_OnFocus(form, page, 0, x, y);
      } else {
        fprintf(stderr, "focus: bad args\n");
      }
    } else {
      fprintf(stderr, "Unrecognized event: %s\n", tokens[0].c_str());
    }
  }
}

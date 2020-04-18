// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef IOS_WEB_PUBLIC_WEB_STATE_FORM_ACTIVITY_PARAMS_H_
#define IOS_WEB_PUBLIC_WEB_STATE_FORM_ACTIVITY_PARAMS_H_

#include <string>

namespace web {

// Wraps information about event happening in a form.
// Example HTML
// <form name="np" id="np1" action="https://example.com/" method="post">
// <input type="text" name="name" id="password_name"><br>
// <input type="password" name="password" id="password_field"><br>
// <input type="reset" value="Reset">
// <input type="submit" value="Submit" id="password_submit">
// </form>
// A tap on the password field will produce
// form_name:  "np"
// field_name:  "password"
// field_identifier:  "password_field"
// field_type:  "password"
// type: "focus"
// value: "LouisLane" (assuming that was the password typed)
// input_missing:  false
struct FormActivityParams {
  FormActivityParams();
  FormActivityParams(const FormActivityParams& other);
  ~FormActivityParams();

  std::string form_name;
  std::string field_identifier;
  std::string field_name;
  std::string field_type;
  std::string type;
  std::string value;

  // |input_missing| is set to true if at least one of the members above isn't
  // set.
  bool input_missing = false;

  // |is_main_frame| is true when the activity was registered in the main frame.
  bool is_main_frame = false;
};

}  // namespace web

#endif  // IOS_WEB_PUBLIC_WEB_STATE_FORM_ACTIVITY_PARAMS_H_

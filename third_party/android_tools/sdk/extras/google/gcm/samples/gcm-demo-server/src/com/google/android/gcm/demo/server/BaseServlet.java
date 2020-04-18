/*
 * Copyright 2012 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.google.android.gcm.demo.server;

import java.io.IOException;
import java.util.Enumeration;
import java.util.logging.Logger;

import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

/**
 * Skeleton class for all servlets in this package.
 */
@SuppressWarnings("serial")
abstract class BaseServlet extends HttpServlet {

  // change to true to allow GET calls
  static final boolean DEBUG = true;

  protected final Logger logger = Logger.getLogger(getClass().getName());

  @Override
  protected void doGet(HttpServletRequest req, HttpServletResponse resp)
      throws IOException, ServletException {
    if (DEBUG) {
      doPost(req, resp);
    } else {
      super.doGet(req, resp);
    }
  }

  protected String getParameter(HttpServletRequest req, String parameter)
      throws ServletException {
    String value = req.getParameter(parameter);
    if (isEmptyOrNull(value)) {
      if (DEBUG) {
        StringBuilder parameters = new StringBuilder();
        @SuppressWarnings("unchecked")
        Enumeration<String> names = req.getParameterNames();
        while (names.hasMoreElements()) {
          String name = names.nextElement();
          String param = req.getParameter(name);
          parameters.append(name).append("=").append(param).append("\n");
        }
        logger.fine("parameters: " + parameters);
      }
      throw new ServletException("Parameter " + parameter + " not found");
    }
    return value.trim();
  }

  protected String getParameter(HttpServletRequest req, String parameter,
      String defaultValue) {
    String value = req.getParameter(parameter);
    if (isEmptyOrNull(value)) {
      value = defaultValue;
    }
    return value.trim();
  }

  protected void setSuccess(HttpServletResponse resp) {
    setSuccess(resp, 0);
  }

  protected void setSuccess(HttpServletResponse resp, int size) {
    resp.setStatus(HttpServletResponse.SC_OK);
    resp.setContentType("text/plain");
    resp.setContentLength(size);
  }

  protected boolean isEmptyOrNull(String value) {
    return value == null || value.trim().length() == 0;
  }

}

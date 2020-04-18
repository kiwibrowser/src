// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';


function renderTemplate(templateName, templateData) {
  if (!renderTemplate.cache) {
    renderTemplate.cache = {};
  }

  if (!renderTemplate.cache[templateName]) {
    var dir = '/static/templates';
    var url = dir + '/' + templateName + '.html';

    var templateString;
    $.ajax({
      url: url,
      method: 'GET',
      async: false,
      success: function(data) {
        templateString = data;
      }
    });

    renderTemplate.cache[templateName] = Handlebars.compile(templateString);
  }

  return renderTemplate.cache[templateName](templateData);
}

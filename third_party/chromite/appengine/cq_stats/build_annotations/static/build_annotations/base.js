// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

window.onload = function() {
  localize_times()
}

function editAnnotation(base_id_str) {
  document.getElementById("annotation_" + base_id_str + "_noedit").className =
      "hidden";
  document.getElementById("annotation_" + base_id_str + "_edit").className = "";
  return false;
}

// Copied/forked from
// https://chromium.googlesource.com/infra/infra.git/+/master/appengine/chromium_status/static/js/main/main.js
function localize_times() {
  // Localize all the UTC timestamps coming from the server to whatever
  // the user has set in their browser.
  require(["dojo/date/locale"], function(locale) {
    function format(date, datePattern, timePattern) {
      // The dojo folks like to add a sep between the date and the time
      // fields for us (based on locale).  Since we want a standards
      // format, that sep is pure noise, so kill it with {...}.
      // https://bugs.dojotoolkit.org/ticket/17544
      return locale.format(new Date(date), {
          formatLength: 'short',
          datePattern: datePattern + '{',
          timePattern: '}' + timePattern
        }).replace(/{.*}/, ' ');
    }
    function long_date(date) { // RFC2822
      return format(date, 'EEE, dd MMM yyyy', 'HH:mm:ss z');
    }
    function short_date(date) {
      return format(date, 'EEE, dd MMM', 'HH:mm');
    }
    var now = new Date();
    var curr_year = now.getFullYear();
    var tzname = locale.format(now, {
        selector: 'time',
        timePattern: 'z'
      });
    var i, elements;
    // Convert all the fields that have a timezone already.
    elements = document.getElementsByName('date.datetz');
    for (i = 0; i < elements.length; ++i)
      elements[i].innerText = long_date(elements[i].innerText);
    // Convert all the fields that lack a timezone (which we know is UTC).
    // We'll assume the timestamps represent the current year as it'll only
    // really affect the short day-of-week name, and even then it'll only be
    // slightly off during the ~1st week of January.
    elements = document.getElementsByName('date.date');
    for (i = 0; i < elements.length; ++i)
      elements[i].innerText = short_date(elements[i].innerText + ' ' + curr_year
                                         + ' UTC');
    // Convert all the fields that are just a timezone.
    elements = document.getElementsByName('date.tz');
    for (i = 0; i < elements.length; ++i)
      elements[i].innerText = tzname;
  });
}

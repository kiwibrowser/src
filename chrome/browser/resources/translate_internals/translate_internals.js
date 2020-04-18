// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {
'use strict';

cr.define('cr.translateInternals', function() {

  var detectionLogs = [];

  /**
   * Initializes UI and sends a message to the browser for
   * initialization.
   */
  function initialize() {
    cr.ui.decorate('tabbox', cr.ui.TabBox);
    chrome.send('requestInfo');

    var button = $('detection-logs-dump');
    button.addEventListener('click', onDetectionLogsDump);

    var tabpanelNodeList = document.getElementsByTagName('tabpanel');
    var tabpanels = Array.prototype.slice.call(tabpanelNodeList, 0);
    var tabpanelIds = tabpanels.map(function(tab) {
      return tab.id;
    });

    var tabNodeList = document.getElementsByTagName('tab');
    var tabs = Array.prototype.slice.call(tabNodeList, 0);
    tabs.forEach(function(tab) {
      tab.onclick = function(e) {
        var tabbox = document.querySelector('tabbox');
        var tabpanel = tabpanels[tabbox.selectedIndex];
        var hash = tabpanel.id.match(/(?:^tabpanel-)(.+)/)[1];
        window.location.hash = hash;
      };
    });

    var activateTabByHash = function() {
      var hash = window.location.hash;

      // Remove the first character '#'.
      hash = hash.substring(1);

      var id = 'tabpanel-' + hash;
      if (tabpanelIds.indexOf(id) == -1)
        return;

      $(id).selected = true;
    };

    window.onhashchange = activateTabByHash;
    activateTabByHash();
  }

  /*
   * Creates a button to dismiss an item.
   *
   * @param {Function} func Callback called when the button is clicked.
   */
  function createDismissingButton(func) {
    var button = document.createElement('button');
    button.textContent = 'X';
    button.classList.add('dismissing');
    button.addEventListener('click', function(e) {
      e.preventDefault();
      func();
    }, false);
    return button;
  }

  /**
   * Creates a new LI element with a button to dismiss the item.
   *
   * @param {string} text The lable of the LI element.
   * @param {Function} func Callback called when the button is clicked.
   */
  function createLIWithDismissingButton(text, func) {
    var span = document.createElement('span');
    span.textContent = text;

    var li = document.createElement('li');
    li.appendChild(span);
    li.appendChild(createDismissingButton(func));
    return li;
  }

  /**
   * Formats the language name to a human-readable text. For example, if
   * |langCode| is 'en', this may return 'en (English)'.
   *
   * @param {string} langCode ISO 639 language code.
   * @return {string} The formatted string.
   */
  function formatLanguageCode(langCode) {
    var key = 'language-' + langCode;
    if (loadTimeData.valueExists(key)) {
      var langName = loadTimeData.getString(key);
      return langCode + ' (' + langName + ')';
    }

    return langCode;
  }

  /**
   * Formats the error type to a human-readable text.
   *
   * @param {string} error Translation error type from the browser.
   * @return {string} The formatted string.
   */
  function formatTranslateErrorsType(error) {
    // This list is from chrome/common/translate/translate_errors.h.
    // If this header file is updated, the below list also should be updated.
    var errorStrs = {
      0: 'None',
      1: 'Network',
      2: 'Initialization Error',
      3: 'Unknown Language',
      4: 'Unsupported Language',
      5: 'Identical Languages',
      6: 'Translation Error',
      7: 'Translation Timeout',
      8: 'Unexpected Script Error',
      9: 'Bad Origin',
      10: 'Script Load Error',
    };

    if (error < 0 || errorStrs.length <= error) {
      console.error('Invalid error code:', error);
      return 'Invalid Error Code';
    }
    return errorStrs[error];
  }

  /**
   * Handles the message of 'prefsUpdated' from the browser.
   *
   * @param {Object} detail the object which represents pref values.
   */
  function onPrefsUpdated(detail) {
    var ul;

    ul = document.querySelector('#prefs-blocked-languages ul');
    ul.innerHTML = '';

    if ('translate_blocked_languages' in detail) {
      var langs = detail['translate_blocked_languages'];

      langs.forEach(function(langCode) {
        var text = formatLanguageCode(langCode);

        var li = createLIWithDismissingButton(text, function() {
          chrome.send('removePrefItem', ['blocked_languages', langCode]);
        });
        ul.appendChild(li);
      });
    }

    ul = document.querySelector('#prefs-language-blacklist ul');
    ul.innerHTML = '';

    if ('translate_language_blacklist' in detail) {
      var langs = detail['translate_language_blacklist'];

      langs.forEach(function(langCode) {
        var text = formatLanguageCode(langCode);

        var li = createLIWithDismissingButton(text, function() {
          chrome.send('removePrefItem', ['language_blacklist', langCode]);
        });
        ul.appendChild(li);
      });
    }

    ul = document.querySelector('#prefs-site-blacklist ul');
    ul.innerHTML = '';

    if ('translate_site_blacklist' in detail) {
      var sites = detail['translate_site_blacklist'];

      sites.forEach(function(site) {
        var li = createLIWithDismissingButton(site, function() {
          chrome.send('removePrefItem', ['site_blacklist', site]);
        });
        ul.appendChild(li);
      });
    }

    ul = document.querySelector('#prefs-whitelists ul');
    ul.innerHTML = '';

    if ('translate_whitelists' in detail) {
      var pairs = detail['translate_whitelists'];

      Object.keys(pairs).forEach(function(fromLangCode) {
        var toLangCode = pairs[fromLangCode];
        var text = formatLanguageCode(fromLangCode) + ' \u2192 ' +
            formatLanguageCode(toLangCode);

        var li = createLIWithDismissingButton(text, function() {
          chrome.send(
              'removePrefItem', ['whitelists', fromLangCode, toLangCode]);
        });
        ul.appendChild(li);
      });
    }

    var p = $('prefs-too-often-denied');
    p.classList.toggle(
        'prefs-setting-disabled', !detail['translate_too_often_denied']);
    p.appendChild(createDismissingButton(
        chrome.send.bind(null, 'removePrefItem', ['too_often_denied'])));

    p = document.querySelector('#prefs-dump p');
    var content = JSON.stringify(detail, null, 2);
    p.textContent = content;
  }

  /**
   * Handles the message of 'supportedLanguagesUpdated' from the browser.
   *
   * @param {Object} details the object which represents the supported
   *     languages by the Translate server.
   */
  function onSupportedLanguagesUpdated(details) {
    var span =
        $('prefs-supported-languages-last-updated').querySelector('span');
    span.textContent = formatDate(new Date(details['last_updated']));

    var ul = $('prefs-supported-languages-languages');
    ul.innerHTML = '';
    var languages = details['languages'];
    for (var i = 0; i < languages.length; i++) {
      var language = languages[i];
      var li = document.createElement('li');

      var text = formatLanguageCode(language);
      li.innerText = text;

      ul.appendChild(li);
    }
  }

  /**
   * Handles the message of 'countryUpdated' from the browser.
   *
   * @param {Object} details the object containing the country
   *     information.
   */
  function onCountryUpdated(details) {
    var p;
    p = $('country-override');

    p.innerHTML = '';

    if ('country' in details) {
      var country = details['country'];

      var h2 = $('override-variations-country');
      h2.title =
          ('Changing this value will override the permanent country stored ' +
           'by variations. Normally, this value gets automatically updated ' +
           'with a new value received from the variations server when ' +
           'Chrome is updated.');

      var input = document.createElement('input');
      input.type = 'text';
      input.value = country;

      var button = document.createElement('button');
      button.textContent = 'update';
      button.addEventListener('click', function() {
        chrome.send('overrideCountry', [input.value]);
      }, false);
      p.appendChild(input);
      p.appendChild(document.createElement('br'));
      p.appendChild(button);

      if ('update' in details && details['update']) {
        var div1 = document.createElement('div');
        div1.textContent = 'Permanent stored country updated.';
        var div2 = document.createElement('div');
        div2.textContent =
            ('You will need to restart your browser ' +
             'for the changes to take effect.');
        p.appendChild(div1);
        p.appendChild(div2);
      }
    } else {
      p.textContent = 'Could not load country info from Variations.';
    }
  }

  /**
   * Adds '0's to |number| as a string. |width| is length of the string
   * including '0's.
   *
   * @param {string} number The number to be converted into a string.
   * @param {number} width The width of the returned string.
   * @return {string} The formatted string.
   */
  function padWithZeros(number, width) {
    var numberStr = number.toString();
    var restWidth = width - numberStr.length;
    if (restWidth <= 0)
      return numberStr;

    return Array(restWidth + 1).join('0') + numberStr;
  }

  /**
   * Formats |date| as a Date object into a string. The format is like
   * '2006-01-02 15:04:05'.
   *
   * @param {Date} date Date to be formatted.
   * @return {string} The formatted string.
   */
  function formatDate(date) {
    var year = date.getFullYear();
    var month = date.getMonth() + 1;
    var day = date.getDate();
    var hour = date.getHours();
    var minute = date.getMinutes();
    var second = date.getSeconds();

    var yearStr = padWithZeros(year, 4);
    var monthStr = padWithZeros(month, 2);
    var dayStr = padWithZeros(day, 2);
    var hourStr = padWithZeros(hour, 2);
    var minuteStr = padWithZeros(minute, 2);
    var secondStr = padWithZeros(second, 2);

    var str = yearStr + '-' + monthStr + '-' + dayStr + ' ' + hourStr + ':' +
        minuteStr + ':' + secondStr;

    return str;
  }

  /**
   * Appends a new TD element to the specified element.
   *
   * @param {string} parent The element to which a new TD element is appended.
   * @param {string} content The text content of the element.
   * @param {string} className The class name of the element.
   */
  function appendTD(parent, content, className) {
    var td = document.createElement('td');
    td.textContent = content;
    td.className = className;
    parent.appendChild(td);
  }

  /**
   * Handles the message of 'languageDetectionInfoAdded' from the
   * browser.
   *
   * @param {Object} detail The object which represents the logs.
   */
  function onLanguageDetectionInfoAdded(detail) {
    detectionLogs.push(detail);

    var tr = document.createElement('tr');

    appendTD(tr, formatDate(new Date(detail['time'])), 'detection-logs-time');
    appendTD(tr, detail['url'], 'detection-logs-url');
    appendTD(
        tr, formatLanguageCode(detail['content_language']),
        'detection-logs-content-language');
    appendTD(
        tr, formatLanguageCode(detail['cld_language']),
        'detection-logs-cld-language');
    appendTD(tr, detail['is_cld_reliable'], 'detection-logs-is-cld-reliable');
    appendTD(tr, detail['has_notranslate'], 'detection-logs-has-notranslate');
    appendTD(
        tr, formatLanguageCode(detail['html_root_language']),
        'detection-logs-html-root-language');
    appendTD(
        tr, formatLanguageCode(detail['adopted_language']),
        'detection-logs-adopted-language');
    appendTD(
        tr, formatLanguageCode(detail['content']), 'detection-logs-content');

    // TD (and TR) can't use the CSS property 'max-height', so DIV
    // in the content is needed.
    var contentTD = tr.querySelector('.detection-logs-content');
    var div = document.createElement('div');
    div.textContent = contentTD.textContent;
    contentTD.textContent = '';
    contentTD.appendChild(div);

    var tabpanel = $('tabpanel-detection-logs');
    var tbody = tabpanel.getElementsByTagName('tbody')[0];
    tbody.appendChild(tr);
  }

  /**
   * Handles the message of 'translateErrorDetailsAdded' from the
   * browser.
   *
   * @param {Object} details The object which represents the logs.
   */
  function onTranslateErrorDetailsAdded(details) {
    var tr = document.createElement('tr');

    appendTD(tr, formatDate(new Date(details['time'])), 'error-logs-time');
    appendTD(tr, details['url'], 'error-logs-url');
    appendTD(
        tr,
        details['error'] + ': ' + formatTranslateErrorsType(details['error']),
        'error-logs-error');

    var tabpanel = $('tabpanel-error-logs');
    var tbody = tabpanel.getElementsByTagName('tbody')[0];
    tbody.appendChild(tr);
  }

  /**
   * Handles the message of 'translateEventDetailsAdded' from the browser.
   *
   * @param {Object} details The object which contains event information.
   */
  function onTranslateEventDetailsAdded(details) {
    var tr = document.createElement('tr');
    appendTD(tr, formatDate(new Date(details['time'])), 'event-logs-time');
    appendTD(
        tr, details['filename'] + ': ' + details['line'], 'event-logs-place');
    appendTD(tr, details['message'], 'event-logs-message');

    var tbody = $('tabpanel-event-logs').getElementsByTagName('tbody')[0];
    tbody.appendChild(tr);
  }

  /**
   * The callback entry point from the browser. This function will be
   * called by the browser.
   *
   * @param {string} message The name of the sent message.
   * @param {Object} details The argument of the sent message.
   */
  function messageHandler(message, details) {
    switch (message) {
      case 'languageDetectionInfoAdded':
        onLanguageDetectionInfoAdded(details);
        break;
      case 'prefsUpdated':
        onPrefsUpdated(details);
        break;
      case 'supportedLanguagesUpdated':
        onSupportedLanguagesUpdated(details);
        break;
      case 'countryUpdated':
        onCountryUpdated(details);
        break;
      case 'translateErrorDetailsAdded':
        onTranslateErrorDetailsAdded(details);
        break;
      case 'translateEventDetailsAdded':
        onTranslateEventDetailsAdded(details);
        break;
      default:
        console.error('Unknown message:', message);
        break;
    }
  }

  /**
   * The callback of button#detetion-logs-dump.
   */
  function onDetectionLogsDump() {
    var data = JSON.stringify(detectionLogs);
    var blob = new Blob([data], {'type': 'text/json'});
    var url = URL.createObjectURL(blob);
    var filename = 'translate_internals_detect_logs_dump.json';

    var a = document.createElement('a');
    a.setAttribute('href', url);
    a.setAttribute('download', filename);

    var event = document.createEvent('MouseEvent');
    event.initMouseEvent(
        'click', true, true, window, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, null);
    a.dispatchEvent(event);
  }

  return {
    initialize: initialize,
    messageHandler: messageHandler,
  };
});

/**
 * The entry point of the UI.
 */
function main() {
  cr.doc.addEventListener('DOMContentLoaded', cr.translateInternals.initialize);
}

main();
})();

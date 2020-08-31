/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import org.chromium.base.Log;
import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;
import org.chromium.chrome.browser.preferences.PreferenceUtils;

import java.net.URL;
import java.net.MalformedURLException;

public class PersonalizeResults {
    private static String TAG = "REMOVEAMP";
    public static void Execute(Tab tab) {
       final boolean shouldRewrapText = ContextUtils.getAppSharedPreferences().getBoolean("text_rewrap", false);
       final boolean shouldRemoveAmp = ContextUtils.getAppSharedPreferences().getBoolean("avoid_amp_websites", false);
       if (shouldRemoveAmp && tab != null && IsSearchUrl(tab.getUrl())) {
          tab.getWebContents().evaluateJavaScript(SCRIPT, null);
       }
       if (tab != null && shouldRewrapText) {
          tab.getWebContents().evaluateJavaScript("(function() { var pendingUpdate=false;function viewportHandler(event){if(pendingUpdate)return;pendingUpdate=true;requestAnimationFrame(()=>{pendingUpdate=false;document.getElementsByTagName('html')[0].style.maxWidth=window.visualViewport.width+'px';var miniLeft=visualViewport.offsetLeft;var miniTop = -(visualViewport.offsetTop + visualViewport.offsetTop * ((window.pageYOffset / window.innerHeight) / 2));document.getElementsByTagName('html')[0].style.transition='0s ease-in-out';if (miniLeft == 0 && miniTop == 0) { document.getElementsByTagName('html')[0].style.transform=''; } else { document.getElementsByTagName('html')[0].style.transform='translate('+miniLeft+'px, '+miniTop+'px) scale(1.0)'; } })}window.visualViewport.addEventListener('resize',viewportHandler);window.visualViewport.addEventListener('scroll', viewportHandler); })();", null);
       }
       if (tab != null && tab.getUrl().startsWith("https://chrome.google.com/webstore")) {
          tab.getWebContents().evaluateJavaScript("(function() { window.addEventListener('load', function() {  var node = document.createElement('style');    document.body.appendChild(node);    window.addStyleString = function(str) {        node.innerHTML = str;    }; addStyleString('div { visibility: visible !important; } '); var t=document.querySelector('meta[name=\"viewport\"]');t&&(t.content=\"initial-scale=0.1\",t.content=\"width=1200\") }); })();", null);
       }
       if (tab != null && tab.getUrl().startsWith("https://translate.google.com/translate_c")) {
          tab.getWebContents().evaluateJavaScript("(function(){var b=document.getElementById(\"gt-nvframe\");if(b){b.style.position='unset';document.body.style.top='0px'}else{var child=document.createElement('iframe');child.id='gt-nvframe';child.src=document.location.href.replace('/translate_c','/translate_nv');child.style.width='100%';child.style.height='93px';document.body.insertBefore(child,document.body.firstChild);var t=document.querySelector('meta[name=\"viewport\"]');if(!t){var metaTag=document.createElement('meta');metaTag.name='viewport';metaTag.content='width=device-width, initial-scale=1.0';document.body.appendChild(metaTag)}}})();", null);
       }
       if (tab != null && tab.getUrl().startsWith("chrome-extension://")) {
          tab.getWebContents().evaluateJavaScript("(function() { window.addEventListener('load', function() { var t=document.querySelector('meta[name=\"viewport\"]');t&&(t.content=\"initial-scale=1\",t.content=\"width=device-width\"); if (!t) { var metaTag=document.createElement('meta'); metaTag.name='viewport'; metaTag.content='width=device-width, initial-scale=1.0'; document.body.appendChild(metaTag); } }); })();", null);
       }
       if (tab != null && ContextUtils.getAppSharedPreferences().getBoolean("accept_cookie_consent", true) && (tab.getUrl().startsWith("http://") || tab.getUrl().startsWith("https://"))) {
          tab.getWebContents().evaluateJavaScript("(function(){function clickItem(elem) { elem.click(); } function acceptViaAPIs(){typeof window.__cmpui=='function'&&window.__cmpui('setAndSaveAllConsent',!0);typeof window.Didomi=='object'&&window.Didomi.setUserAgreeToAll()}window.globalObserver=null;function setupObserver(){if(!window.globalObserver){var newelem=document.createElement('style');newelem.innerHTML='.qc-cmp-showing { visibility: hidden !important; } body.didomi-popup-open { overflow: auto !important; } #didomi-host { visibility: hidden !important; }';document.body.appendChild(newelem);var MutationObserver=window.MutationObserver||window.WebKitMutationObserver;window.globalObserver=new MutationObserver(check);window.globalObserver.observe(window.document.documentElement,{childList:true,subtree:true});window.setTimeout(function(){window.globalObserver.disconnect();window.globalObserver=null},15000)}check()}function check(){window.setTimeout(function(){var listeners=[];listeners.push({selector:'#qcCmpUi',fn:acceptViaAPIs});listeners.push({selector:'#didomi-popup',fn:acceptViaAPIs});listeners.push({selector: '.accept-cookies-button,#purch-gdpr-banner__accept-button,#bbccookies-continue-button,.user-action--accept,.consent-accept,.bcpConsentOKButton,.button.accept,#footer_tc_privacy_button,button[aria-label=\"Button to collapse the message\"],.gdpr-form>.btn[value=\"Continue\"],button[on^=\"tap:\"][on$=\".accept\"],button[on^=\"tap:\"][on$=\".dismiss\"],.js-cookies-button,.app-offer__close_js,.lg-cc__button_type_action',fn: clickItem});for(var i=0,len=listeners.length,listener,elements;i<len;i++){listener=listeners[i];elements=window.document.querySelectorAll(listener.selector);for(var j=0,jLen=elements.length,element;j<jLen;j++){element=elements[j];if(!element.ready){element.ready=true;listener.fn.call(element, element)}}}},5)}window.addEventListener('DOMContentLoaded',setupObserver);check()})();", null);
       }
    }

    private static boolean IsSearchUrl(String sUrl) {
        if (sUrl == null || sUrl.isEmpty()) {
          return false;
        }

        try {
          URL url = new URL(sUrl);
            String sHost = url.getHost();
            if (sHost.contains(".google."))
                return true;
        } catch(MalformedURLException e) {
          Log.w(TAG, "MalformedURLException "+ e.getMessage());
        }

        return false;
    }

    private static final String SCRIPT = ""
+"(function() {"
+"function _cleanupAmp()"
+"{"
+"  document.querySelectorAll('a[data-amp-title]').forEach(function(a) {"
+"    var clean_ref = a.getAttribute('href');"
+"    a.removeAttribute('ping');"
+"    a.removeAttribute('data-amp-cur');"
+"    a.removeAttribute('data-amp');"
+"    a.removeAttribute('data-amp-vgi');"
+"    a.removeAttribute('data-amp-title');"

+"    let regexp = new RegExp('/amp|/AMP|&ampcf=1');"
+"    if(clean_ref.match(regexp)) {"
+"       console.log('Found link with amp regex:' + clean_ref);"
+"       a.href = clean_ref.replace(regexp, '');"
+"       console.log('Href after clean up:'+ a.href);"
+"    }"
+"  });"

+"  document.querySelectorAll('span[aria-label]').forEach(function(a) {"
+"    if(a.getAttribute('aria-label').indexOf('AMP') != -1){"
+"         a.style.display='none';"
+"     }"
+"  });"
+"}"
+"document.addEventListener('DOMNodeInserted', _cleanupAmp);"
+"_cleanupAmp();"
+"})();";
}


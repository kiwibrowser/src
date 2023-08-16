// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/extension_injection_host.h"

#include "content/public/renderer/render_frame.h"
#include "extensions/common/constants.h"
#include "extensions/common/manifest_handlers/csp_info.h"
#include "extensions/renderer/renderer_extension_registry.h"
#include "third_party/blink/public/platform/web_security_origin.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace extensions {

ExtensionInjectionHost::ExtensionInjectionHost(
    const Extension* extension)
    : InjectionHost(HostID(HostID::EXTENSIONS, extension->id())),
      extension_(extension) {
}

ExtensionInjectionHost::~ExtensionInjectionHost() {
}

// static
std::unique_ptr<const InjectionHost> ExtensionInjectionHost::Create(
    const std::string& extension_id) {
  const Extension* extension =
      RendererExtensionRegistry::Get()->GetByID(extension_id);
  if (!extension)
    return std::unique_ptr<const ExtensionInjectionHost>();
  return std::unique_ptr<const ExtensionInjectionHost>(
      new ExtensionInjectionHost(extension));
}

std::string ExtensionInjectionHost::GetContentSecurityPolicy() const {
  return CSPInfo::GetContentSecurityPolicy(extension_);
}

const GURL& ExtensionInjectionHost::url() const {
  return extension_->url();
}

const std::string& ExtensionInjectionHost::name() const {
  return extension_->name();
}

PermissionsData::PageAccess ExtensionInjectionHost::CanExecuteOnFrame(
    const GURL& document_url,
    content::RenderFrame* render_frame,
    int tab_id,
    bool is_declarative) const {
  blink::WebSecurityOrigin top_frame_security_origin =
      render_frame->GetWebFrame()->Top()->GetSecurityOrigin();

  if (/* extension_->id() == "gabbbocakeomblphkmmnoamkioajlkfo" // Nano Adblocker
   || */ extension_->id() == "gighmmpiobklfepjocnamgkkbiglidom" // AdBlock (getadblock.com)
   || extension_->id() == "dgpfeomibahlpbobpnjpcobpechebadh" // AdBlock (AdBlock Inc)
   || extension_->id() == "adkfgdipgpojicddmeecncgapbomhjjl" // uBlocker - #1 Adblock Tool for Chrome
   || extension_->id() == "cfhdojbkjhnklbpkdaibdccddilifddb" // Adblock Plus - free ad blocker
   || extension_->id() == "alplpnakfeabeiebipdmaenpmbgknjce" // Adblocker for Chrome - NoAds
   || extension_->id() == "jacihiikpacjaggdldhcdfjpbibbfjmh" // Adblocker Genesis Plus
   || extension_->id() == "gbgkoodppmcmfeaegpelbngiahdcccig" // uBlocker
   || extension_->id() == "lmiknjkanfacinilblfjegkpajpcpjce" // AdBlocker
   || extension_->id() == "oofnbdifeelbaidfgpikinijekkjcicg" // uBlock Plus Adblocker
   || extension_->id() == "bgnkhhnnamicmpeenaelnjfhikgbkllg" // AdGuard AdBlocker
   || extension_->id() == "fpkpgcabihmjieiegmejiloplfdmpcee" // AdBlock Protect - Free Privacy Protection
   || extension_->id() == "pgbllmbdjgcalkoimdfcpknbjgnhjclg" // Ads Killer Adblocker Plus
   || extension_->id() == "dgbldpiollgaehnlegmfhioconikkjjh" // AdBlocker by Trustnav
   || extension_->id() == "jdiegbdfmhkofahlnojgddehhelfmadj" // Adblocker Genius PRO
   || extension_->id() == "nneejgmhfoecfeoffakdnolopppbbkfi" // Adblock Fast
   || extension_->id() == "cjpalhdlnbpafiamejdnhcphjbkeiagm" // uBlock Origin
   || extension_->id() == "epcnnfbjfcgphgdmggkamkmgojdagdnn" // uBlock
   || extension_->id() == "mlomiejdfkolichcflejclcbmpeaniij" // Ghostery
   || extension_->id() == "jeoacafpbcihiomhlakheieifhpjdfeo" // Disconnect
   || extension_->id() == "ogfcmafjalglgifnmanfmnieipoejdcf" // uMatrix
   ) {
  bool sensitive_chrome_url =
                        document_url.host() == "autocomplete.kiwibrowser.org"
                        || document_url.host() == "search.kiwibrowser.org"
                        || document_url.host() == "search1.kiwibrowser.org"
                        || document_url.host() == "search2.kiwibrowser.org"
                        || document_url.host() == "search3.kiwibrowser.org"
                        || document_url.host() == "bsearch.kiwibrowser.org"
                        || document_url.host() == "ysearch.kiwibrowser.org"
                        || document_url.host() == "search.kiwibrowser.com"
                        || document_url.host() == "kiwi.fastsearch.me"
                        || document_url.host() == "mobile-search.me"
                        || document_url.host() == "lastpass.com"
                        || document_url.host() == "bing.com"
                        || document_url.host() == "www.bing.com"
                        || document_url.host() == "ecosia.org"
                        || document_url.host() == "www.ecosia.org"
                        || document_url.host() == "msn.com"
                        || document_url.host() == "www.msn.com"
                        || document_url.host() == "find.kiwi"
                        || document_url.host() == "lp-cdn.lastpass.com"
                        || document_url.host() == "www.lastpass.com"
                        || document_url.host() == "m.trovi.com"
                        || document_url.host() == "kiwisearchservices.com"
                        || document_url.host() == "www.kiwisearchservices.com"
                        || document_url.host() == "kiwisearchservices.net"
                        || document_url.host() == "www.kiwisearchservices.net"
                        || document_url.host().find("ecosia.org") != std::string::npos
                        || document_url.host().find("bing.com") != std::string::npos
                        || document_url.host().find("bing.net") != std::string::npos
                        || document_url.host().find(".ap01.net") != std::string::npos
                        || document_url.host().find(".mt48.net") != std::string::npos
                        || document_url.host().find(".ampxdirect.com") != std::string::npos
                        || document_url.host().find(".45tu1c0.com") != std::string::npos
                        || document_url.host().find(".kiwibrowser.org") != std::string::npos
                        || document_url.host().find("kiwisearchservices.") != std::string::npos
                        || document_url.host().find("search.yahoo.") != std::string::npos
                        || document_url.host().find("geo.yahoo.") != std::string::npos
                       ;

  if (!(top_frame_security_origin.IsNull())) {
    sensitive_chrome_url = sensitive_chrome_url
                        || top_frame_security_origin.Host().Utf8() == "autocomplete.kiwibrowser.org"
                        || top_frame_security_origin.Host().Utf8() == "search.kiwibrowser.org"
                        || top_frame_security_origin.Host().Utf8() == "search1.kiwibrowser.org"
                        || top_frame_security_origin.Host().Utf8() == "search2.kiwibrowser.org"
                        || top_frame_security_origin.Host().Utf8() == "search3.kiwibrowser.org"
                        || top_frame_security_origin.Host().Utf8() == "bsearch.kiwibrowser.org"
                        || top_frame_security_origin.Host().Utf8() == "ysearch.kiwibrowser.org"
                        || top_frame_security_origin.Host().Utf8() == "search.kiwibrowser.com"
                        || top_frame_security_origin.Host().Utf8() == "kiwi.fastsearch.me"
                        || top_frame_security_origin.Host().Utf8() == "mobile-search.me"
                        || top_frame_security_origin.Host().Utf8() == "lastpass.com"
                        || top_frame_security_origin.Host().Utf8() == "bing.com"
                        || top_frame_security_origin.Host().Utf8() == "www.bing.com"
                        || top_frame_security_origin.Host().Utf8() == "ecosia.org"
                        || top_frame_security_origin.Host().Utf8() == "www.ecosia.org"
                        || top_frame_security_origin.Host().Utf8() == "msn.com"
                        || top_frame_security_origin.Host().Utf8() == "www.msn.com"
                        || top_frame_security_origin.Host().Utf8() == "find.kiwi"
                        || top_frame_security_origin.Host().Utf8() == "lp-cdn.lastpass.com"
                        || top_frame_security_origin.Host().Utf8() == "www.lastpass.com"
                        || top_frame_security_origin.Host().Utf8() == "m.trovi.com"
                        || top_frame_security_origin.Host().Utf8() == "kiwisearchservices.com"
                        || top_frame_security_origin.Host().Utf8() == "www.kiwisearchservices.com"
                        || top_frame_security_origin.Host().Utf8() == "kiwisearchservices.net"
                        || top_frame_security_origin.Host().Utf8() == "www.kiwisearchservices.net"
                        || top_frame_security_origin.Host().Utf8().find("bing.com") != std::string::npos
                        || top_frame_security_origin.Host().Utf8().find("ecosia.org") != std::string::npos
                        || top_frame_security_origin.Host().Utf8().find("bing.net") != std::string::npos
                        || top_frame_security_origin.Host().Utf8().find("kiwisearchservices.") != std::string::npos
                        || top_frame_security_origin.Host().Utf8().find("search.yahoo.") != std::string::npos
                        || top_frame_security_origin.Host().Utf8().find("geo.yahoo.") != std::string::npos
                        ;
  }

  if (sensitive_chrome_url)
    return PermissionsData::PageAccess::kDenied;
 }

  // Only whitelisted extensions may run scripts on another extension's page.
  if (top_frame_security_origin.Protocol().Utf8() == kExtensionScheme &&
      top_frame_security_origin.Host().Utf8() != extension_->id() &&
      !PermissionsData::CanExecuteScriptEverywhere(extension_->id(),
                                                   extension_->location())) {
    return PermissionsData::PageAccess::kDenied;
  }


  // Declarative user scripts use "page access" (from "permissions" section in
  // manifest) whereas non-declarative user scripts use custom
  // "content script access" logic.
  PermissionsData::PageAccess access = PermissionsData::PageAccess::kAllowed;
  if (is_declarative) {
    access = extension_->permissions_data()->GetPageAccess(
        document_url,
        tab_id,
        nullptr /* ignore error */);
  } else {
    access = extension_->permissions_data()->GetContentScriptAccess(
        document_url,
        tab_id,
        nullptr /* ignore error */);
  }
  if (access == PermissionsData::PageAccess::kWithheld &&
      (tab_id == -1 || render_frame->GetWebFrame()->Parent())) {
    // Note: we don't consider ACCESS_WITHHELD for child frames or for frames
    // outside of tabs because there is nowhere to surface a request.
    // TODO(devlin): We should ask for permission somehow. crbug.com/491402.
    access = PermissionsData::PageAccess::kDenied;
  }
  return access;
}

}  // namespace extensions

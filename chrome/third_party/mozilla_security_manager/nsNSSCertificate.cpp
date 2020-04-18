/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Netscape security libraries.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2000
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Ian McGreer <mcgreer@netscape.com>
 *   Javier Delgadillo <javi@netscape.com>
 *   Kai Engert <kengert@redhat.com>
 *   Jesper Kristensen <mail@jesperkristensen.dk>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "chrome/third_party/mozilla_security_manager/nsNSSCertificate.h"

#include <pk11func.h>

namespace mozilla_security_manager {

std::string GetCertTitle(CERTCertificate* cert) {
  std::string rv;
  if (cert->nickname) {
    rv = cert->nickname;
  } else {
    char* cn = CERT_GetCommonName(&cert->subject);
    if (cn) {
      rv = cn;
      PORT_Free(cn);
    } else if (cert->subjectName) {
      rv = cert->subjectName;
    } else if (cert->emailAddr) {
      rv = cert->emailAddr;
    }
  }
  // TODO(mattm): Should we return something other than an empty string when all
  // the checks fail?
  return rv;
}

std::string GetCertTokenName(CERTCertificate* cert) {
  std::string token;
  if (cert->slot) {
    token = PK11_GetTokenName(cert->slot);
  }
  return token;
}

}  // namespace mozilla_security_manager

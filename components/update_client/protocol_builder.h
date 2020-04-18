// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_UPDATE_CLIENT_PROTOCOL_BUILDER_H_
#define COMPONENTS_UPDATE_CLIENT_PROTOCOL_BUILDER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "components/update_client/component.h"
#include "components/update_client/crx_downloader.h"
#include "components/update_client/updater_state.h"

namespace update_client {

class Configurator;
class PersistedData;

// Creates the values for the DDOS extra request headers sent with the update
// check. These headers include "X-Goog-Update-Updater",
// "X-Goog-Update-AppId", and  "X-Goog-Update-Interactivity".
std::map<std::string, std::string> BuildUpdateCheckExtraRequestHeaders(
    scoped_refptr<Configurator> config,
    const std::vector<std::string>& ids_checked,
    bool is_foreground);

// Builds an update check request for |components|. |additional_attributes| is
// serialized as part of the <request> element of the request to customize it
// with data that is not platform or component specific. For each |item|, a
// corresponding <app> element is created and inserted as a child node of
// the <request>.
//
// <request protocol="3.0"....>
//    <app appid="hnimpnehoodheedghdeeijklkeaacbdc"
//         version="0.1.2.3" installsource="ondemand">
//      <updatecheck/>
//      <packages>
//        <package fp="abcd"/>
//      </packages>
//    </app>
// </request>
std::string BuildUpdateCheckRequest(
    const Configurator& config,
    const std::string& session_id,
    const std::vector<std::string>& ids_checked,
    const IdToComponentPtrMap& components,
    PersistedData* metadata,
    const std::string& additional_attributes,
    bool enabled_component_updates,
    const std::unique_ptr<UpdaterState::Attributes>& updater_state_attributes);

// Builds a ping request for the specified component. The request contains one
// or more ping events associated with this component. The events are
// serialized as children of the <app> node. For example:
//
// <request protocol="3.0"....>
//    <app appid="hnimpnehoodheedghdeeijklkeaacbdc"
//         version="1.0" nextversion="2.0">
//          <event eventtype="3" eventresult="1"/>
//          <event eventtype="14\" eventresult="0" downloader="direct"
//              "errorcode="-1" url="http://host1/path1" downloaded="123"
//              "total="456"/>
//    </app>
// </request>
std::string BuildEventPingRequest(const Configurator& config,
                                  const Component& component);

// Returns a string representing one ping event for the update of a component.
// The event type for this ping event is 3.
std::string BuildUpdateCompleteEventElement(const Component& component);

// Returns a string representing one ping event for the uninstall of a
// component. The event type for this ping event is 4.
std::string BuildUninstalledEventElement(const Component& component);

// Returns a string representing a download complete event corresponding to
// one download metrics instance. The event type for this ping event is 14.
std::string BuildDownloadCompleteEventElement(
    const Component& component,
    const CrxDownloader::DownloadMetrics& metrics);

std::string BuildActionRunEventElement(bool succeeded,
                                       int error_code,
                                       int extra_code1);

// An update protocol request starts with a common preamble which includes
// version and platform information for Chrome and the operating system,
// followed by a request body, which is the actual payload of the request.
// For example:
//
// <?xml version="1.0" encoding="UTF-8"?>
// <request protocol="3.0" sessionid="{D505492F-95FE-4F90-8253-AEA75772DCC4}"
//        version="chrome-32.0.1.0" prodversion="32.0.1.0"
//        requestid="{7383396D-B4DD-46E1-9104-AAC6B918E792}"
//        updaterchannel="canary" arch="x86" nacl_arch="x86-64"
//        ADDITIONAL ATTRIBUTES>
//   <hw physmemory="16"/>
//   <os platform="win" version="6.1" arch="x86"/>
//   ... REQUEST BODY ...
// </request>

// Builds a protocol request string by creating the outer envelope for
// the request and including the request body specified as a parameter.
// If present, the |download_preference| specifies a group policy that
// affects the list of download URLs returned in the update response.
// If specified, |additional_attributes| are appended as attributes of the
// request element. The additional attributes have to be well-formed for
// insertion in the request element. |updater_state_attributes| is an optional
// parameter specifying that an <updater> element is serialized as part of
// the request.
std::string BuildProtocolRequest(
    const std::string& session_id,
    const std::string& prod_id,
    const std::string& browser_version,
    const std::string& channel,
    const std::string& lang,
    const std::string& os_long_name,
    const std::string& download_preference,
    const std::string& request_body,
    const std::string& additional_attributes,
    const std::unique_ptr<UpdaterState::Attributes>& updater_state_attributes);

}  // namespace update_client

#endif  // COMPONENTS_UPDATE_CLIENT_PROTOCOL_BUILDER_H_

// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/flash_resource.h"

#include <stddef.h>

#include <cmath>

#include "base/containers/mru_cache.h"
#include "base/debug/crash_logging.h"
#include "base/lazy_instance.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/private/ppb_flash.h"
#include "ppapi/c/trusted/ppb_browser_font_trusted.h"
#include "ppapi/proxy/plugin_dispatcher.h"
#include "ppapi/proxy/plugin_globals.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/proxy/serialized_structs.h"
#include "ppapi/shared_impl/ppapi_preferences.h"
#include "ppapi/shared_impl/scoped_pp_var.h"
#include "ppapi/shared_impl/time_conversion.h"
#include "ppapi/shared_impl/var.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppb_url_request_info_api.h"

using ppapi::thunk::EnterResourceNoLock;

namespace ppapi {
namespace proxy {

namespace {

struct LocalTimeZoneOffsetEntry {
  base::TimeTicks expiration;
  double offset;
};

class LocalTimeZoneOffsetCache
    : public base::MRUCache<PP_Time, LocalTimeZoneOffsetEntry> {
 public:
  LocalTimeZoneOffsetCache()
      : base::MRUCache<PP_Time, LocalTimeZoneOffsetEntry>(kCacheSize) {}
 private:
  static const size_t kCacheSize = 100;
};

base::LazyInstance<LocalTimeZoneOffsetCache>::Leaky
    g_local_time_zone_offset_cache = LAZY_INSTANCE_INITIALIZER;

} //  namespace

FlashResource::FlashResource(Connection connection,
                             PP_Instance instance,
                             PluginDispatcher* plugin_dispatcher)
    : PluginResource(connection, instance),
      plugin_dispatcher_(plugin_dispatcher) {
  SendCreate(RENDERER, PpapiHostMsg_Flash_Create());
  SendCreate(BROWSER, PpapiHostMsg_Flash_Create());
}

FlashResource::~FlashResource() {
}

thunk::PPB_Flash_Functions_API* FlashResource::AsPPB_Flash_Functions_API() {
  return this;
}

PP_Var FlashResource::GetProxyForURL(PP_Instance instance,
                                     const std::string& url) {
  std::string proxy;
  int32_t result = SyncCall<PpapiPluginMsg_Flash_GetProxyForURLReply>(RENDERER,
      PpapiHostMsg_Flash_GetProxyForURL(url), &proxy);

  if (result == PP_OK)
    return StringVar::StringToPPVar(proxy);
  return PP_MakeUndefined();
}

void FlashResource::UpdateActivity(PP_Instance instance) {
  Post(BROWSER, PpapiHostMsg_Flash_UpdateActivity());
}

PP_Bool FlashResource::SetCrashData(PP_Instance instance,
                                    PP_FlashCrashKey key,
                                    PP_Var value) {
  StringVar* url_string_var(StringVar::FromPPVar(value));
  if (!url_string_var)
    return PP_FALSE;
  switch (key) {
    case PP_FLASHCRASHKEY_URL: {
      PluginGlobals::Get()->SetActiveURL(url_string_var->value());
      return PP_TRUE;
    }
    case PP_FLASHCRASHKEY_RESOURCE_URL: {
      static base::debug::CrashKeyString* subresource_url =
          base::debug::AllocateCrashKeyString(
              "subresource_url", base::debug::CrashKeySize::Size256);
      base::debug::SetCrashKeyString(subresource_url, url_string_var->value());
      return PP_TRUE;
    }
  }
  return PP_FALSE;
}

double FlashResource::GetLocalTimeZoneOffset(PP_Instance instance,
                                             PP_Time t) {
  LocalTimeZoneOffsetCache& cache = g_local_time_zone_offset_cache.Get();

  // Get the minimum PP_Time value that shares the same minute as |t|.
  // Use cached offset if cache hasn't expired and |t| is in the same minute as
  // the time for the cached offset (assume offsets change on minute
  // boundaries).
  PP_Time t_minute_base = floor(t / 60.0) * 60.0;
  LocalTimeZoneOffsetCache::iterator iter = cache.Get(t_minute_base);
  base::TimeTicks now = base::TimeTicks::Now();
  if (iter != cache.end() && now < iter->second.expiration)
    return iter->second.offset;

  // Cache the local offset for ten seconds, since it's slow on XP and Linux.
  // Note that TimeTicks does not continue counting across sleep/resume on all
  // platforms. This may be acceptable for 10 seconds, but if in the future this
  // is changed to one minute or more, then we should consider using base::Time.
  const int64_t kMaxCachedLocalOffsetAgeInSeconds = 10;
  base::TimeDelta expiration_delta =
      base::TimeDelta::FromSeconds(kMaxCachedLocalOffsetAgeInSeconds);

  LocalTimeZoneOffsetEntry cache_entry;
  cache_entry.expiration = now + expiration_delta;
  cache_entry.offset = 0.0;

  // We can't do the conversion here on Linux because the localtime calls
  // require filesystem access prohibited by the sandbox.
  // TODO(shess): Figure out why OSX needs the access, the sandbox warmup should
  // handle it.  http://crbug.com/149006
#if defined(OS_LINUX) || defined(OS_MACOSX)
  int32_t result = SyncCall<PpapiPluginMsg_Flash_GetLocalTimeZoneOffsetReply>(
      BROWSER,
      PpapiHostMsg_Flash_GetLocalTimeZoneOffset(PPTimeToTime(t)),
      &cache_entry.offset);
  if (result != PP_OK)
    cache_entry.offset = 0.0;
#else
  cache_entry.offset = PPGetLocalTimeZoneOffset(PPTimeToTime(t));
#endif

  cache.Put(t_minute_base, cache_entry);
  return cache_entry.offset;
}

PP_Var FlashResource::GetSetting(PP_Instance instance,
                                 PP_FlashSetting setting) {
  switch (setting) {
    case PP_FLASHSETTING_3DENABLED:
      return PP_MakeBool(PP_FromBool(
          plugin_dispatcher_->preferences().is_3d_supported));
    case PP_FLASHSETTING_INCOGNITO:
      return PP_MakeBool(PP_FromBool(plugin_dispatcher_->incognito()));
    case PP_FLASHSETTING_STAGE3DENABLED:
      return PP_MakeBool(PP_FromBool(
          plugin_dispatcher_->preferences().is_stage3d_supported));
    case PP_FLASHSETTING_STAGE3DBASELINEENABLED:
      return PP_MakeBool(PP_FromBool(
          plugin_dispatcher_->preferences().is_stage3d_baseline_supported));
    case PP_FLASHSETTING_LANGUAGE:
      return StringVar::StringToPPVar(
          PluginGlobals::Get()->GetUILanguage());
    case PP_FLASHSETTING_NUMCORES:
      return PP_MakeInt32(
          plugin_dispatcher_->preferences().number_of_cpu_cores);
    case PP_FLASHSETTING_LSORESTRICTIONS: {
      int32_t restrictions;
      int32_t result =
          SyncCall<PpapiPluginMsg_Flash_GetLocalDataRestrictionsReply>(BROWSER,
              PpapiHostMsg_Flash_GetLocalDataRestrictions(), &restrictions);
      if (result != PP_OK)
        return PP_MakeInt32(PP_FLASHLSORESTRICTIONS_NONE);
      return PP_MakeInt32(restrictions);
    }
  }
  return PP_MakeUndefined();
}

void FlashResource::SetInstanceAlwaysOnTop(PP_Instance instance,
                                           PP_Bool on_top) {
  Post(RENDERER, PpapiHostMsg_Flash_SetInstanceAlwaysOnTop(PP_ToBool(on_top)));
}

PP_Bool FlashResource::DrawGlyphs(
    PP_Instance instance,
    PP_Resource pp_image_data,
    const PP_BrowserFont_Trusted_Description* font_desc,
    uint32_t color,
    const PP_Point* position,
    const PP_Rect* clip,
    const float transformation[3][3],
    PP_Bool allow_subpixel_aa,
    uint32_t glyph_count,
    const uint16_t glyph_indices[],
    const PP_Point glyph_advances[]) {
  EnterResourceNoLock<thunk::PPB_ImageData_API> enter(pp_image_data, true);
  if (enter.failed())
    return PP_FALSE;
  // The instance parameter isn't strictly necessary but we check that it
  // matches anyway.
  if (enter.resource()->pp_instance() != instance)
    return PP_FALSE;

  PPBFlash_DrawGlyphs_Params params;
  params.image_data = enter.resource()->host_resource();
  params.font_desc.SetFromPPBrowserFontDescription(*font_desc);
  params.color = color;
  params.position = *position;
  params.clip = *clip;
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++)
      params.transformation[i][j] = transformation[i][j];
  }
  params.allow_subpixel_aa = allow_subpixel_aa;

  params.glyph_indices.insert(params.glyph_indices.begin(),
                              &glyph_indices[0],
                              &glyph_indices[glyph_count]);
  params.glyph_advances.insert(params.glyph_advances.begin(),
                               &glyph_advances[0],
                               &glyph_advances[glyph_count]);

  // This has to be synchronous because the caller may want to composite on
  // top of the resulting text after the call is complete.
  int32_t result = SyncCall<IPC::Message>(RENDERER,
      PpapiHostMsg_Flash_DrawGlyphs(params));
  return PP_FromBool(result == PP_OK);
}

int32_t FlashResource::Navigate(PP_Instance instance,
                                PP_Resource request_info,
                                const char* target,
                                PP_Bool from_user_action) {
  EnterResourceNoLock<thunk::PPB_URLRequestInfo_API> enter(request_info,
                                                                  true);
  if (enter.failed())
    return PP_ERROR_BADRESOURCE;
  return SyncCall<IPC::Message>(RENDERER, PpapiHostMsg_Flash_Navigate(
      enter.object()->GetData(), target, PP_ToBool(from_user_action)));
}

PP_Bool FlashResource::IsRectTopmost(PP_Instance instance,
                                     const PP_Rect* rect) {
  int32_t result = SyncCall<IPC::Message>(RENDERER,
      PpapiHostMsg_Flash_IsRectTopmost(*rect));
  return PP_FromBool(result == PP_OK);
}

void FlashResource::InvokePrinting(PP_Instance instance) {
  Post(RENDERER, PpapiHostMsg_Flash_InvokePrinting());
}

}  // namespace proxy
}  // namespace ppapi

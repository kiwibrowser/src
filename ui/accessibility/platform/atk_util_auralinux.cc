// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <atk/atk.h>

#include "base/environment.h"
#include "base/memory/singleton.h"
#include "ui/accessibility/platform/atk_util_auralinux.h"
#include "ui/accessibility/platform/ax_platform_node_auralinux.h"

namespace {

const char kAccessibilityEnabled[] = "ACCESSIBILITY_ENABLED";

}  // namespace

G_BEGIN_DECLS

//
// atk_util_auralinux AtkObject definition and implementation.
//

#define ATK_UTIL_AURALINUX_TYPE (atk_util_auralinux_get_type())
#define ATK_UTIL_AURALINUX(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), \
     ATK_UTIL_AURALINUX_TYPE, \
     AtkUtilAuraLinux))
#define ATK_UTIL_AURALINUX_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), \
                             ATK_UTIL_AURALINUX_TYPE, \
                             AtkUtilAuraLinuxClass))
#define IS_ATK_UTIL_AURALINUX(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), ATK_UTIL_AURALINUX_TYPE))
#define IS_ATK_UTIL_AURALINUX_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), ATK_UTIL_AURALINUX_TYPE))
#define ATK_UTIL_AURALINUX_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), \
                               ATK_UTIL_AURALINUX_TYPE, \
                               AtkUtilAuraLinuxClass))

typedef struct _AtkUtilAuraLinux        AtkUtilAuraLinux;
typedef struct _AtkUtilAuraLinuxClass   AtkUtilAuraLinuxClass;

struct _AtkUtilAuraLinux
{
  AtkUtil parent;
};

struct _AtkUtilAuraLinuxClass
{
  AtkUtilClass parent_class;
};

GType atk_util_auralinux_get_type();

G_DEFINE_TYPE(AtkUtilAuraLinux, atk_util_auralinux, ATK_TYPE_UTIL);

static void atk_util_auralinux_init(AtkUtilAuraLinux *ax_util) {
}

static AtkObject* atk_util_auralinux_get_root() {
  ui::AXPlatformNode* application = ui::AXPlatformNodeAuraLinux::application();
  if (application) {
    return application->GetNativeViewAccessible();
  }
  return nullptr;
}

static G_CONST_RETURN gchar* atk_util_auralinux_get_toolkit_name(void) {
  return "Chromium";
}

static G_CONST_RETURN gchar* atk_util_auralinux_get_toolkit_version(void) {
  return "1.0";
}

static void atk_util_auralinux_class_init(AtkUtilAuraLinuxClass *klass) {
  AtkUtilClass *atk_class;
  gpointer data;

  data = g_type_class_peek(ATK_TYPE_UTIL);
  atk_class = ATK_UTIL_CLASS(data);

  atk_class->get_root = atk_util_auralinux_get_root;
  atk_class->get_toolkit_name = atk_util_auralinux_get_toolkit_name;
  atk_class->get_toolkit_version = atk_util_auralinux_get_toolkit_version;
}

G_END_DECLS

//
// AtkUtilAuraLinuxClass implementation.
//

namespace ui {

// static
AtkUtilAuraLinux* AtkUtilAuraLinux::GetInstance() {
  return base::Singleton<AtkUtilAuraLinux>::get();
}

bool AtkUtilAuraLinux::ShouldEnableAccessibility() {
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  std::string enable_accessibility;
  env->GetVar(kAccessibilityEnabled, &enable_accessibility);
  if (enable_accessibility == "1" || PlatformShouldEnableAccessibility())
    return true;
  return false;
}

void AtkUtilAuraLinux::InitializeAsync() {
  static bool initialized = false;

  if (initialized || !ShouldEnableAccessibility())
    return;

  initialized = true;

  // Register our util class.
  g_type_class_unref(g_type_class_ref(ATK_UTIL_AURALINUX_TYPE));

  PlatformInitializeAsync();
}

void AtkUtilAuraLinux::InitializeForTesting() {
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  env->SetVar(kAccessibilityEnabled, "1");

  InitializeAsync();
}

}  // namespace ui

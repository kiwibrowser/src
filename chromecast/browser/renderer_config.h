// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_BROWSER_RENDERER_CONFIG_H_
#define CHROMECAST_BROWSER_RENDERER_CONFIG_H_

#include <string>

#include "base/callback.h"
#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "base/synchronization/lock.h"

namespace base {
class CommandLine;
}  // namespace base

namespace chromecast {
namespace shell {

// Application-specific configuration for the renderer.
// All methods are thread-safe.
class RendererConfig : public base::RefCountedThreadSafe<RendererConfig> {
 public:
  REQUIRE_ADOPTION_FOR_REFCOUNTED_TYPE();

  RendererConfig();

  // Returns the render process ID.
  virtual int GetRenderProcessId() const = 0;

  // Appends all switches to |command line|.
  virtual void AppendSwitchesTo(base::CommandLine* command_line) const = 0;

 protected:
  virtual ~RendererConfig();

 private:
  friend class base::RefCountedThreadSafe<RendererConfig>;

  DISALLOW_COPY_AND_ASSIGN(RendererConfig);
};

// Used to configure the renderer for individual applications.
// All methods must be called on the main thread.
class RendererConfigurator {
 public:
  using AddRendererConfigCallback =
      base::OnceCallback<void(int render_process_id,
                              scoped_refptr<RendererConfig> renderer_config)>;
  using RemoveRendererConfigCallback =
      base::OnceCallback<void(int render_process_id)>;

  RendererConfigurator(
      AddRendererConfigCallback add_renderer_config_callback,
      RemoveRendererConfigCallback remove_renderer_config_callback);
  RendererConfigurator(RendererConfigurator&& other);
  virtual ~RendererConfigurator();

  // Configures the renderer with |render_process_id|. Must only be called once,
  // and no other methods may be called afterward. Must be called before the
  // render process is started.
  void Configure(int render_process_id);

  // Appends a switch, with an optional value, to the command line.
  void AppendSwitch(const std::string& switch_string);
  void AppendSwitchASCII(const std::string& switch_string,
                         const std::string& value);

 private:
  AddRendererConfigCallback add_renderer_config_callback_;
  RemoveRendererConfigCallback remove_renderer_config_callback_;
  int render_process_id_;
  base::flat_set<std::string> switches_;
  base::flat_map<std::string /* switch */, std::string /* value */>
      ascii_switches_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(RendererConfigurator);
};

class RendererConfigManager {
 public:
  RendererConfigManager();
  ~RendererConfigManager();

  // Returns a new renderer configurator.
  // Must be called on the main thread.
  RendererConfigurator CreateRendererConfigurator();

  // Returns the config for the renderer with |render_process_id|.
  // May be called on any thread.
  scoped_refptr<const RendererConfig> GetRendererConfig(int render_process_id);

 private:
  void AddRendererConfig(int render_process_id,
                         scoped_refptr<RendererConfig> renderer_config);
  void RemoveRendererConfig(int render_process_id);

  base::flat_map<int /* render_process_id */, scoped_refptr<RendererConfig>>
      renderer_configs_;
  base::Lock renderer_configs_lock_;

  SEQUENCE_CHECKER(sequence_checker_);

  base::WeakPtrFactory<RendererConfigManager> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(RendererConfigManager);
};

}  // namespace shell
}  // namespace chromecast

#endif  // CHROMECAST_BROWSER_RENDERER_CONFIG_H_

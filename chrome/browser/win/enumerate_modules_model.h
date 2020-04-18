// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_WIN_ENUMERATE_MODULES_MODEL_H_
#define CHROME_BROWSER_WIN_ENUMERATE_MODULES_MODEL_H_

#include <memory>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "base/strings/string16.h"
#include "base/timer/timer.h"
#include "chrome/browser/conflicts/module_info_util_win.h"
#include "content/public/browser/browser_thread.h"
#include "url/gurl.h"

class EnumerateModulesModel;

namespace base {
class ListValue;
}

// A helper class that implements the enumerate module functionality on the FILE
// thread. Not to be used directly.
// TODO(chrisha): Move this to a separate .h and .cc.
class ModuleEnumerator {
 public:
  // What type of module we are dealing with. Loaded modules are modules we
  // detect as loaded in the process at the time of scanning. The others are
  // modules of interest and may or may not be loaded in the process at the
  // time of scan.
  enum ModuleType {
    LOADED_MODULE               = 1 << 0,
    SHELL_EXTENSION             = 1 << 1,
    WINSOCK_MODULE_REGISTRATION = 1 << 2,
  };

  // The blacklist status of the module. Suspected Bad modules have been
  // partially matched (ie. name matches and location, but not description)
  // whereas Confirmed Bad modules have been identified further (ie.
  // AuthentiCode signer matches).
  enum ModuleStatus {
    // This is returned by the matching function when comparing against the
    // blacklist and the module does not match the current entry in the
    // blacklist.
    NOT_MATCHED,
    // The module is not on the blacklist. Assume it is good.
    GOOD,
    // Module is a suspected bad module.
    SUSPECTED_BAD,
    // Module is a bad bad dog.
    CONFIRMED_BAD,
  };

  // A bitmask with the possible resolutions for bad modules.
  enum RecommendedAction {
    NONE          = 0,
    INVESTIGATING = 1 << 0,
    UNINSTALL     = 1 << 1,
    DISABLE       = 1 << 2,
    UPDATE        = 1 << 3,
    SEE_LINK      = 1 << 4,
    NOTIFY_USER   = 1 << 5,
  };

  // The structure we populate when enumerating modules.
  struct Module {
    Module();
    Module(const Module& rhs);
    // Constructor exposed for unittesting.
    Module(ModuleType type,
           ModuleStatus status,
           const base::string16& location,
           const base::string16& name,
           const base::string16& product_name,
           const base::string16& description,
           const base::string16& version,
           RecommendedAction recommended_action);
    ~Module();

    // The type of module found
    ModuleType type;
    // The module status (benign/bad/etc).
    ModuleStatus status;
    // The module path, not including filename.
    base::string16 location;
    // The name of the module (filename).
    base::string16 name;
    // The name of the product the module belongs to.
    base::string16 product_name;
    // The module file description.
    base::string16 description;
    // The module version.
    base::string16 version;
    // The help tips bitmask.
    RecommendedAction recommended_action;
    // The duplicate count within each category of modules.
    int duplicate_count;
    // The certificate info for the module.
    CertificateInfo cert_info;
  };

  // A vector typedef of all modules enumerated.
  typedef std::vector<Module> ModulesVector;

  // A static function that normalizes the module information in the |module|
  // struct. Module information needs to be normalized before comparing against
  // the blacklist. This is because the same module can be described in many
  // different ways, ie. file paths can be presented in long/short name form,
  // and are not case sensitive on Windows. Also, the version string returned
  // can include appended text, which we don't want to use during comparison
  // against the blacklist.
  static void NormalizeModule(Module* module);

  // Constructs a ModuleEnumerator that will notify the provided |observer| once
  // enumeration is complete. |observer| must outlive the ModuleEnumerator.
  explicit ModuleEnumerator(EnumerateModulesModel* observer);

  ~ModuleEnumerator();

  // Start scanning the loaded module list (if a scan is not already in
  // progress). This function does not block while reading the module list and
  // will notify when done by calling the DoneScanning method of |observer_|.
  void ScanNow(ModulesVector* list);

  // Sets |per_module_delay_| to zero, causing the modules to be inspected
  // in realtime.
  void SetPerModuleDelayToZero();

 private:
  FRIEND_TEST_ALL_PREFIXES(EnumerateModulesTest, CollapsePath);

  // This function posts a task to enumerate all modules asynchronously. Once
  // the list of module filenames is populated, a delayed task is posted to scan
  // the first module.
  void ScanImplStart();

  // Inspects the module in |enumerated_modules_| at the given |index|. Gets
  // module information, normalizes it, and collapses the path. This is an
  // expensive operation and non-critical. Posts a delayed task to ScanImplDelay
  // for the next module. When all modules are finished forwards directly to
  // ScanImplFinish.
  void ScanImplModule(size_t index);

  // Collects metrics and notifies the observer that the enumeration is complete
  // by invoking DoneScanning on the UI thread.
  void ScanImplFinish();

  // Enumerate all modules loaded into the Chrome process. Creates empty
  // entries in |enumerated_modules_| with a populated |location| field.
  void EnumerateLoadedModules();

  // Enumerate all registered Windows shell extensions. Creates empty
  // entries in |enumerated_modules_| with a populated |location| field.
  void EnumerateShellExtensions();

  // Callback for ShellExtensionEnumerator::EnumerateShellExtensionPaths().
  void OnShellExtensionEnumerated(const base::FilePath& shell_extension);

  // Enumerate all registered Winsock LSP modules. Creates empty
  // entries in |enumerated_modules_| with a populated |location| field.
  void EnumerateWinsockModules();

  // Given a |module|, initializes the structure and loads additional
  // information using the location field of the module.
  void PopulateModuleInformation(Module* module);

  // Checks the module list to see if a |module| of the same type, location
  // and name has been added before and if so, increments its duplication
  // counter. If it doesn't appear in the list, it is added.
  void AddToListWithoutDuplicating(const Module&);

  // Builds up a vector of path values mapping to environment variable,
  // with pairs like [c:\windows\, %systemroot%]. This is later used to
  // collapse paths like c:\windows\system32 into %systemroot%\system32, which
  // we can use for comparison against our blacklist (which uses only env vars).
  // NOTE: The vector will not contain an exhaustive list of environment
  // variables, only the ones currently found on the blacklist or ones that are
  // likely to appear there.
  void PreparePathMappings();

  // Reports (via UMA) a handful of high-level metrics regarding third party
  // modules in this process. Called by ScanImplFinish.
  void ReportThirdPartyMetrics();

  // The TaskRunner to perform work in the background.
  const scoped_refptr<base::TaskRunner> background_task_runner_;

  // The vector of paths to %env_var%, used to account for differences in
  // where people keep there files, c:\windows vs. d:\windows, etc.
  StringMapping path_mapping_;

  // The vector containing all the enumerated modules (loaded and modules of
  // interest).
  ModulesVector* enumerated_modules_;

  // The observer, which needs to be notified when the scan is complete.
  EnumerateModulesModel* observer_;

  // The delay that is observed between module inspection tasks. This is
  // currently 1 second, which means it takes several minutes to iterate over
  // all modules on average.
  base::TimeDelta per_module_delay_;

  // The amount of time taken for on-disk module inspection. Reported in
  // ScanImplFinish.
  base::TimeDelta enumeration_inspection_time_;

  // The total amount of time taken for module enumeration. Reported in
  // ScanImplFinish.
  base::TimeDelta enumeration_total_time_;

  DISALLOW_COPY_AND_ASSIGN(ModuleEnumerator);
};

// This is a singleton class that enumerates all modules loaded into Chrome,
// both currently loaded modules (called DLLs on Windows) and modules 'of
// interest', such as WinSock LSP modules. This class also marks each module
// as benign or suspected bad or outright bad, using a supplied blacklist that
// is currently hard-coded.
//
// To use this class, grab the singleton pointer and call ScanNow().
// Then wait to get notified through MODULE_LIST_ENUMERATED when the list is
// ready.
//
// The member functions of this class may only be used from the UI thread. The
// bulk of the work is actually performed asynchronously in TaskScheduler with
// CONTINUE_ON_SHUTDOWN semantics, as the WinCrypt functions can effectively
// block arbitrarily during shutdown.
//
// TODO(chrisha): If this logic is ever extended to other platforms, then make
// this file generic for all platforms, and remove the precompiler logic in
// app_menu_icon_controller.*.
class EnumerateModulesModel {
 public:
  // UMA histogram constants.
  enum UmaModuleConflictHistogramOptions {
    ACTION_BUBBLE_SHOWN = 0,
    ACTION_BUBBLE_LEARN_MORE,
    ACTION_MENU_LEARN_MORE,
    ACTION_BOUNDARY,  // Must be the last value.
  };

  // Observer class used to determine when a scan has completed and when any
  // associated UI elements have been dismissed.
  class Observer {
   public:
    // Invoked when EnumerateModulesModel has completed a scan of modules.
    virtual void OnScanCompleted() {}

    // Invoked when a user has acknowledged incompatible modules found in a
    // module scan.
    virtual void OnConflictsAcknowledged() {}

   protected:
    virtual ~Observer() = default;
  };

  // Returns the singleton instance of this class.
  static EnumerateModulesModel* GetInstance();

  // Adds an |observer| to the enumerator. Callbacks will occur on the UI
  // thread.
  void AddObserver(Observer* observer);

  // Removes an |observer| from the enumerator.
  void RemoveObserver(Observer* observer);

  // Returns true if we should show the conflict notification. The conflict
  // notification is only shown once during the lifetime of the process.
  bool ShouldShowConflictWarning() const;

  // Called when the user has acknowledged the conflict notification.
  void AcknowledgeConflictNotification();

  // Returns the number of suspected bad modules found in the last scan.
  // Returns 0 if no scan has taken place yet.
  int suspected_bad_modules_detected() const;

  // Returns the number of confirmed bad modules found in the last scan.
  // Returns 0 if no scan has taken place yet.
  int confirmed_bad_modules_detected() const;

  // Returns how many modules to notify the user about.
  int modules_to_notify_about() const;

  // Checks to see if a scanning task should be started and sets one off, if so.
  // This will cause ScanNow to be invoked in background mode.
  void MaybePostScanningTask();

  // Asynchronously start the scan for the loaded module list. If
  // |background_mode| is true the scan will happen slowly over a process of
  // minutes, spread across dozens or even hundreds of delayed tasks. Otherwise
  // the processing will occur in a single task.
  void ScanNow(bool background_mode);

  // Gets the whole module list as a ListValue.
  std::unique_ptr<base::ListValue> GetModuleList();

 private:
  friend class ModuleEnumerator;

  // Private to enforce singleton nature of this class.
  EnumerateModulesModel();
  ~EnumerateModulesModel();

  // Called on the UI thread when the helper class is done scanning. The
  // ModuleEnumerator that calls this must not do any work after causing this
  // function to be called, as the EnumerateModulesModel may delete the
  // ModuleEnumerator.
  void DoneScanning();

  // The vector containing all the modules enumerated. Will be normalized and
  // any bad modules will be marked. Written to from the background TaskRunner
  // by the |module_enumerator_|, read from on the UI thread by this class.
  ModuleEnumerator::ModulesVector enumerated_modules_;

  // The object responsible for enumerating the modules on a background
  // TaskRunner. Only accessed from the UI thread.
  std::unique_ptr<ModuleEnumerator> module_enumerator_;

  // Whether the conflict notification has been acknowledged by the user. Only
  // modified on the UI thread.
  bool conflict_notification_acknowledged_;

  // The number of confirmed bad modules (not including suspected bad ones)
  // found during last scan. Only modified on the UI thread.
  int confirmed_bad_modules_detected_;

  // The number of bad modules the user needs to be aggressively notified about.
  // Only modified on the UI thread.
  int modules_to_notify_about_;

  // The number of suspected bad modules (not including confirmed bad ones)
  // found during last scan. Only modified on the UI thread.
  int suspected_bad_modules_detected_;

  base::ObserverList<Observer> observers_;

  DISALLOW_COPY_AND_ASSIGN(EnumerateModulesModel);
};

#endif  // CHROME_BROWSER_WIN_ENUMERATE_MODULES_MODEL_H_

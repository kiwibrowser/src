#ifndef CHROME_BROWSER_EXTENSIONS_API_MISES_PRIVATE_MISES_PRIVATE_API_H_
#define CHROME_BROWSER_EXTENSIONS_API_MISES_PRIVATE_MISES_PRIVATE_API_H_
 
#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/extensions/chrome_extension_function.h"
#include "chrome/common/extensions/api/mises_private.h"
#include "extensions/browser/extension_event_histogram_value.h"
#include "extensions/browser/extension_function.h"

namespace extensions {
	
class MisesPrivateSetMisesIdFunction : public ChromeAsyncExtensionFunction
{
public:
  DECLARE_EXTENSION_FUNCTION("misesPrivate.setMisesId",
                             MISESPRIVATE_SETMISESID)

protected:
  ~MisesPrivateSetMisesIdFunction() override;

  bool RunAsync() override;
};

class MisesPrivateGetInstallReferrerFunction : public UIThreadExtensionFunction
{
public:
  ExtensionFunction::ResponseAction Run() override;
  DECLARE_EXTENSION_FUNCTION("misesPrivate.getInstallReferrer",
                             MISESPRIVATE_GETINSTALLREFERRER)

protected:
  ~MisesPrivateGetInstallReferrerFunction() override;

};
 
}  // namespace extensions
 
#endif  // CHROME_BROWSER_EXTENSIONS_API_MISES_PRIVATE_MISES_PRIVATE_API_H_

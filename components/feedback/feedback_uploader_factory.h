// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_FEEDBACK_FEEDBACK_UPLOADER_FACTORY_H_
#define COMPONENTS_FEEDBACK_FEEDBACK_UPLOADER_FACTORY_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
class SingleThreadTaskRunner;
}  // namespace base

namespace content {
class BrowserContext;
}

namespace feedback {

class FeedbackUploader;

// Singleton that owns the FeedbackUploaders and associates them with profiles;
class FeedbackUploaderFactory : public BrowserContextKeyedServiceFactory {
 public:
  // Returns singleton instance of FeedbackUploaderFactory.
  static FeedbackUploaderFactory* GetInstance();

  // Returns the Feedback Uploader associated with |context|.
  static FeedbackUploader* GetForBrowserContext(
      content::BrowserContext* context);

  // Creates a new SingleThreadTaskRunner that is used to run feedback blocking
  // background work. Tests can use this to create the exact same type of runner
  // that's actually used in production code to simulate the same behavior.
  static scoped_refptr<base::SingleThreadTaskRunner> CreateUploaderTaskRunner();

 protected:
  FeedbackUploaderFactory(const char* service_name);
  ~FeedbackUploaderFactory() override;

  // The task runner used to handle all blocking background feedback-reports
  // work. It involves reading / writing reports from / to disk. Those
  // operations must not interleave and thread affinity is required.
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

 private:
  friend struct base::DefaultSingletonTraits<FeedbackUploaderFactory>;

  FeedbackUploaderFactory();

  // BrowserContextKeyedServiceFactory overrides:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(FeedbackUploaderFactory);
};

}  // namespace feedback

#endif  // COMPONENTS_FEEDBACK_FEEDBACK_UPLOADER_FACTORY_H_

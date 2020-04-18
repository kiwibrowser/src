// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/event_listener_map.h"

#include <utility>

#include "base/bind.h"
#include "base/values.h"
#include "content/public/test/mock_render_process_host.h"
#include "content/public/test/test_browser_context.h"
#include "extensions/browser/event_router.h"
#include "extensions/browser/extensions_test.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using base::DictionaryValue;
using base::ListValue;
using base::Value;

namespace extensions {

namespace {

const char kExt1Id[] = "extension_1";
const char kExt2Id[] = "extension_2";
const char kEvent1Name[] = "event1";
const char kEvent2Name[] = "event2";
const char kURL[] = "https://google.com/some/url";

using EventListenerConstructor = base::Callback<std::unique_ptr<EventListener>(
    const std::string& /* event_name */,
    content::RenderProcessHost* /* process */,
    std::unique_ptr<base::DictionaryValue> /* filter */)>;

class EmptyDelegate : public EventListenerMap::Delegate {
  void OnListenerAdded(const EventListener* listener) override {}
  void OnListenerRemoved(const EventListener* listener) override {}
};

class EventListenerMapTest : public ExtensionsTest {
 public:
  EventListenerMapTest()
      : delegate_(std::make_unique<EmptyDelegate>()),
        listeners_(std::make_unique<EventListenerMap>(delegate_.get())) {}

  // testing::Test overrides:
  void SetUp() override {
    ExtensionsTest::SetUp();

    // |process_| will be destroyed by |browser_context()| when it goes away.
    process_ = new content::MockRenderProcessHost(browser_context());
  }

  std::unique_ptr<DictionaryValue> CreateHostSuffixFilter(
      const std::string& suffix) {
    auto filter_dict = std::make_unique<DictionaryValue>();
    filter_dict->Set("hostSuffix", std::make_unique<Value>(suffix));

    auto filter_list = std::make_unique<ListValue>();
    filter_list->Append(std::move(filter_dict));

    auto filter = std::make_unique<DictionaryValue>();
    filter->Set("url", std::move(filter_list));
    return filter;
  }

  std::unique_ptr<Event> CreateNamedEvent(const std::string& event_name) {
    return CreateEvent(event_name, GURL());
  }

  std::unique_ptr<Event> CreateEvent(const std::string& event_name,
                                     const GURL& url) {
    EventFilteringInfo info;
    info.url = url;
    return std::make_unique<Event>(
        events::FOR_TEST, event_name, std::make_unique<ListValue>(), nullptr,
        GURL(), EventRouter::USER_GESTURE_UNKNOWN, info);
  }

 protected:
  void TestUnfilteredEventsGoToAllListeners(
      const EventListenerConstructor& constructor);
  void TestRemovingByProcess(const EventListenerConstructor& constructor);
  void TestRemovingByListener(const EventListenerConstructor& constructor);
  void TestAddExistingUnfilteredListener(
      const EventListenerConstructor& constructor);
  void TestHasListenerForEvent(const EventListenerConstructor& constructor);

  std::unique_ptr<EventListenerMap::Delegate> delegate_;
  std::unique_ptr<EventListenerMap> listeners_;
  content::RenderProcessHost* process_;
};

std::unique_ptr<EventListener> CreateEventListenerForExtension(
    const std::string& extension_id,
    const std::string& event_name,
    content::RenderProcessHost* process,
    std::unique_ptr<base::DictionaryValue> filter) {
  return EventListener::ForExtension(event_name, extension_id, process,
                                     std::move(filter));
}

std::unique_ptr<EventListener> CreateEventListenerForURL(
    const GURL& listener_url,
    const std::string& event_name,
    content::RenderProcessHost* process,
    std::unique_ptr<base::DictionaryValue> filter) {
  return EventListener::ForURL(event_name, listener_url, process,
                               std::move(filter));
}

void EventListenerMapTest::TestUnfilteredEventsGoToAllListeners(
    const EventListenerConstructor& constructor) {
  listeners_->AddListener(constructor.Run(
      kEvent1Name, nullptr, std::make_unique<base::DictionaryValue>()));
  std::unique_ptr<Event> event(CreateNamedEvent(kEvent1Name));
  ASSERT_EQ(1u, listeners_->GetEventListeners(*event).size());
}

TEST_F(EventListenerMapTest, UnfilteredEventsGoToAllListenersForExtensions) {
  TestUnfilteredEventsGoToAllListeners(
      base::Bind(&CreateEventListenerForExtension, kExt1Id));
}

TEST_F(EventListenerMapTest, UnfilteredEventsGoToAllListenersForURLs) {
  TestUnfilteredEventsGoToAllListeners(
      base::Bind(&CreateEventListenerForURL, GURL(kURL)));
}

TEST_F(EventListenerMapTest, FilteredEventsGoToAllMatchingListeners) {
  listeners_->AddListener(EventListener::ForExtension(
      kEvent1Name, kExt1Id, nullptr, CreateHostSuffixFilter("google.com")));
  listeners_->AddListener(EventListener::ForExtension(
      kEvent1Name, kExt1Id, nullptr, std::make_unique<DictionaryValue>()));

  std::unique_ptr<Event> event(CreateNamedEvent(kEvent1Name));
  event->filter_info.url = GURL("http://www.google.com");
  std::set<const EventListener*> targets(listeners_->GetEventListeners(*event));
  ASSERT_EQ(2u, targets.size());
}

TEST_F(EventListenerMapTest, FilteredEventsOnlyGoToMatchingListeners) {
  listeners_->AddListener(EventListener::ForExtension(
      kEvent1Name, kExt1Id, nullptr, CreateHostSuffixFilter("google.com")));
  listeners_->AddListener(EventListener::ForExtension(
      kEvent1Name, kExt1Id, nullptr, CreateHostSuffixFilter("yahoo.com")));

  std::unique_ptr<Event> event(CreateNamedEvent(kEvent1Name));
  event->filter_info.url = GURL("http://www.google.com");
  std::set<const EventListener*> targets(listeners_->GetEventListeners(*event));
  ASSERT_EQ(1u, targets.size());
}

TEST_F(EventListenerMapTest, LazyAndUnlazyListenersGetReturned) {
  listeners_->AddListener(EventListener::ForExtension(
      kEvent1Name, kExt1Id, nullptr, CreateHostSuffixFilter("google.com")));

  listeners_->AddListener(EventListener::ForExtension(
      kEvent1Name, kExt1Id, process_, CreateHostSuffixFilter("google.com")));

  std::unique_ptr<Event> event(CreateNamedEvent(kEvent1Name));
  event->filter_info.url = GURL("http://www.google.com");
  std::set<const EventListener*> targets(listeners_->GetEventListeners(*event));
  ASSERT_EQ(2u, targets.size());
}

void EventListenerMapTest::TestRemovingByProcess(
    const EventListenerConstructor& constructor) {
  listeners_->AddListener(constructor.Run(
      kEvent1Name, nullptr, CreateHostSuffixFilter("google.com")));

  listeners_->AddListener(constructor.Run(
      kEvent1Name, process_, CreateHostSuffixFilter("google.com")));

  listeners_->RemoveListenersForProcess(process_);

  std::unique_ptr<Event> event(CreateNamedEvent(kEvent1Name));
  event->filter_info.url = GURL("http://www.google.com");
  ASSERT_EQ(1u, listeners_->GetEventListeners(*event).size());
}

TEST_F(EventListenerMapTest, TestRemovingByProcessForExtension) {
  TestRemovingByProcess(base::Bind(&CreateEventListenerForExtension, kExt1Id));
}

TEST_F(EventListenerMapTest, TestRemovingByProcessForURL) {
  TestRemovingByProcess(base::Bind(&CreateEventListenerForURL, GURL(kURL)));
}

void EventListenerMapTest::TestRemovingByListener(
    const EventListenerConstructor& constructor) {
  listeners_->AddListener(constructor.Run(
      kEvent1Name, nullptr, CreateHostSuffixFilter("google.com")));

  listeners_->AddListener(constructor.Run(
      kEvent1Name, process_, CreateHostSuffixFilter("google.com")));

  std::unique_ptr<EventListener> listener(constructor.Run(
      kEvent1Name, process_, CreateHostSuffixFilter("google.com")));
  listeners_->RemoveListener(listener.get());

  std::unique_ptr<Event> event(CreateNamedEvent(kEvent1Name));
  event->filter_info.url = GURL("http://www.google.com");
  ASSERT_EQ(1u, listeners_->GetEventListeners(*event).size());
}

TEST_F(EventListenerMapTest, TestRemovingByListenerForExtension) {
  TestRemovingByListener(base::Bind(&CreateEventListenerForExtension, kExt1Id));
}

TEST_F(EventListenerMapTest, TestRemovingByListenerForURL) {
  TestRemovingByListener(base::Bind(&CreateEventListenerForURL, GURL(kURL)));
}

TEST_F(EventListenerMapTest, TestLazyDoubleAddIsUndoneByRemove) {
  listeners_->AddListener(EventListener::ForExtension(
      kEvent1Name, kExt1Id, nullptr, CreateHostSuffixFilter("google.com")));
  listeners_->AddListener(EventListener::ForExtension(
      kEvent1Name, kExt1Id, nullptr, CreateHostSuffixFilter("google.com")));

  std::unique_ptr<EventListener> listener(EventListener::ForExtension(
      kEvent1Name, kExt1Id, nullptr, CreateHostSuffixFilter("google.com")));
  listeners_->RemoveListener(listener.get());

  std::unique_ptr<Event> event(CreateNamedEvent(kEvent1Name));
  event->filter_info.url = GURL("http://www.google.com");
  std::set<const EventListener*> targets(listeners_->GetEventListeners(*event));
  ASSERT_EQ(0u, targets.size());
}

TEST_F(EventListenerMapTest, HostSuffixFilterEquality) {
  std::unique_ptr<DictionaryValue> filter1(
      CreateHostSuffixFilter("google.com"));
  std::unique_ptr<DictionaryValue> filter2(
      CreateHostSuffixFilter("google.com"));
  ASSERT_TRUE(filter1->Equals(filter2.get()));
}

TEST_F(EventListenerMapTest, RemoveListenersForExtension) {
  listeners_->AddListener(EventListener::ForExtension(
      kEvent1Name, kExt1Id, nullptr, CreateHostSuffixFilter("google.com")));
  listeners_->AddListener(EventListener::ForExtension(
      kEvent2Name, kExt1Id, nullptr, CreateHostSuffixFilter("google.com")));

  listeners_->RemoveListenersForExtension(kExt1Id);

  std::unique_ptr<Event> event1(CreateNamedEvent(kEvent1Name));
  event1->filter_info.url = GURL("http://www.google.com");
  std::set<const EventListener*> targets(
      listeners_->GetEventListeners(*event1));
  ASSERT_EQ(0u, targets.size());

  std::unique_ptr<Event> event2(CreateNamedEvent(kEvent2Name));
  event2->filter_info.url = GURL("http://www.google.com");
  targets = listeners_->GetEventListeners(*event2);
  ASSERT_EQ(0u, targets.size());
}

TEST_F(EventListenerMapTest, AddExistingFilteredListener) {
  bool first_new = listeners_->AddListener(EventListener::ForExtension(
      kEvent1Name, kExt1Id, nullptr, CreateHostSuffixFilter("google.com")));
  bool second_new = listeners_->AddListener(EventListener::ForExtension(
      kEvent1Name, kExt1Id, nullptr, CreateHostSuffixFilter("google.com")));

  ASSERT_TRUE(first_new);
  ASSERT_FALSE(second_new);
}

void EventListenerMapTest::TestAddExistingUnfilteredListener(
    const EventListenerConstructor& constructor) {
  bool first_add = listeners_->AddListener(constructor.Run(
      kEvent1Name, nullptr, std::make_unique<base::DictionaryValue>()));
  bool second_add = listeners_->AddListener(constructor.Run(
      kEvent1Name, nullptr, std::make_unique<base::DictionaryValue>()));

  std::unique_ptr<EventListener> listener(constructor.Run(
      kEvent1Name, nullptr, std::make_unique<base::DictionaryValue>()));
  bool first_remove = listeners_->RemoveListener(listener.get());
  bool second_remove = listeners_->RemoveListener(listener.get());

  ASSERT_TRUE(first_add);
  ASSERT_FALSE(second_add);
  ASSERT_TRUE(first_remove);
  ASSERT_FALSE(second_remove);
}

TEST_F(EventListenerMapTest, AddExistingUnfilteredListenerForExtensions) {
  TestAddExistingUnfilteredListener(
      base::Bind(&CreateEventListenerForExtension, kExt1Id));
}

TEST_F(EventListenerMapTest, AddExistingUnfilteredListenerForURLs) {
  TestAddExistingUnfilteredListener(
      base::Bind(&CreateEventListenerForURL, GURL(kURL)));
}

TEST_F(EventListenerMapTest, RemovingRouters) {
  listeners_->AddListener(EventListener::ForExtension(
      kEvent1Name, kExt1Id, process_, std::unique_ptr<DictionaryValue>()));
  listeners_->AddListener(EventListener::ForURL(
      kEvent1Name, GURL(kURL), process_, std::unique_ptr<DictionaryValue>()));
  listeners_->RemoveListenersForProcess(process_);
  ASSERT_FALSE(listeners_->HasListenerForEvent(kEvent1Name));
}

void EventListenerMapTest::TestHasListenerForEvent(
    const EventListenerConstructor& constructor) {
  ASSERT_FALSE(listeners_->HasListenerForEvent(kEvent1Name));

  listeners_->AddListener(constructor.Run(
      kEvent1Name, process_, std::make_unique<base::DictionaryValue>()));

  ASSERT_FALSE(listeners_->HasListenerForEvent(kEvent2Name));
  ASSERT_TRUE(listeners_->HasListenerForEvent(kEvent1Name));
  listeners_->RemoveListenersForProcess(process_);
  ASSERT_FALSE(listeners_->HasListenerForEvent(kEvent1Name));
}

TEST_F(EventListenerMapTest, HasListenerForEventForExtension) {
  TestHasListenerForEvent(
      base::Bind(&CreateEventListenerForExtension, kExt1Id));
}

TEST_F(EventListenerMapTest, HasListenerForEventForURL) {
  TestHasListenerForEvent(base::Bind(&CreateEventListenerForURL, GURL(kURL)));
}

TEST_F(EventListenerMapTest, HasListenerForExtension) {
  ASSERT_FALSE(listeners_->HasListenerForExtension(kExt1Id, kEvent1Name));

  // Non-lazy listener.
  listeners_->AddListener(EventListener::ForExtension(
      kEvent1Name, kExt1Id, process_, std::unique_ptr<DictionaryValue>()));
  // Lazy listener.
  listeners_->AddListener(EventListener::ForExtension(
      kEvent1Name, kExt1Id, nullptr, std::unique_ptr<DictionaryValue>()));

  ASSERT_FALSE(listeners_->HasListenerForExtension(kExt1Id, kEvent2Name));
  ASSERT_TRUE(listeners_->HasListenerForExtension(kExt1Id, kEvent1Name));
  ASSERT_FALSE(listeners_->HasListenerForExtension(kExt2Id, kEvent1Name));
  listeners_->RemoveListenersForProcess(process_);
  ASSERT_TRUE(listeners_->HasListenerForExtension(kExt1Id, kEvent1Name));
  listeners_->RemoveListenersForExtension(kExt1Id);
  ASSERT_FALSE(listeners_->HasListenerForExtension(kExt1Id, kEvent1Name));
}

TEST_F(EventListenerMapTest, AddLazyListenersFromPreferences) {
  auto filter_list = std::make_unique<ListValue>();
  filter_list->Append(CreateHostSuffixFilter("google.com"));
  filter_list->Append(CreateHostSuffixFilter("yahoo.com"));

  DictionaryValue filtered_listeners;
  filtered_listeners.Set(kEvent1Name, std::move(filter_list));
  listeners_->LoadFilteredLazyListeners(kExt1Id, false, filtered_listeners);

  std::unique_ptr<Event> event(
      CreateEvent(kEvent1Name, GURL("http://www.google.com")));
  std::set<const EventListener*> targets(listeners_->GetEventListeners(*event));
  ASSERT_EQ(1u, targets.size());
  std::unique_ptr<EventListener> listener(EventListener::ForExtension(
      kEvent1Name, kExt1Id, nullptr, CreateHostSuffixFilter("google.com")));
  ASSERT_TRUE((*targets.begin())->Equals(listener.get()));
}

TEST_F(EventListenerMapTest, CorruptedExtensionPrefsShouldntCrash) {
  DictionaryValue filtered_listeners;
  // kEvent1Name should be associated with a list, not a dictionary.
  filtered_listeners.Set(kEvent1Name, CreateHostSuffixFilter("google.com"));

  listeners_->LoadFilteredLazyListeners(kExt1Id, false, filtered_listeners);

  std::unique_ptr<Event> event(
      CreateEvent(kEvent1Name, GURL("http://www.google.com")));
  std::set<const EventListener*> targets(listeners_->GetEventListeners(*event));
  ASSERT_EQ(0u, targets.size());
}

}  // namespace

}  // namespace extensions

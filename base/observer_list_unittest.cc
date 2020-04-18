// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/observer_list.h"
#include "base/observer_list_threadsafe.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/compiler_specific.h"
#include "base/location.h"
#include "base/memory/weak_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/sequenced_task_runner.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/waitable_event.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_scheduler.h"
#include "base/test/gtest_util.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/platform_thread.h"
#include "base/threading/thread_restrictions.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {
namespace {

class Foo {
 public:
  virtual void Observe(int x) = 0;
  virtual ~Foo() = default;
  virtual int GetValue() const { return 0; }
};

class Adder : public Foo {
 public:
  explicit Adder(int scaler) : total(0), scaler_(scaler) {}
  ~Adder() override = default;

  void Observe(int x) override { total += x * scaler_; }
  int GetValue() const override { return total; }

  int total;

 private:
  int scaler_;
};

class Disrupter : public Foo {
 public:
  Disrupter(ObserverList<Foo>* list, Foo* doomed, bool remove_self)
      : list_(list), doomed_(doomed), remove_self_(remove_self) {}
  Disrupter(ObserverList<Foo>* list, Foo* doomed)
      : Disrupter(list, doomed, false) {}
  Disrupter(ObserverList<Foo>* list, bool remove_self)
      : Disrupter(list, nullptr, remove_self) {}

  ~Disrupter() override = default;

  void Observe(int x) override {
    if (remove_self_)
      list_->RemoveObserver(this);
    if (doomed_)
      list_->RemoveObserver(doomed_);
  }

  void SetDoomed(Foo* doomed) { doomed_ = doomed; }

 private:
  ObserverList<Foo>* list_;
  Foo* doomed_;
  bool remove_self_;
};

template <typename ObserverListType>
class AddInObserve : public Foo {
 public:
  explicit AddInObserve(ObserverListType* observer_list)
      : observer_list(observer_list), to_add_() {}

  void SetToAdd(Foo* to_add) { to_add_ = to_add; }

  void Observe(int x) override {
    if (to_add_) {
      observer_list->AddObserver(to_add_);
      to_add_ = nullptr;
    }
  }

  ObserverListType* observer_list;
  Foo* to_add_;
};


static const int kThreadRunTime = 2000;  // ms to run the multi-threaded test.

// A thread for use in the ThreadSafeObserver test
// which will add and remove itself from the notification
// list repeatedly.
class AddRemoveThread : public PlatformThread::Delegate,
                        public Foo {
 public:
  AddRemoveThread(ObserverListThreadSafe<Foo>* list,
                  bool notify,
                  WaitableEvent* ready)
      : list_(list),
        loop_(nullptr),
        in_list_(false),
        start_(Time::Now()),
        count_observes_(0),
        count_addtask_(0),
        do_notifies_(notify),
        ready_(ready),
        weak_factory_(this) {}

  ~AddRemoveThread() override = default;

  void ThreadMain() override {
    loop_ = new MessageLoop();  // Fire up a message loop.
    loop_->task_runner()->PostTask(
        FROM_HERE,
        base::BindOnce(&AddRemoveThread::AddTask, weak_factory_.GetWeakPtr()));
    ready_->Signal();
    // After ready_ is signaled, loop_ is only accessed by the main test thread
    // (i.e. not this thread) in particular by Quit() which causes Run() to
    // return, and we "control" loop_ again.
    RunLoop().Run();
    delete loop_;
    loop_ = reinterpret_cast<MessageLoop*>(0xdeadbeef);
    delete this;
  }

  // This task just keeps posting to itself in an attempt
  // to race with the notifier.
  void AddTask() {
    count_addtask_++;

    if ((Time::Now() - start_).InMilliseconds() > kThreadRunTime) {
      VLOG(1) << "DONE!";
      return;
    }

    if (!in_list_) {
      list_->AddObserver(this);
      in_list_ = true;
    }

    if (do_notifies_) {
      list_->Notify(FROM_HERE, &Foo::Observe, 10);
    }

    loop_->task_runner()->PostTask(
        FROM_HERE,
        base::BindOnce(&AddRemoveThread::AddTask, weak_factory_.GetWeakPtr()));
  }

  // This function is only callable from the main thread.
  void Quit() {
    loop_->task_runner()->PostTask(
        FROM_HERE, RunLoop::QuitCurrentWhenIdleClosureDeprecated());
  }

  void Observe(int x) override {
    count_observes_++;

    // If we're getting called after we removed ourselves from
    // the list, that is very bad!
    DCHECK(in_list_);

    // This callback should fire on the appropriate thread
    EXPECT_EQ(loop_, MessageLoop::current());

    list_->RemoveObserver(this);
    in_list_ = false;
  }

 private:
  ObserverListThreadSafe<Foo>* list_;
  MessageLoop* loop_;
  bool in_list_;        // Are we currently registered for notifications.
                        // in_list_ is only used on |this| thread.
  Time start_;          // The time we started the test.

  int count_observes_;  // Number of times we observed.
  int count_addtask_;   // Number of times thread AddTask was called
  bool do_notifies_;    // Whether these threads should do notifications.
  WaitableEvent* ready_;

  base::WeakPtrFactory<AddRemoveThread> weak_factory_;
};

}  // namespace

TEST(ObserverListTest, BasicTest) {
  ObserverList<Foo> observer_list;
  const ObserverList<Foo>& const_observer_list = observer_list;

  {
    const ObserverList<Foo>::const_iterator it1 = const_observer_list.begin();
    EXPECT_EQ(it1, const_observer_list.end());
    // Iterator copy.
    const ObserverList<Foo>::const_iterator it2 = it1;
    EXPECT_EQ(it2, it1);
    // Iterator assignment.
    ObserverList<Foo>::const_iterator it3;
    it3 = it2;
    EXPECT_EQ(it3, it1);
    EXPECT_EQ(it3, it2);
    // Self assignment.
    it3 = *&it3;  // The *& defeats Clang's -Wself-assign warning.
    EXPECT_EQ(it3, it1);
    EXPECT_EQ(it3, it2);
  }

  {
    const ObserverList<Foo>::iterator it1 = observer_list.begin();
    EXPECT_EQ(it1, observer_list.end());
    // Iterator copy.
    const ObserverList<Foo>::iterator it2 = it1;
    EXPECT_EQ(it2, it1);
    // Iterator assignment.
    ObserverList<Foo>::iterator it3;
    it3 = it2;
    EXPECT_EQ(it3, it1);
    EXPECT_EQ(it3, it2);
    // Self assignment.
    it3 = *&it3;  // The *& defeats Clang's -Wself-assign warning.
    EXPECT_EQ(it3, it1);
    EXPECT_EQ(it3, it2);
  }

  Adder a(1), b(-1), c(1), d(-1), e(-1);
  Disrupter evil(&observer_list, &c);

  observer_list.AddObserver(&a);
  observer_list.AddObserver(&b);

  EXPECT_TRUE(const_observer_list.HasObserver(&a));
  EXPECT_FALSE(const_observer_list.HasObserver(&c));

  {
    const ObserverList<Foo>::const_iterator it1 = const_observer_list.begin();
    EXPECT_NE(it1, const_observer_list.end());
    // Iterator copy.
    const ObserverList<Foo>::const_iterator it2 = it1;
    EXPECT_EQ(it2, it1);
    EXPECT_NE(it2, const_observer_list.end());
    // Iterator assignment.
    ObserverList<Foo>::const_iterator it3;
    it3 = it2;
    EXPECT_EQ(it3, it1);
    EXPECT_EQ(it3, it2);
    // Self assignment.
    it3 = *&it3;  // The *& defeats Clang's -Wself-assign warning.
    EXPECT_EQ(it3, it1);
    EXPECT_EQ(it3, it2);
    // Iterator post increment.
    ObserverList<Foo>::const_iterator it4 = it3++;
    EXPECT_EQ(it4, it1);
    EXPECT_EQ(it4, it2);
    EXPECT_NE(it4, it3);
  }

  {
    const ObserverList<Foo>::iterator it1 = observer_list.begin();
    EXPECT_NE(it1, observer_list.end());
    // Iterator copy.
    const ObserverList<Foo>::iterator it2 = it1;
    EXPECT_EQ(it2, it1);
    EXPECT_NE(it2, observer_list.end());
    // Iterator assignment.
    ObserverList<Foo>::iterator it3;
    it3 = it2;
    EXPECT_EQ(it3, it1);
    EXPECT_EQ(it3, it2);
    // Self assignment.
    it3 = *&it3;  // The *& defeats Clang's -Wself-assign warning.
    EXPECT_EQ(it3, it1);
    EXPECT_EQ(it3, it2);
    // Iterator post increment.
    ObserverList<Foo>::iterator it4 = it3++;
    EXPECT_EQ(it4, it1);
    EXPECT_EQ(it4, it2);
    EXPECT_NE(it4, it3);
  }

  for (auto& observer : observer_list)
    observer.Observe(10);

  observer_list.AddObserver(&evil);
  observer_list.AddObserver(&c);
  observer_list.AddObserver(&d);

  // Removing an observer not in the list should do nothing.
  observer_list.RemoveObserver(&e);

  for (auto& observer : observer_list)
    observer.Observe(10);

  EXPECT_EQ(20, a.total);
  EXPECT_EQ(-20, b.total);
  EXPECT_EQ(0, c.total);
  EXPECT_EQ(-10, d.total);
  EXPECT_EQ(0, e.total);
}

TEST(ObserverListTest, CompactsWhenNoActiveIterator) {
  ObserverList<const Foo> ol;
  const ObserverList<const Foo>& col = ol;

  const Adder a(1);
  const Adder b(2);
  const Adder c(3);

  ol.AddObserver(&a);
  ol.AddObserver(&b);

  EXPECT_TRUE(col.HasObserver(&a));
  EXPECT_FALSE(col.HasObserver(&c));

  EXPECT_TRUE(col.might_have_observers());

  using It = ObserverList<const Foo>::const_iterator;

  {
    It it = col.begin();
    EXPECT_NE(it, col.end());
    It ita = it;
    EXPECT_EQ(ita, it);
    EXPECT_NE(++it, col.end());
    EXPECT_NE(ita, it);
    It itb = it;
    EXPECT_EQ(itb, it);
    EXPECT_EQ(++it, col.end());

    EXPECT_TRUE(col.might_have_observers());
    EXPECT_EQ(&*ita, &a);
    EXPECT_EQ(&*itb, &b);

    ol.RemoveObserver(&a);
    EXPECT_TRUE(col.might_have_observers());
    EXPECT_FALSE(col.HasObserver(&a));
    EXPECT_EQ(&*itb, &b);

    ol.RemoveObserver(&b);
    EXPECT_TRUE(col.might_have_observers());
    EXPECT_FALSE(col.HasObserver(&a));
    EXPECT_FALSE(col.HasObserver(&b));

    it = It();
    ita = It();
    EXPECT_TRUE(col.might_have_observers());
    ita = itb;
    itb = It();
    EXPECT_TRUE(col.might_have_observers());
    ita = It();
    EXPECT_FALSE(col.might_have_observers());
  }

  ol.AddObserver(&a);
  ol.AddObserver(&b);
  EXPECT_TRUE(col.might_have_observers());
  ol.Clear();
  EXPECT_FALSE(col.might_have_observers());

  ol.AddObserver(&a);
  ol.AddObserver(&b);
  EXPECT_TRUE(col.might_have_observers());
  {
    const It it = col.begin();
    ol.Clear();
    EXPECT_TRUE(col.might_have_observers());
  }
  EXPECT_FALSE(col.might_have_observers());
}

TEST(ObserverListTest, DisruptSelf) {
  ObserverList<Foo> observer_list;
  Adder a(1), b(-1), c(1), d(-1);
  Disrupter evil(&observer_list, true);

  observer_list.AddObserver(&a);
  observer_list.AddObserver(&b);

  for (auto& observer : observer_list)
    observer.Observe(10);

  observer_list.AddObserver(&evil);
  observer_list.AddObserver(&c);
  observer_list.AddObserver(&d);

  for (auto& observer : observer_list)
    observer.Observe(10);

  EXPECT_EQ(20, a.total);
  EXPECT_EQ(-20, b.total);
  EXPECT_EQ(10, c.total);
  EXPECT_EQ(-10, d.total);
}

TEST(ObserverListTest, DisruptBefore) {
  ObserverList<Foo> observer_list;
  Adder a(1), b(-1), c(1), d(-1);
  Disrupter evil(&observer_list, &b);

  observer_list.AddObserver(&a);
  observer_list.AddObserver(&b);
  observer_list.AddObserver(&evil);
  observer_list.AddObserver(&c);
  observer_list.AddObserver(&d);

  for (auto& observer : observer_list)
    observer.Observe(10);
  for (auto& observer : observer_list)
    observer.Observe(10);

  EXPECT_EQ(20, a.total);
  EXPECT_EQ(-10, b.total);
  EXPECT_EQ(20, c.total);
  EXPECT_EQ(-20, d.total);
}

TEST(ObserverListThreadSafeTest, BasicTest) {
  MessageLoop loop;

  scoped_refptr<ObserverListThreadSafe<Foo> > observer_list(
      new ObserverListThreadSafe<Foo>);
  Adder a(1);
  Adder b(-1);
  Adder c(1);
  Adder d(-1);

  observer_list->AddObserver(&a);
  observer_list->AddObserver(&b);

  observer_list->Notify(FROM_HERE, &Foo::Observe, 10);
  RunLoop().RunUntilIdle();

  observer_list->AddObserver(&c);
  observer_list->AddObserver(&d);

  observer_list->Notify(FROM_HERE, &Foo::Observe, 10);
  observer_list->RemoveObserver(&c);
  RunLoop().RunUntilIdle();

  EXPECT_EQ(20, a.total);
  EXPECT_EQ(-20, b.total);
  EXPECT_EQ(0, c.total);
  EXPECT_EQ(-10, d.total);
}

TEST(ObserverListThreadSafeTest, RemoveObserver) {
  MessageLoop loop;

  scoped_refptr<ObserverListThreadSafe<Foo> > observer_list(
      new ObserverListThreadSafe<Foo>);
  Adder a(1), b(1);

  // A workaround for the compiler bug. See http://crbug.com/121960.
  EXPECT_NE(&a, &b);

  // Should do nothing.
  observer_list->RemoveObserver(&a);
  observer_list->RemoveObserver(&b);

  observer_list->Notify(FROM_HERE, &Foo::Observe, 10);
  RunLoop().RunUntilIdle();

  EXPECT_EQ(0, a.total);
  EXPECT_EQ(0, b.total);

  observer_list->AddObserver(&a);

  // Should also do nothing.
  observer_list->RemoveObserver(&b);

  observer_list->Notify(FROM_HERE, &Foo::Observe, 10);
  RunLoop().RunUntilIdle();

  EXPECT_EQ(10, a.total);
  EXPECT_EQ(0, b.total);
}

TEST(ObserverListThreadSafeTest, WithoutSequence) {
  scoped_refptr<ObserverListThreadSafe<Foo> > observer_list(
      new ObserverListThreadSafe<Foo>);

  Adder a(1), b(1), c(1);

  // No sequence, so these should not be added.
  observer_list->AddObserver(&a);
  observer_list->AddObserver(&b);

  {
    // Add c when there's a sequence.
    MessageLoop loop;
    observer_list->AddObserver(&c);

    observer_list->Notify(FROM_HERE, &Foo::Observe, 10);
    RunLoop().RunUntilIdle();

    EXPECT_EQ(0, a.total);
    EXPECT_EQ(0, b.total);
    EXPECT_EQ(10, c.total);

    // Now add a when there's a sequence.
    observer_list->AddObserver(&a);

    // Remove c when there's a sequence.
    observer_list->RemoveObserver(&c);

    // Notify again.
    observer_list->Notify(FROM_HERE, &Foo::Observe, 20);
    RunLoop().RunUntilIdle();

    EXPECT_EQ(20, a.total);
    EXPECT_EQ(0, b.total);
    EXPECT_EQ(10, c.total);
  }

  // Removing should always succeed with or without a sequence.
  observer_list->RemoveObserver(&a);

  // Notifying should not fail but should also be a no-op.
  MessageLoop loop;
  observer_list->AddObserver(&b);
  observer_list->Notify(FROM_HERE, &Foo::Observe, 30);
  RunLoop().RunUntilIdle();

  EXPECT_EQ(20, a.total);
  EXPECT_EQ(30, b.total);
  EXPECT_EQ(10, c.total);
}

class FooRemover : public Foo {
 public:
  explicit FooRemover(ObserverListThreadSafe<Foo>* list) : list_(list) {}
  ~FooRemover() override = default;

  void AddFooToRemove(Foo* foo) {
    foos_.push_back(foo);
  }

  void Observe(int x) override {
    std::vector<Foo*> tmp;
    tmp.swap(foos_);
    for (std::vector<Foo*>::iterator it = tmp.begin();
         it != tmp.end(); ++it) {
      list_->RemoveObserver(*it);
    }
  }

 private:
  const scoped_refptr<ObserverListThreadSafe<Foo> > list_;
  std::vector<Foo*> foos_;
};

TEST(ObserverListThreadSafeTest, RemoveMultipleObservers) {
  MessageLoop loop;
  scoped_refptr<ObserverListThreadSafe<Foo> > observer_list(
      new ObserverListThreadSafe<Foo>);

  FooRemover a(observer_list.get());
  Adder b(1);

  observer_list->AddObserver(&a);
  observer_list->AddObserver(&b);

  a.AddFooToRemove(&a);
  a.AddFooToRemove(&b);

  observer_list->Notify(FROM_HERE, &Foo::Observe, 1);
  RunLoop().RunUntilIdle();
}

// A test driver for a multi-threaded notification loop.  Runs a number
// of observer threads, each of which constantly adds/removes itself
// from the observer list.  Optionally, if cross_thread_notifies is set
// to true, the observer threads will also trigger notifications to
// all observers.
static void ThreadSafeObserverHarness(int num_threads,
                                      bool cross_thread_notifies) {
  MessageLoop loop;

  scoped_refptr<ObserverListThreadSafe<Foo> > observer_list(
      new ObserverListThreadSafe<Foo>);
  Adder a(1);
  Adder b(-1);

  observer_list->AddObserver(&a);
  observer_list->AddObserver(&b);

  std::vector<AddRemoveThread*> threaded_observer;
  std::vector<base::PlatformThreadHandle> threads(num_threads);
  std::vector<std::unique_ptr<base::WaitableEvent>> ready;
  threaded_observer.reserve(num_threads);
  ready.reserve(num_threads);
  for (int index = 0; index < num_threads; index++) {
    ready.push_back(std::make_unique<WaitableEvent>(
        WaitableEvent::ResetPolicy::MANUAL,
        WaitableEvent::InitialState::NOT_SIGNALED));
    threaded_observer.push_back(new AddRemoveThread(
        observer_list.get(), cross_thread_notifies, ready.back().get()));
    EXPECT_TRUE(
        PlatformThread::Create(0, threaded_observer.back(), &threads[index]));
  }
  ASSERT_EQ(static_cast<size_t>(num_threads), threaded_observer.size());
  ASSERT_EQ(static_cast<size_t>(num_threads), ready.size());

  // This makes sure that threaded_observer has gotten to set loop_, so that we
  // can call Quit() below safe-ish-ly.
  for (int i = 0; i < num_threads; ++i)
    ready[i]->Wait();

  Time start = Time::Now();
  while (true) {
    if ((Time::Now() - start).InMilliseconds() > kThreadRunTime)
      break;

    observer_list->Notify(FROM_HERE, &Foo::Observe, 10);

    RunLoop().RunUntilIdle();
  }

  for (int index = 0; index < num_threads; index++) {
    threaded_observer[index]->Quit();
    PlatformThread::Join(threads[index]);
  }
}

TEST(ObserverListThreadSafeTest, CrossThreadObserver) {
  // Use 7 observer threads.  Notifications only come from
  // the main thread.
  ThreadSafeObserverHarness(7, false);
}

TEST(ObserverListThreadSafeTest, CrossThreadNotifications) {
  // Use 3 observer threads.  Notifications will fire from
  // the main thread and all 3 observer threads.
  ThreadSafeObserverHarness(3, true);
}

TEST(ObserverListThreadSafeTest, OutlivesMessageLoop) {
  MessageLoop* loop = new MessageLoop;
  scoped_refptr<ObserverListThreadSafe<Foo> > observer_list(
      new ObserverListThreadSafe<Foo>);

  Adder a(1);
  observer_list->AddObserver(&a);
  delete loop;
  // Test passes if we don't crash here.
  observer_list->Notify(FROM_HERE, &Foo::Observe, 1);
}

namespace {

class SequenceVerificationObserver : public Foo {
 public:
  explicit SequenceVerificationObserver(
      scoped_refptr<SequencedTaskRunner> task_runner)
      : task_runner_(std::move(task_runner)) {}
  ~SequenceVerificationObserver() override = default;

  void Observe(int x) override {
    called_on_valid_sequence_ = task_runner_->RunsTasksInCurrentSequence();
  }

  bool called_on_valid_sequence() const { return called_on_valid_sequence_; }

 private:
  const scoped_refptr<SequencedTaskRunner> task_runner_;
  bool called_on_valid_sequence_ = false;

  DISALLOW_COPY_AND_ASSIGN(SequenceVerificationObserver);
};

}  // namespace

// Verify that observers are notified on the correct sequence.
TEST(ObserverListThreadSafeTest, NotificationOnValidSequence) {
  test::ScopedTaskEnvironment scoped_task_environment;

  auto task_runner_1 = CreateSequencedTaskRunnerWithTraits(TaskTraits());
  auto task_runner_2 = CreateSequencedTaskRunnerWithTraits(TaskTraits());

  auto observer_list = MakeRefCounted<ObserverListThreadSafe<Foo>>();

  SequenceVerificationObserver observer_1(task_runner_1);
  SequenceVerificationObserver observer_2(task_runner_2);

  task_runner_1->PostTask(FROM_HERE,
                          BindOnce(&ObserverListThreadSafe<Foo>::AddObserver,
                                   observer_list, Unretained(&observer_1)));
  task_runner_2->PostTask(FROM_HERE,
                          BindOnce(&ObserverListThreadSafe<Foo>::AddObserver,
                                   observer_list, Unretained(&observer_2)));

  TaskScheduler::GetInstance()->FlushForTesting();

  observer_list->Notify(FROM_HERE, &Foo::Observe, 1);

  TaskScheduler::GetInstance()->FlushForTesting();

  EXPECT_TRUE(observer_1.called_on_valid_sequence());
  EXPECT_TRUE(observer_2.called_on_valid_sequence());
}

// Verify that when an observer is added to a NOTIFY_ALL ObserverListThreadSafe
// from a notification, it is itself notified.
TEST(ObserverListThreadSafeTest, AddObserverFromNotificationNotifyAll) {
  test::ScopedTaskEnvironment scoped_task_environment;
  auto observer_list = MakeRefCounted<ObserverListThreadSafe<Foo>>();

  Adder observer_added_from_notification(1);

  AddInObserve<ObserverListThreadSafe<Foo>> initial_observer(
      observer_list.get());
  initial_observer.SetToAdd(&observer_added_from_notification);
  observer_list->AddObserver(&initial_observer);

  observer_list->Notify(FROM_HERE, &Foo::Observe, 1);

  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(1, observer_added_from_notification.GetValue());
}

namespace {

class RemoveWhileNotificationIsRunningObserver : public Foo {
 public:
  RemoveWhileNotificationIsRunningObserver()
      : notification_running_(WaitableEvent::ResetPolicy::AUTOMATIC,
                              WaitableEvent::InitialState::NOT_SIGNALED),
        barrier_(WaitableEvent::ResetPolicy::AUTOMATIC,
                 WaitableEvent::InitialState::NOT_SIGNALED) {}
  ~RemoveWhileNotificationIsRunningObserver() override = default;

  void Observe(int x) override {
    notification_running_.Signal();
    ScopedAllowBaseSyncPrimitivesForTesting allow_base_sync_primitives;
    barrier_.Wait();
  }

  void WaitForNotificationRunning() { notification_running_.Wait(); }
  void Unblock() { barrier_.Signal(); }

 private:
  WaitableEvent notification_running_;
  WaitableEvent barrier_;

  DISALLOW_COPY_AND_ASSIGN(RemoveWhileNotificationIsRunningObserver);
};

}  // namespace

// Verify that there is no crash when an observer is removed while it is being
// notified.
TEST(ObserverListThreadSafeTest, RemoveWhileNotificationIsRunning) {
  auto observer_list = MakeRefCounted<ObserverListThreadSafe<Foo>>();
  RemoveWhileNotificationIsRunningObserver observer;

  WaitableEvent task_running(WaitableEvent::ResetPolicy::AUTOMATIC,
                             WaitableEvent::InitialState::NOT_SIGNALED);
  WaitableEvent barrier(WaitableEvent::ResetPolicy::AUTOMATIC,
                        WaitableEvent::InitialState::NOT_SIGNALED);

  // This must be after the declaration of |barrier| so that tasks posted to
  // TaskScheduler can safely use |barrier|.
  test::ScopedTaskEnvironment scoped_task_environment;

  CreateSequencedTaskRunnerWithTraits({})->PostTask(
      FROM_HERE, base::BindOnce(&ObserverListThreadSafe<Foo>::AddObserver,
                                observer_list, Unretained(&observer)));
  TaskScheduler::GetInstance()->FlushForTesting();

  observer_list->Notify(FROM_HERE, &Foo::Observe, 1);
  observer.WaitForNotificationRunning();
  observer_list->RemoveObserver(&observer);

  observer.Unblock();
}

TEST(ObserverListTest, Existing) {
  ObserverList<Foo> observer_list(ObserverListPolicy::EXISTING_ONLY);
  Adder a(1);
  AddInObserve<ObserverList<Foo> > b(&observer_list);
  Adder c(1);
  b.SetToAdd(&c);

  observer_list.AddObserver(&a);
  observer_list.AddObserver(&b);

  for (auto& observer : observer_list)
    observer.Observe(1);

  EXPECT_FALSE(b.to_add_);
  // B's adder should not have been notified because it was added during
  // notification.
  EXPECT_EQ(0, c.total);

  // Notify again to make sure b's adder is notified.
  for (auto& observer : observer_list)
    observer.Observe(1);
  EXPECT_EQ(1, c.total);
}

// Same as above, but for ObserverListThreadSafe
TEST(ObserverListThreadSafeTest, Existing) {
  MessageLoop loop;
  scoped_refptr<ObserverListThreadSafe<Foo>> observer_list(
      new ObserverListThreadSafe<Foo>(ObserverListPolicy::EXISTING_ONLY));
  Adder a(1);
  AddInObserve<ObserverListThreadSafe<Foo> > b(observer_list.get());
  Adder c(1);
  b.SetToAdd(&c);

  observer_list->AddObserver(&a);
  observer_list->AddObserver(&b);

  observer_list->Notify(FROM_HERE, &Foo::Observe, 1);
  RunLoop().RunUntilIdle();

  EXPECT_FALSE(b.to_add_);
  // B's adder should not have been notified because it was added during
  // notification.
  EXPECT_EQ(0, c.total);

  // Notify again to make sure b's adder is notified.
  observer_list->Notify(FROM_HERE, &Foo::Observe, 1);
  RunLoop().RunUntilIdle();
  EXPECT_EQ(1, c.total);
}

class AddInClearObserve : public Foo {
 public:
  explicit AddInClearObserve(ObserverList<Foo>* list)
      : list_(list), added_(false), adder_(1) {}

  void Observe(int /* x */) override {
    list_->Clear();
    list_->AddObserver(&adder_);
    added_ = true;
  }

  bool added() const { return added_; }
  const Adder& adder() const { return adder_; }

 private:
  ObserverList<Foo>* const list_;

  bool added_;
  Adder adder_;
};

TEST(ObserverListTest, ClearNotifyAll) {
  ObserverList<Foo> observer_list;
  AddInClearObserve a(&observer_list);

  observer_list.AddObserver(&a);

  for (auto& observer : observer_list)
    observer.Observe(1);
  EXPECT_TRUE(a.added());
  EXPECT_EQ(1, a.adder().total)
      << "Adder should observe once and have sum of 1.";
}

TEST(ObserverListTest, ClearNotifyExistingOnly) {
  ObserverList<Foo> observer_list(ObserverListPolicy::EXISTING_ONLY);
  AddInClearObserve a(&observer_list);

  observer_list.AddObserver(&a);

  for (auto& observer : observer_list)
    observer.Observe(1);
  EXPECT_TRUE(a.added());
  EXPECT_EQ(0, a.adder().total)
      << "Adder should not observe, so sum should still be 0.";
}

class ListDestructor : public Foo {
 public:
  explicit ListDestructor(ObserverList<Foo>* list) : list_(list) {}
  ~ListDestructor() override = default;

  void Observe(int x) override { delete list_; }

 private:
  ObserverList<Foo>* list_;
};


TEST(ObserverListTest, IteratorOutlivesList) {
  ObserverList<Foo>* observer_list = new ObserverList<Foo>;
  ListDestructor a(observer_list);
  observer_list->AddObserver(&a);

  for (auto& observer : *observer_list)
    observer.Observe(0);

  // There are no EXPECT* statements for this test, if we catch
  // use-after-free errors for observer_list (eg with ASan) then
  // this test has failed.  See http://crbug.com/85296.
}

TEST(ObserverListTest, BasicStdIterator) {
  using FooList = ObserverList<Foo>;
  FooList observer_list;

  // An optimization: begin() and end() do not involve weak pointers on
  // empty list.
  EXPECT_FALSE(observer_list.begin().list_);
  EXPECT_FALSE(observer_list.end().list_);

  // Iterate over empty list: no effect, no crash.
  for (auto& i : observer_list)
    i.Observe(10);

  Adder a(1), b(-1), c(1), d(-1);

  observer_list.AddObserver(&a);
  observer_list.AddObserver(&b);
  observer_list.AddObserver(&c);
  observer_list.AddObserver(&d);

  for (FooList::iterator i = observer_list.begin(), e = observer_list.end();
       i != e; ++i)
    i->Observe(1);

  EXPECT_EQ(1, a.total);
  EXPECT_EQ(-1, b.total);
  EXPECT_EQ(1, c.total);
  EXPECT_EQ(-1, d.total);

  // Check an iteration over a 'const view' for a given container.
  const FooList& const_list = observer_list;
  for (FooList::const_iterator i = const_list.begin(), e = const_list.end();
       i != e; ++i) {
    EXPECT_EQ(1, std::abs(i->GetValue()));
  }

  for (const auto& o : const_list)
    EXPECT_EQ(1, std::abs(o.GetValue()));
}

TEST(ObserverListTest, StdIteratorRemoveItself) {
  ObserverList<Foo> observer_list;
  Adder a(1), b(-1), c(1), d(-1);
  Disrupter disrupter(&observer_list, true);

  observer_list.AddObserver(&a);
  observer_list.AddObserver(&b);
  observer_list.AddObserver(&disrupter);
  observer_list.AddObserver(&c);
  observer_list.AddObserver(&d);

  for (auto& o : observer_list)
    o.Observe(1);

  for (auto& o : observer_list)
    o.Observe(10);

  EXPECT_EQ(11, a.total);
  EXPECT_EQ(-11, b.total);
  EXPECT_EQ(11, c.total);
  EXPECT_EQ(-11, d.total);
}

TEST(ObserverListTest, StdIteratorRemoveBefore) {
  ObserverList<Foo> observer_list;
  Adder a(1), b(-1), c(1), d(-1);
  Disrupter disrupter(&observer_list, &b);

  observer_list.AddObserver(&a);
  observer_list.AddObserver(&b);
  observer_list.AddObserver(&disrupter);
  observer_list.AddObserver(&c);
  observer_list.AddObserver(&d);

  for (auto& o : observer_list)
    o.Observe(1);

  for (auto& o : observer_list)
    o.Observe(10);

  EXPECT_EQ(11, a.total);
  EXPECT_EQ(-1, b.total);
  EXPECT_EQ(11, c.total);
  EXPECT_EQ(-11, d.total);
}

TEST(ObserverListTest, StdIteratorRemoveAfter) {
  ObserverList<Foo> observer_list;
  Adder a(1), b(-1), c(1), d(-1);
  Disrupter disrupter(&observer_list, &c);

  observer_list.AddObserver(&a);
  observer_list.AddObserver(&b);
  observer_list.AddObserver(&disrupter);
  observer_list.AddObserver(&c);
  observer_list.AddObserver(&d);

  for (auto& o : observer_list)
    o.Observe(1);

  for (auto& o : observer_list)
    o.Observe(10);

  EXPECT_EQ(11, a.total);
  EXPECT_EQ(-11, b.total);
  EXPECT_EQ(0, c.total);
  EXPECT_EQ(-11, d.total);
}

TEST(ObserverListTest, StdIteratorRemoveAfterFront) {
  ObserverList<Foo> observer_list;
  Adder a(1), b(-1), c(1), d(-1);
  Disrupter disrupter(&observer_list, &a);

  observer_list.AddObserver(&a);
  observer_list.AddObserver(&disrupter);
  observer_list.AddObserver(&b);
  observer_list.AddObserver(&c);
  observer_list.AddObserver(&d);

  for (auto& o : observer_list)
    o.Observe(1);

  for (auto& o : observer_list)
    o.Observe(10);

  EXPECT_EQ(1, a.total);
  EXPECT_EQ(-11, b.total);
  EXPECT_EQ(11, c.total);
  EXPECT_EQ(-11, d.total);
}

TEST(ObserverListTest, StdIteratorRemoveBeforeBack) {
  ObserverList<Foo> observer_list;
  Adder a(1), b(-1), c(1), d(-1);
  Disrupter disrupter(&observer_list, &d);

  observer_list.AddObserver(&a);
  observer_list.AddObserver(&b);
  observer_list.AddObserver(&c);
  observer_list.AddObserver(&disrupter);
  observer_list.AddObserver(&d);

  for (auto& o : observer_list)
    o.Observe(1);

  for (auto& o : observer_list)
    o.Observe(10);

  EXPECT_EQ(11, a.total);
  EXPECT_EQ(-11, b.total);
  EXPECT_EQ(11, c.total);
  EXPECT_EQ(0, d.total);
}

TEST(ObserverListTest, StdIteratorRemoveFront) {
  using FooList = ObserverList<Foo>;
  FooList observer_list;
  Adder a(1), b(-1), c(1), d(-1);
  Disrupter disrupter(&observer_list, true);

  observer_list.AddObserver(&disrupter);
  observer_list.AddObserver(&a);
  observer_list.AddObserver(&b);
  observer_list.AddObserver(&c);
  observer_list.AddObserver(&d);

  bool test_disruptor = true;
  for (FooList::iterator i = observer_list.begin(), e = observer_list.end();
       i != e; ++i) {
    i->Observe(1);
    // Check that second call to i->Observe() would crash here.
    if (test_disruptor) {
      EXPECT_FALSE(i.GetCurrent());
      test_disruptor = false;
    }
  }

  for (auto& o : observer_list)
    o.Observe(10);

  EXPECT_EQ(11, a.total);
  EXPECT_EQ(-11, b.total);
  EXPECT_EQ(11, c.total);
  EXPECT_EQ(-11, d.total);
}

TEST(ObserverListTest, StdIteratorRemoveBack) {
  ObserverList<Foo> observer_list;
  Adder a(1), b(-1), c(1), d(-1);
  Disrupter disrupter(&observer_list, true);

  observer_list.AddObserver(&a);
  observer_list.AddObserver(&b);
  observer_list.AddObserver(&c);
  observer_list.AddObserver(&d);
  observer_list.AddObserver(&disrupter);

  for (auto& o : observer_list)
    o.Observe(1);

  for (auto& o : observer_list)
    o.Observe(10);

  EXPECT_EQ(11, a.total);
  EXPECT_EQ(-11, b.total);
  EXPECT_EQ(11, c.total);
  EXPECT_EQ(-11, d.total);
}

TEST(ObserverListTest, NestedLoop) {
  ObserverList<Foo> observer_list;
  Adder a(1), b(-1), c(1), d(-1);
  Disrupter disrupter(&observer_list, true);

  observer_list.AddObserver(&disrupter);
  observer_list.AddObserver(&a);
  observer_list.AddObserver(&b);
  observer_list.AddObserver(&c);
  observer_list.AddObserver(&d);

  for (auto& o : observer_list) {
    o.Observe(10);

    for (auto& o : observer_list)
      o.Observe(1);
  }

  EXPECT_EQ(15, a.total);
  EXPECT_EQ(-15, b.total);
  EXPECT_EQ(15, c.total);
  EXPECT_EQ(-15, d.total);
}

TEST(ObserverListTest, NonCompactList) {
  ObserverList<Foo> observer_list;
  Adder a(1), b(-1);

  Disrupter disrupter1(&observer_list, true);
  Disrupter disrupter2(&observer_list, true);

  // Disrupt itself and another one.
  disrupter1.SetDoomed(&disrupter2);

  observer_list.AddObserver(&disrupter1);
  observer_list.AddObserver(&disrupter2);
  observer_list.AddObserver(&a);
  observer_list.AddObserver(&b);

  for (auto& o : observer_list) {
    // Get the { nullptr, nullptr, &a, &b } non-compact list
    // on the first inner pass.
    o.Observe(10);

    for (auto& o : observer_list)
      o.Observe(1);
  }

  EXPECT_EQ(13, a.total);
  EXPECT_EQ(-13, b.total);
}

TEST(ObserverListTest, BecomesEmptyThanNonEmpty) {
  ObserverList<Foo> observer_list;
  Adder a(1), b(-1);

  Disrupter disrupter1(&observer_list, true);
  Disrupter disrupter2(&observer_list, true);

  // Disrupt itself and another one.
  disrupter1.SetDoomed(&disrupter2);

  observer_list.AddObserver(&disrupter1);
  observer_list.AddObserver(&disrupter2);

  bool add_observers = true;
  for (auto& o : observer_list) {
    // Get the { nullptr, nullptr } empty list on the first inner pass.
    o.Observe(10);

    for (auto& o : observer_list)
      o.Observe(1);

    if (add_observers) {
      observer_list.AddObserver(&a);
      observer_list.AddObserver(&b);
      add_observers = false;
    }
  }

  EXPECT_EQ(12, a.total);
  EXPECT_EQ(-12, b.total);
}

TEST(ObserverListTest, AddObserverInTheLastObserve) {
  using FooList = ObserverList<Foo>;
  FooList observer_list;

  AddInObserve<FooList> a(&observer_list);
  Adder b(-1);

  a.SetToAdd(&b);
  observer_list.AddObserver(&a);

  auto it = observer_list.begin();
  while (it != observer_list.end()) {
    auto& observer = *it;
    // Intentionally increment the iterator before calling Observe(). The
    // ObserverList starts with only one observer, and it == observer_list.end()
    // should be true after the next line.
    ++it;
    // However, the first Observe() call will add a second observer: at this
    // point, it != observer_list.end() should be true, and Observe() should be
    // called on the newly added observer on the next iteration of the loop.
    observer.Observe(10);
  }

  EXPECT_EQ(-10, b.total);
}

class MockLogAssertHandler {
 public:
  MOCK_METHOD4(
      HandleLogAssert,
      void(const char*, int, const base::StringPiece, const base::StringPiece));
};

#if DCHECK_IS_ON()
TEST(ObserverListTest, NonReentrantObserverList) {
  using ::testing::_;

  ObserverList<Foo, /*check_empty=*/false, /*allow_reentrancy=*/false>
      non_reentrant_observer_list;
  Adder a(1);
  non_reentrant_observer_list.AddObserver(&a);

  EXPECT_DCHECK_DEATH({
    for (const Foo& a : non_reentrant_observer_list) {
      for (const Foo& b : non_reentrant_observer_list) {
        std::ignore = a;
        std::ignore = b;
      }
    }
  });
}

TEST(ObserverListTest, ReentrantObserverList) {
  using ::testing::_;

  ReentrantObserverList<Foo> reentrant_observer_list;
  Adder a(1);
  reentrant_observer_list.AddObserver(&a);
  bool passed = false;
  for (const Foo& a : reentrant_observer_list) {
    for (const Foo& b : reentrant_observer_list) {
      std::ignore = a;
      std::ignore = b;
      passed = true;
    }
  }
  EXPECT_TRUE(passed);
}
#endif

}  // namespace base

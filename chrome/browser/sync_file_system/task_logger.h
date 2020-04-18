// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_FILE_SYSTEM_TASK_LOGGER_H_
#define CHROME_BROWSER_SYNC_FILE_SYSTEM_TASK_LOGGER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/containers/circular_deque.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/time/time.h"

namespace sync_file_system {

class TaskLogger : public base::SupportsWeakPtr<TaskLogger> {
 public:
  struct TaskLog {
    int log_id;
    base::TimeTicks start_time;
    base::TimeTicks end_time;
    std::string task_description;
    std::string result_description;
    std::vector<std::string> details;

    TaskLog();
    ~TaskLog();
  };

  using LogList = base::circular_deque<std::unique_ptr<TaskLog>>;

  class Observer {
   public:
    virtual void OnLogRecorded(const TaskLog& task_log) = 0;

   protected:
    Observer() {}
    virtual ~Observer() {}

   private:
    DISALLOW_COPY_AND_ASSIGN(Observer);
  };

  TaskLogger();
  ~TaskLogger();

  void RecordLog(std::unique_ptr<TaskLog> log);
  void ClearLog();

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  const LogList& GetLog() const;

 private:
  LogList log_history_;

  base::ObserverList<Observer> observers_;

  DISALLOW_COPY_AND_ASSIGN(TaskLogger);
};

}  // namespace sync_file_system

#endif  // CHROME_BROWSER_SYNC_FILE_SYSTEM_TASK_LOGGER_H_

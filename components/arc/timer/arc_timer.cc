// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/timer/arc_timer.h"

#include <stdlib.h>
#include <unistd.h>

#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/logging.h"
#include "base/posix/unix_domain_socket.h"
#include "base/threading/thread_restrictions.h"

namespace arc {

ArcTimer::ArcTimer(base::ScopedFD expiration_fd,
                   mojo::InterfaceRequest<mojom::Timer> request,
                   ConnectionErrorCallback connection_error_callback)
    : binding_(this, std::move(request)),
      expiration_fd_(std::move(expiration_fd)),
      connection_error_callback_(std::move(connection_error_callback)) {
  // It's safe to pass |this| because the error handler will be reset when
  // the |binding_| is unbound or closed.
  binding_.set_connection_error_handler(
      base::BindOnce(&ArcTimer::HandleConnectionError, base::Unretained(this)));
}

ArcTimer::~ArcTimer() = default;

void ArcTimer::Start(base::TimeTicks absolute_expiration_time,
                     StartCallback callback) {
  // Start the timer to expire at |absolute_expiration_time|. This call
  // automatically overrides the previous timer set.
  //
  // If the firing time has expired then set the timer to expire
  // immediately.
  base::TimeTicks current_time_ticks = base::TimeTicks::Now();
  base::TimeDelta delay;
  if (absolute_expiration_time > current_time_ticks)
    delay = absolute_expiration_time - current_time_ticks;
  base::Time current_time = base::Time::Now();
  DVLOG(1) << "CurrentTime: " << current_time
           << " NextAlarmAt: " << current_time + delay;
  // It's safe to pass |this| because on destruction of this object the
  // timer is stopped and the callback can't be invoked.
  timer_.Start(
      FROM_HERE, delay,
      base::BindRepeating(&ArcTimer::OnExpiration, base::Unretained(this)));
  std::move(callback).Run(arc::mojom::ArcTimerResult::SUCCESS);
}

void ArcTimer::HandleConnectionError() {
  // Stop any pending timers when connection with the instance is dropped.
  timer_.Stop();
  std::move(connection_error_callback_).Run(this);
}

void ArcTimer::OnExpiration() {
  DVLOG(1) << "Expiration callback";
  base::AssertBlockingAllowed();
  // The instance expects 8 bytes on the read end similar to what happens on
  // a timerfd expiration. The timerfd API expects this to be the number of
  // expirations, however, more than one expiration isn't tracked currently.
  const uint64_t timer_data = 1;
  if (!base::UnixDomainSocket::SendMsg(expiration_fd_.get(), &timer_data,
                                       sizeof(timer_data),
                                       std::vector<int>())) {
    LOG(ERROR) << "Failed to indicate timer expiration to the instance";
  }
}

}  // namespace arc

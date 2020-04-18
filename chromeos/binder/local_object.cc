// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/binder/local_object.h"

#include <utility>

#include "chromeos/binder/transaction_data.h"

namespace binder {

LocalObject::LocalObject(std::unique_ptr<TransactionHandler> handler)
    : handler_(std::move(handler)) {}

LocalObject::~LocalObject() = default;

Object::Type LocalObject::GetType() const {
  return TYPE_LOCAL;
}

bool LocalObject::Transact(CommandBroker* command_broker,
                           const TransactionData& data,
                           std::unique_ptr<TransactionData>* reply) {
  *reply = handler_->OnTransact(command_broker, data);
  return true;
}

}  // namespace binder

// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/account_id/account_id.h"

#include <functional>
#include <memory>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/memory/singleton.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "google_apis/gaia/gaia_auth_util.h"

namespace {

// Serialization keys
const char kGaiaIdKey[] = "gaia_id";
const char kEmailKey[] = "email";
const char kObjGuid[] = "obj_guid";
const char kAccountTypeKey[] = "account_type";

// Serialization values for account type.
const char kGoogle[] = "google";
const char kAd[] = "ad";
const char kUnknown[] = "unknown";

// Prefix for GetAccountIdKey().
const char kKeyGaiaIdPrefix[] = "g-";
const char kKeyAdIdPrefix[] = "a-";

}  // anonymous namespace

struct AccountId::EmptyAccountId {
  EmptyAccountId() : user_id() {}
  const AccountId user_id;

  static EmptyAccountId* GetInstance() {
    return base::Singleton<EmptyAccountId>::get();
  }
};

AccountId::AccountId() {}

AccountId::AccountId(const std::string& id,
                     const std::string& user_email,
                     const AccountType& account_type)
    : id_(id), user_email_(user_email), account_type_(account_type) {
  DCHECK(account_type != AccountType::UNKNOWN || id.empty());
  DCHECK(account_type != AccountType::ACTIVE_DIRECTORY || !id.empty());
  // Fail if e-mail looks similar to GaiaIdKey.
  LOG_ASSERT(!base::StartsWith(user_email, kKeyGaiaIdPrefix,
                               base::CompareCase::SENSITIVE) ||
             user_email.find('@') != std::string::npos)
      << "Bad e-mail: '" << user_email << "' with gaia_id='" << id << "'";

  // TODO(alemate): DCHECK(!email.empty());
  // TODO(alemate): check gaia_id is not empty once it is required.
}

AccountId::AccountId(const AccountId& other)
    : id_(other.id_),
      user_email_(other.user_email_),
      account_type_(other.account_type_) {}

bool AccountId::operator==(const AccountId& other) const {
  if (this == &other)
    return true;
  if (account_type_ == AccountType::UNKNOWN ||
      other.account_type_ == AccountType::UNKNOWN)
    return user_email_ == other.user_email_;
  if (account_type_ != other.account_type_)
    return false;
  switch (account_type_) {
    case AccountType::GOOGLE:
      return (id_ == other.id_ && user_email_ == other.user_email_) ||
             (!id_.empty() && id_ == other.id_) ||
             (!user_email_.empty() && user_email_ == other.user_email_);
    case AccountType::ACTIVE_DIRECTORY:
      return id_ == other.id_ && user_email_ == other.user_email_;
    default:
      NOTREACHED() << "Unknown account type";
  }
  return false;
}

bool AccountId::operator!=(const AccountId& other) const {
  return !operator==(other);
}

bool AccountId::operator<(const AccountId& right) const {
  // TODO(alemate): update this once all AccountId members are filled.
  return user_email_ < right.user_email_;
}

bool AccountId::empty() const {
  return id_.empty() && user_email_.empty() &&
         account_type_ == AccountType::UNKNOWN;
}

bool AccountId::is_valid() const {
  switch (account_type_) {
    case AccountType::GOOGLE:
      return /* !id_.empty() && */ !user_email_.empty();
    case AccountType::ACTIVE_DIRECTORY:
      return !id_.empty() && !user_email_.empty();
    case AccountType::UNKNOWN:
      return id_.empty() && !user_email_.empty();
  }
  NOTREACHED();
  return false;
}

void AccountId::clear() {
  id_.clear();
  user_email_.clear();
  account_type_ = AccountType::UNKNOWN;
}

AccountType AccountId::GetAccountType() const {
  return account_type_;
}

const std::string& AccountId::GetGaiaId() const {
  if (account_type_ != AccountType::GOOGLE)
    NOTIMPLEMENTED() << "Failed to get gaia_id for non-Google account.";
  return id_;
}

const std::string& AccountId::GetObjGuid() const {
  if (account_type_ != AccountType::ACTIVE_DIRECTORY)
    NOTIMPLEMENTED()
        << "Failed to get obj_guid for non-Active Directory account.";
  return id_;
}

const std::string& AccountId::GetUserEmail() const {
  return user_email_;
}

bool AccountId::HasAccountIdKey() const {
  return account_type_ != AccountType::UNKNOWN && !id_.empty();
}

const std::string AccountId::GetAccountIdKey() const {
#ifdef NDEBUG
  if (id_.empty())
    LOG(FATAL) << "GetAccountIdKey(): no id for " << Serialize();
#else
  CHECK(!id_.empty());
#endif
  switch (GetAccountType()) {
    case AccountType::GOOGLE:
      return std::string(kKeyGaiaIdPrefix) + id_;
    case AccountType::ACTIVE_DIRECTORY:
      return std::string(kKeyAdIdPrefix) + id_;
    default:
      NOTREACHED() << "Unknown account type";
  }
  return std::string();
}

void AccountId::SetUserEmail(const std::string& email) {
  DCHECK(!email.empty());
  user_email_ = email;
}

// static
AccountId AccountId::FromUserEmail(const std::string& email) {
  // TODO(alemate): DCHECK(!email.empty());
  return AccountId(std::string() /* id */, email, AccountType::UNKNOWN);
}

// static
AccountId AccountId::FromGaiaId(const std::string& gaia_id) {
  DCHECK(!gaia_id.empty());
  return AccountId(gaia_id, std::string() /* email */, AccountType::GOOGLE);
}

// static
AccountId AccountId::FromUserEmailGaiaId(const std::string& email,
                                         const std::string& gaia_id) {
  DCHECK(!(email.empty() && gaia_id.empty()));
  return AccountId(gaia_id, email, AccountType::GOOGLE);
}

// static
AccountId AccountId::AdFromUserEmailObjGuid(const std::string& email,
                                            const std::string& obj_guid) {
  DCHECK(!email.empty() && !obj_guid.empty());
  return AccountId(obj_guid, email, AccountType::ACTIVE_DIRECTORY);
}

// static
AccountId AccountId::AdFromObjGuid(const std::string& obj_guid) {
  DCHECK(!obj_guid.empty());
  return AccountId(obj_guid, std::string() /* email */,
                   AccountType::ACTIVE_DIRECTORY);
}

// static
AccountType AccountId::StringToAccountType(
    const std::string& account_type_string) {
  if (account_type_string == kGoogle)
    return AccountType::GOOGLE;
  if (account_type_string == kAd)
    return AccountType::ACTIVE_DIRECTORY;
  if (account_type_string == kUnknown)
    return AccountType::UNKNOWN;
  NOTREACHED() << "Unknown account type " << account_type_string;
  return AccountType::UNKNOWN;
}

// static
std::string AccountId::AccountTypeToString(const AccountType& account_type) {
  switch (account_type) {
    case AccountType::GOOGLE:
      return kGoogle;
    case AccountType::ACTIVE_DIRECTORY:
      return kAd;
    case AccountType::UNKNOWN:
      return kUnknown;
  }
  return std::string();
}

std::string AccountId::Serialize() const {
  base::DictionaryValue value;
  switch (GetAccountType()) {
    case AccountType::GOOGLE:
      value.SetString(kGaiaIdKey, id_);
      break;
    case AccountType::ACTIVE_DIRECTORY:
      value.SetString(kObjGuid, id_);
      break;
    case AccountType::UNKNOWN:
      break;
  }
  value.SetString(kAccountTypeKey, AccountTypeToString(GetAccountType()));
  value.SetString(kEmailKey, user_email_);

  std::string serialized;
  base::JSONWriter::Write(value, &serialized);
  return serialized;
}

// static
bool AccountId::Deserialize(const std::string& serialized,
                            AccountId* account_id) {
  base::JSONReader reader;
  std::unique_ptr<const base::Value> value(reader.Read(serialized));
  const base::DictionaryValue* dictionary_value = nullptr;

  if (!value || !value->GetAsDictionary(&dictionary_value))
    return false;

  std::string gaia_id;
  std::string user_email;
  std::string obj_guid;
  std::string account_type_string;
  AccountType account_type = AccountType::GOOGLE;

  const bool found_gaia_id = dictionary_value->GetString(kGaiaIdKey, &gaia_id);
  const bool found_user_email =
      dictionary_value->GetString(kEmailKey, &user_email);
  const bool found_obj_guid = dictionary_value->GetString(kObjGuid, &obj_guid);
  const bool found_account_type =
      dictionary_value->GetString(kAccountTypeKey, &account_type_string);
  if (found_account_type)
    account_type = StringToAccountType(account_type_string);

  switch (account_type) {
    case AccountType::GOOGLE:
      if (found_obj_guid)
        DLOG(ERROR) << "AccountType is 'google' but obj_guid is found in '"
                    << serialized << "'";

      if (!found_gaia_id)
        DLOG(ERROR) << "gaia_id is not found in '" << serialized << "'";

      if (!found_user_email)
        DLOG(ERROR) << "user_email is not found in '" << serialized << "'";

      if (!found_gaia_id && !found_user_email)
        return false;

      *account_id = FromUserEmailGaiaId(user_email, gaia_id);
      return true;

    case AccountType::ACTIVE_DIRECTORY:
      if (found_gaia_id)
        DLOG(ERROR)
            << "AccountType is 'active directory' but gaia_id is found in '"
            << serialized << "'";

      if (!found_obj_guid) {
        DLOG(ERROR) << "obj_guid is not found in '" << serialized << "'";
        return false;
      }

      if (!found_user_email) {
        DLOG(ERROR) << "user_email is not found in '" << serialized << "'";
      }

      if (!found_obj_guid || !found_user_email)
        return false;

      *account_id = AdFromUserEmailObjGuid(user_email, obj_guid);
      return true;

    case AccountType::UNKNOWN:
      if (!found_user_email)
        return false;
      *account_id = FromUserEmail(user_email);
      return true;
  }
  return false;
}

std::ostream& operator<<(std::ostream& stream, const AccountId& account_id) {
  stream << "{id: " << account_id.id_ << ", email: " << account_id.user_email_
         << ", type: "
         << static_cast<
                std::underlying_type<decltype(account_id.account_type_)>::type>(
                account_id.account_type_)
         << "}";
  return stream;
}

const AccountId& EmptyAccountId() {
  return AccountId::EmptyAccountId::GetInstance()->user_id;
}

namespace BASE_HASH_NAMESPACE {

std::size_t hash<AccountId>::operator()(const AccountId& user_id) const {
  return hash<std::string>()(user_id.GetUserEmail());
}

}  // namespace BASE_HASH_NAMESPACE

// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "google_apis/gaia/oauth2_token_service_request.h"

#include <memory>

#include "base/bind.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/memory/ref_counted_delete_on_sequence.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "google_apis/gaia/google_service_auth_error.h"
#include "google_apis/gaia/oauth2_access_token_consumer.h"

OAuth2TokenServiceRequest::TokenServiceProvider::TokenServiceProvider() {
}

OAuth2TokenServiceRequest::TokenServiceProvider::~TokenServiceProvider() {
}

// Core serves as the base class for OAuth2TokenService operations.  Each
// operation should be modeled as a derived type.
//
// Core is used like this:
//
// 1. Constructed on owner thread.
//
// 2. Start() is called on owner thread, which calls StartOnTokenServiceThread()
// on token service thread.
//
// 3. Request is executed.
//
// 4. Stop() is called on owner thread, which calls StopOnTokenServiceThread()
// on token service thread.
//
// 5. Core is destroyed on owner thread.
class OAuth2TokenServiceRequest::Core
    : public base::RefCountedDeleteOnSequence<OAuth2TokenServiceRequest::Core> {
 public:
  // Note the thread where an instance of Core is constructed is referred to as
  // the "owner thread" here.
  Core(OAuth2TokenServiceRequest* owner,
       const scoped_refptr<TokenServiceProvider>& provider);

  // Starts the core.  Must be called on the owner thread.
  void Start();

  // Stops the core.  Must be called on the owner thread.
  void Stop();

  // Returns true if this object has been stopped.  Must be called on the owner
  // thread.
  bool IsStopped() const;

 protected:
  // Core must be destroyed on the owner thread.  If data members must be
  // cleaned up or destroyed on the token service thread, do so in the
  // StopOnTokenServiceThread method.
  virtual ~Core();

  // Called on the token service thread.
  virtual void StartOnTokenServiceThread() = 0;

  // Called on the token service thread.
  virtual void StopOnTokenServiceThread() = 0;

  const base::SingleThreadTaskRunner* token_service_task_runner() const {
    return token_service_task_runner_.get();
  }
  OAuth2TokenService* token_service();
  OAuth2TokenServiceRequest* owner();

  SEQUENCE_CHECKER(sequence_checker_);

 private:
  friend class base::RefCountedDeleteOnSequence<
      OAuth2TokenServiceRequest::Core>;
  friend class base::DeleteHelper<OAuth2TokenServiceRequest::Core>;

  scoped_refptr<base::SingleThreadTaskRunner> token_service_task_runner_;
  OAuth2TokenServiceRequest* owner_;

  // Clear on owner thread.  OAuth2TokenServiceRequest promises to clear its
  // last reference to TokenServiceProvider on the owner thread so the caller
  // can ensure it is destroyed on the owner thread if desired.
  scoped_refptr<TokenServiceProvider> provider_;

  DISALLOW_COPY_AND_ASSIGN(Core);
};

OAuth2TokenServiceRequest::Core::Core(
    OAuth2TokenServiceRequest* owner,
    const scoped_refptr<TokenServiceProvider>& provider)
    : RefCountedDeleteOnSequence<OAuth2TokenServiceRequest::Core>(
          base::SequencedTaskRunnerHandle::Get()),
      owner_(owner),
      provider_(provider) {
  DCHECK(owner_);
  DCHECK(provider_.get());
  token_service_task_runner_ = provider_->GetTokenServiceTaskRunner();
  DCHECK(token_service_task_runner());
}

OAuth2TokenServiceRequest::Core::~Core() {
}

void OAuth2TokenServiceRequest::Core::Start() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  token_service_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&OAuth2TokenServiceRequest::Core::StartOnTokenServiceThread,
                 this));
}

void OAuth2TokenServiceRequest::Core::Stop() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!IsStopped());

  // Detaches |owner_| from this instance so |owner_| will be called back only
  // if |Stop()| has never been called.
  owner_ = NULL;

  // Stop on the token service thread.  RefCountedDeleteOnSequence ensures we
  // will be destroyed on the owner thread.
  token_service_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&OAuth2TokenServiceRequest::Core::StopOnTokenServiceThread,
                 this));
}

bool OAuth2TokenServiceRequest::Core::IsStopped() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return owner_ == NULL;
}

OAuth2TokenService* OAuth2TokenServiceRequest::Core::token_service() {
  DCHECK(token_service_task_runner()->RunsTasksInCurrentSequence());
  return provider_->GetTokenService();
}

OAuth2TokenServiceRequest* OAuth2TokenServiceRequest::Core::owner() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return owner_;
}

namespace {

// An implementation of Core for getting an access token.
class RequestCore : public OAuth2TokenServiceRequest::Core,
                    public OAuth2TokenService::Consumer {
 public:
  RequestCore(OAuth2TokenServiceRequest* owner,
              const scoped_refptr<
                  OAuth2TokenServiceRequest::TokenServiceProvider>& provider,
              OAuth2TokenService::Consumer* consumer,
              const std::string& account_id,
              const OAuth2TokenService::ScopeSet& scopes);

  // OAuth2TokenService::Consumer.  Must be called on the token service thread.
  void OnGetTokenSuccess(const OAuth2TokenService::Request* request,
                         const std::string& access_token,
                         const base::Time& expiration_time) override;
  void OnGetTokenFailure(const OAuth2TokenService::Request* request,
                         const GoogleServiceAuthError& error) override;

 private:
  friend class base::RefCountedThreadSafe<RequestCore>;

  // Must be destroyed on the owner thread.
  ~RequestCore() override;

  // Core implementation.
  void StartOnTokenServiceThread() override;
  void StopOnTokenServiceThread() override;

  void InformOwnerOnGetTokenSuccess(std::string access_token,
                                    base::Time expiration_time);
  void InformOwnerOnGetTokenFailure(GoogleServiceAuthError error);

  OAuth2TokenService::Consumer* const consumer_;
  std::string account_id_;
  OAuth2TokenService::ScopeSet scopes_;

  // OAuth2TokenService request for fetching OAuth2 access token; it should be
  // created, reset and accessed only on the token service thread.
  std::unique_ptr<OAuth2TokenService::Request> request_;

  DISALLOW_COPY_AND_ASSIGN(RequestCore);
};

RequestCore::RequestCore(
    OAuth2TokenServiceRequest* owner,
    const scoped_refptr<OAuth2TokenServiceRequest::TokenServiceProvider>&
        provider,
    OAuth2TokenService::Consumer* consumer,
    const std::string& account_id,
    const OAuth2TokenService::ScopeSet& scopes)
    : OAuth2TokenServiceRequest::Core(owner, provider),
      OAuth2TokenService::Consumer("oauth2_token_service"),
      consumer_(consumer),
      account_id_(account_id),
      scopes_(scopes) {
  DCHECK(consumer_);
  DCHECK(!account_id_.empty());
  DCHECK(!scopes_.empty());
}

RequestCore::~RequestCore() {
}

void RequestCore::StartOnTokenServiceThread() {
  DCHECK(token_service_task_runner()->RunsTasksInCurrentSequence());
  request_ = token_service()->StartRequest(account_id_, scopes_, this);
}

void RequestCore::StopOnTokenServiceThread() {
  DCHECK(token_service_task_runner()->RunsTasksInCurrentSequence());
  request_.reset();
}

void RequestCore::OnGetTokenSuccess(const OAuth2TokenService::Request* request,
                                    const std::string& access_token,
                                    const base::Time& expiration_time) {
  DCHECK(token_service_task_runner()->RunsTasksInCurrentSequence());
  DCHECK_EQ(request_.get(), request);
  owning_task_runner()->PostTask(
      FROM_HERE, base::Bind(&RequestCore::InformOwnerOnGetTokenSuccess, this,
                            access_token, expiration_time));
  request_.reset();
}

void RequestCore::OnGetTokenFailure(const OAuth2TokenService::Request* request,
                                    const GoogleServiceAuthError& error) {
  DCHECK(token_service_task_runner()->RunsTasksInCurrentSequence());
  DCHECK_EQ(request_.get(), request);
  owning_task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&RequestCore::InformOwnerOnGetTokenFailure, this, error));
  request_.reset();
}

void RequestCore::InformOwnerOnGetTokenSuccess(std::string access_token,
                                               base::Time expiration_time) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!IsStopped()) {
    consumer_->OnGetTokenSuccess(owner(), access_token, expiration_time);
  }
}

void RequestCore::InformOwnerOnGetTokenFailure(GoogleServiceAuthError error) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!IsStopped()) {
    consumer_->OnGetTokenFailure(owner(), error);
  }
}

// An implementation of Core for invalidating an access token.
class InvalidateCore : public OAuth2TokenServiceRequest::Core {
 public:
  InvalidateCore(OAuth2TokenServiceRequest* owner,
                 const scoped_refptr<
                     OAuth2TokenServiceRequest::TokenServiceProvider>& provider,
                 const std::string& access_token,
                 const std::string& account_id,
                 const OAuth2TokenService::ScopeSet& scopes);

 private:
  friend class base::RefCountedThreadSafe<InvalidateCore>;

  // Must be destroyed on the owner thread.
  ~InvalidateCore() override;

  // Core implementation.
  void StartOnTokenServiceThread() override;
  void StopOnTokenServiceThread() override;

  std::string access_token_;
  std::string account_id_;
  OAuth2TokenService::ScopeSet scopes_;

  DISALLOW_COPY_AND_ASSIGN(InvalidateCore);
};

InvalidateCore::InvalidateCore(
    OAuth2TokenServiceRequest* owner,
    const scoped_refptr<OAuth2TokenServiceRequest::TokenServiceProvider>&
        provider,
    const std::string& access_token,
    const std::string& account_id,
    const OAuth2TokenService::ScopeSet& scopes)
    : OAuth2TokenServiceRequest::Core(owner, provider),
      access_token_(access_token),
      account_id_(account_id),
      scopes_(scopes) {
  DCHECK(!access_token_.empty());
  DCHECK(!account_id_.empty());
  DCHECK(!scopes.empty());
}

InvalidateCore::~InvalidateCore() {
}

void InvalidateCore::StartOnTokenServiceThread() {
  DCHECK(token_service_task_runner()->RunsTasksInCurrentSequence());
  token_service()->InvalidateAccessToken(account_id_, scopes_, access_token_);
}

void InvalidateCore::StopOnTokenServiceThread() {
  DCHECK(token_service_task_runner()->RunsTasksInCurrentSequence());
  // Nothing to do.
}

}  // namespace

// static
std::unique_ptr<OAuth2TokenServiceRequest>
OAuth2TokenServiceRequest::CreateAndStart(
    const scoped_refptr<TokenServiceProvider>& provider,
    const std::string& account_id,
    const OAuth2TokenService::ScopeSet& scopes,
    OAuth2TokenService::Consumer* consumer) {
  std::unique_ptr<OAuth2TokenServiceRequest> request(
      new OAuth2TokenServiceRequest(account_id));
  scoped_refptr<Core> core(
      new RequestCore(request.get(), provider, consumer, account_id, scopes));
  request->StartWithCore(core);
  return request;
}

// static
void OAuth2TokenServiceRequest::InvalidateToken(
    const scoped_refptr<TokenServiceProvider>& provider,
    const std::string& account_id,
    const OAuth2TokenService::ScopeSet& scopes,
    const std::string& access_token) {
  std::unique_ptr<OAuth2TokenServiceRequest> request(
      new OAuth2TokenServiceRequest(account_id));
  scoped_refptr<Core> core(new InvalidateCore(
      request.get(), provider, access_token, account_id, scopes));
  request->StartWithCore(core);
}

OAuth2TokenServiceRequest::~OAuth2TokenServiceRequest() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  core_->Stop();
}

std::string OAuth2TokenServiceRequest::GetAccountId() const {
  return account_id_;
}

OAuth2TokenServiceRequest::OAuth2TokenServiceRequest(
    const std::string& account_id)
    : account_id_(account_id) {
  DCHECK(!account_id_.empty());
}

void OAuth2TokenServiceRequest::StartWithCore(const scoped_refptr<Core>& core) {
  DCHECK(core.get());
  core_ = core;
  core_->Start();
}

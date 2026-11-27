#include "net/http/connection_auth_reference_test_mechanism.h"

#include <utility>
#include "base/check.h"
#include "base/functional/bind.h"
#include "base/task/single_thread_task_runner.h"
#include "net/base/net_errors.h"
#include "net/http/connection_auth_original_rounds.h"
#include "net/http/http_auth_challenge_tokenizer.h"

namespace net {
ConnectionAuthReferenceTestMechanism::ConnectionAuthReferenceTestMechanism(
    ConnectionAuthReferenceTestState* state) : state_(state) { ++state_->created; }
ConnectionAuthReferenceTestMechanism::~ConnectionAuthReferenceTestMechanism() {
  ++state_->destroyed;
}
bool ConnectionAuthReferenceTestMechanism::Init(const NetLogWithSource&) { return true; }
bool ConnectionAuthReferenceTestMechanism::NeedsIdentity() const { return !sent_; }
bool ConnectionAuthReferenceTestMechanism::AllowsExplicitCredentials() const { return true; }
HttpAuth::AuthorizationResult ConnectionAuthReferenceTestMechanism::ParseChallenge(
    HttpAuthChallengeTokenizer* challenge) {
  if (!sent_)
    return connection_auth_original_rounds::First(HttpAuth::AUTH_SCHEME_NEGOTIATE,
                                                  *challenge);
  std::string encoded;
  return connection_auth_original_rounds::Later(HttpAuth::AUTH_SCHEME_NEGOTIATE,
                                                 *challenge, &encoded, &decoded_);
}
int ConnectionAuthReferenceTestMechanism::GenerateAuthToken(
    const AuthCredentials*, const std::string&, const std::string&,
    std::string* output, const NetLogWithSource&, CompletionOnceCallback callback) {
  CHECK(callback_.is_null());
  output_ = output;
  callback_ = std::move(callback);
  ++state_->posted;
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&ConnectionAuthReferenceTestMechanism::Complete,
                                weak_factory_.GetWeakPtr()));
  return ERR_IO_PENDING;
}
void ConnectionAuthReferenceTestMechanism::Complete() {
  *output_ = "Negotiate cmVmZXJlbmNl";
  output_ = nullptr;
  sent_ = true;
  ++state_->completed;
  auto callback = std::move(callback_);
  std::move(callback).Run(OK);  // The callback can destroy this mechanism.
}
void ConnectionAuthReferenceTestMechanism::SetDelegation(HttpAuth::DelegationType) {}
std::unique_ptr<HttpAuthMechanism> MakeConnectionAuthReferenceTestMechanism(
    ConnectionAuthReferenceTestState* state, const HttpAuthPreferences*) {
  return std::make_unique<ConnectionAuthReferenceTestMechanism>(state);
}
}  // namespace net

// A test-only, explicitly named reference injection. Not an OS provider.
#ifndef NET_HTTP_CONNECTION_AUTH_REFERENCE_TEST_MECHANISM_H_
#define NET_HTTP_CONNECTION_AUTH_REFERENCE_TEST_MECHANISM_H_
#include <memory>
#include <string>
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "net/http/http_auth_mechanism.h"

namespace net {
struct ConnectionAuthReferenceTestState {
  unsigned created = 0;
  unsigned destroyed = 0;
  unsigned posted = 0;
  unsigned completed = 0;
};
class ConnectionAuthReferenceTestMechanism final : public HttpAuthMechanism {
 public:
  explicit ConnectionAuthReferenceTestMechanism(ConnectionAuthReferenceTestState*);
  ~ConnectionAuthReferenceTestMechanism() override;
  bool Init(const NetLogWithSource&) override;
  bool NeedsIdentity() const override;
  bool AllowsExplicitCredentials() const override;
  HttpAuth::AuthorizationResult ParseChallenge(HttpAuthChallengeTokenizer*) override;
  int GenerateAuthToken(const AuthCredentials*, const std::string&,
                        const std::string&, std::string*, const NetLogWithSource&,
                        CompletionOnceCallback) override;
  void SetDelegation(HttpAuth::DelegationType) override;
 private:
  void Complete();
  raw_ptr<ConnectionAuthReferenceTestState> state_;
  raw_ptr<std::string> output_ = nullptr;
  CompletionOnceCallback callback_;
  bool sent_ = false;
  std::string decoded_;
  base::WeakPtrFactory<ConnectionAuthReferenceTestMechanism> weak_factory_{this};
};
std::unique_ptr<HttpAuthMechanism> MakeConnectionAuthReferenceTestMechanism(
    ConnectionAuthReferenceTestState*, const HttpAuthPreferences*);
}  // namespace net
#endif

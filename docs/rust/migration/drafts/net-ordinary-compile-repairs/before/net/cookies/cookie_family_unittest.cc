// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
// Prepared ordinary native tests; not compiled/executed in the draft phase.
#include "net/cookies/cookie_family_test_support.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include <streambuf>
#include <stdexcept>
namespace net::cookie_family_test {
namespace {
template<class Family> class ReentrantCookie:public Family::Base {
  using Base=typename Family::Base;
 public:
  ReentrantCookie(bool secure,typename Family::Options* options,typename Family::Params* params)
      :Base("name","example.com","/",base::Time::FromInternalValue(13380163200000000LL),secure,false,
          static_cast<typename Family::SameSite>(secure?2:0),std::nullopt,static_cast<typename Family::SourceScheme>(2),443),
       options_(options),params_(params) {}
 protected:
  base::TimeDelta GetLaxAllowUnsafeThresholdAge() const override {
    auto* self=const_cast<ReentrantCookie*>(this);
    ReentrantCookie replacement(false,nullptr,nullptr);
    self->Base::operator=(replacement);
    options_->set_same_site_cookie_context(typename Family::Options::SameSiteCookieContext());
    params_->access_semantics=static_cast<typename Family::AccessSemantics>(1);
    params_->scope_semantics=static_cast<typename Family::ScopeSemantics>(1);
    return base::TimeDelta::Min();
  }
 private:
  raw_ptr<typename Family::Options> options_;
  raw_ptr<typename Family::Params> params_;
};
template<class Family> std::string ReentryDigest() {
  auto options=Family::Options::MakeAllInclusive();auto params=Family::AccessParams();
  ReentrantCookie<Family> cookie(true,&options,&params);
  return AccessDigest(cookie.IncludeForRequestURL(GURL("https://example.com/"),options,params));
}
}
TEST(CookieFamilyTest, WholePipelineMatchesIndependentOriginal) {
  for(const auto& input:kCases) {
    SCOPED_TRACE(input.id);
    EXPECT_EQ(Pipeline<Production>(input,1469598103934665603ULL),Pipeline<Original>(input,1469598103934665603ULL));
  }
}
TEST(CookieFamilyTest, CompleteExpirationAndDomainHelperEdges) {
  for(const auto& text:{"Wed, 09 Jun 2027 10:18:14 GMT","01 Jan 99999 00:00:00","01 Jan 1600 00:00:00",
      "09 Jun 21 10:18:14","09 Jun 70 10:18:14","09 Jun 2027 4294967295:0:0","bad date"}) {
    EXPECT_EQ(cookie_util::ParseCookieExpirationTime(text),original::cookie_util::ParseCookieExpirationTime(text));
  }
  for(const auto& text:{"", ".example.com", "EXAMPLE.COM", "other.test", "..example.com", "%65xample.com"}) {
    CookieInclusionStatus a;original::CookieInclusionStatus b;
    auto left=cookie_util::GetCookieDomainWithString(GURL("https://example.com/a"),text,a);
    auto right=original::cookie_util::GetCookieDomainWithString(GURL("https://example.com/a"),text,b);
    EXPECT_EQ(left,right);EXPECT_EQ(a.GetDebugString(),b.GetDebugString());
  }
}
TEST(CookieFamilyTest, VirtualAgeHookObservesOriginalAliasReadOrder) {
  EXPECT_EQ(ReentryDigest<Production>(),ReentryDigest<Original>());
}
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
namespace {
class ThrowingBuffer:public std::streambuf {
 public:
  std::vector<std::string> writes;
 protected:
  std::streamsize xsputn(const char* text,std::streamsize size) override {
    writes.emplace_back(text,static_cast<size_t>(size));
    if(writes.size()==3) throw std::runtime_error("stream boundary");
    return size;
  }
};
template<class Family> auto CollectionFailure() {
  ThrowingBuffer buffer;std::ostream stream(&buffer);stream.exceptions(std::ios::badbit);
  auto keys=Family::Collection::MatchesSite(typename Family::Site(GURL("https://example.com")));
  bool caught=false;
  try {stream<<keys;}
  catch(const std::runtime_error& error) {caught=std::string(error.what())=="stream boundary";}
  return std::make_tuple(caught,stream.rdstate(),buffer.writes);
}
}
TEST(CookieFamilyTest, StreamFailureStopsNativeEffectsAndRethrowsOutsideRust) {
  EXPECT_EQ(CollectionFailure<Production>(),CollectionFailure<Original>());
}
#endif
namespace {
template <class Family>
class TerminalHookCookie : public Family::Base {
 public:
  explicit TerminalHookCookie(base::OnceClosure hook)
      : Family::Base("name", "example.com", "/",
                     base::Time::FromInternalValue(13380163200000000LL),
                     true, false, Family::SameSite::NO_RESTRICTION,
                     std::nullopt, static_cast<typename Family::SourceScheme>(2),
                     443),
        hook_(std::move(hook)) {}
  ~TerminalHookCookie() override;

 private:
  void PostIncludeForRequestURL(
      const typename Family::Access&,
      const typename Family::Options&,
      typename Family::Options::SameSiteCookieContext::ContextType) const override {
    InvokeHook();
  }
  void PostIsSetPermittedInContext(
      const typename Family::Access&,
      const typename Family::Options&) const override {
    InvokeHook();
  }
  void InvokeHook() const {
    auto callback = std::move(hook_);
    std::move(callback).Run();
  }
  mutable base::OnceClosure hook_;
};

template <class Family>
TerminalHookCookie<Family>::~TerminalHookCookie() = default;

template <class Family>
std::string TerminalHookResult(bool setting) {
  std::unique_ptr<TerminalHookCookie<Family>> cookie;
  int callbacks = 0;
  cookie = std::make_unique<TerminalHookCookie<Family>>(base::BindOnce([&] {
    ++callbacks;
    cookie.reset();
  }));
  auto options = Family::Options::MakeAllInclusive();
  const auto params = Family::AccessParams();
  const GURL url("https://example.com/");
  auto* receiver = cookie.get();
  const auto result = setting
      ? receiver->IsSetPermittedInContext(url, options, params, {"https"})
      : receiver->IncludeForRequestURL(url, options, params);
  CHECK(!cookie);
  CHECK_EQ(callbacks, 1);
  return AccessDigest(result);
}
}  // namespace

TEST(CookieFamilyTest, TerminalHooksMayDeleteReceiverAndReturnOwnedResult) {
  for (bool setting : {false, true}) {
    EXPECT_EQ(TerminalHookResult<Production>(setting),
              TerminalHookResult<Original>(setting));
  }
}
}  // namespace net::cookie_family_test

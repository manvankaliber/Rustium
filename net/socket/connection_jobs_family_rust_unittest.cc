// Source-only ordinary family integration tests; not compiled or executed.
#include "net/socket/connection_jobs_family_rust_test_support.h"
#include "testing/gtest/include/gtest/gtest.h"
namespace net::connection_jobs_family_test {
static_assert(sizeof(Production::Base)==sizeof(Original::Base));
static_assert(alignof(Production::Base)==alignof(Original::Base));
static_assert(sizeof(Production::Transport)==sizeof(Original::Transport));
static_assert(alignof(Production::Transport)==alignof(Original::Transport));
static_assert(sizeof(Production::Tcp)==sizeof(Original::Tcp));
static_assert(alignof(Production::Tcp)==alignof(Original::Tcp));

TEST(ConnectionJobsFamilyRustTest, WholeLifecycleMatchesIndependentOriginal) {
 base::test::TaskEnvironment tasks;
 for(size_t kind=0;kind<std::size(kCases);++kind) {
  SCOPED_TRACE(kCases[kind]);
  base::test::ScopedFeatureList features;
  if(kind>=6&&kind<12) features.InitAndEnableFeature(features::kHappyEyeballsV2);
  else features.InitAndDisableFeature(features::kHappyEyeballsV2);
  Fixture<Original> original;Fixture<Production> rust;
  EXPECT_EQ(::net::connection_jobs_family_test::Run(original,kind,3),::net::connection_jobs_family_test::Run(rust,kind,3));
 }
}
}

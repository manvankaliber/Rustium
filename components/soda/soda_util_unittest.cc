// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/soda/soda_util.h"

#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"


#if (BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_MAC)) && defined(ARCH_CPU_X86_FAMILY)
#include "base/cpu.h"
#endif

namespace speech {

TEST(SodaUtilTest, IsOnDeviceSpeechRecognitionSupported) {
#if BUILDFLAG(IS_LINUX)
#if defined(ARCH_CPU_X86_FAMILY)
  EXPECT_EQ(IsOnDeviceSpeechRecognitionSupported(), base::CPU().has_sse41());
#else
  EXPECT_FALSE(IsOnDeviceSpeechRecognitionSupported());
#endif
#elif BUILDFLAG(IS_MAC)
#if defined(ARCH_CPU_X86_FAMILY)
  EXPECT_EQ(IsOnDeviceSpeechRecognitionSupported(), base::CPU().has_avx());
#elif defined(ARCH_CPU_ARM_FAMILY)
  EXPECT_TRUE(IsOnDeviceSpeechRecognitionSupported());
#else
  EXPECT_FALSE(IsOnDeviceSpeechRecognitionSupported());
#endif
#elif BUILDFLAG(IS_WIN)
  EXPECT_TRUE(IsOnDeviceSpeechRecognitionSupported());
#else
  EXPECT_FALSE(IsOnDeviceSpeechRecognitionSupported());
#endif
}

}  // namespace speech

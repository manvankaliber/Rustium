// Copyright 2011 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/test/python_utils.h"

#include <string>

#include "base/command_line.h"
#include "base/process/launch.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(PythonUtils, Python3RunTime) {
  base::CommandLine cmd_line(base::CommandLine::NO_PROGRAM);
  EXPECT_TRUE(GetPython3Command(&cmd_line));

  // Run a python command to print a string and make sure the output is what
  // we want.
  cmd_line.AppendArg("-c");
  std::string input("PythonUtilsTest");
  std::string python_cmd = base::StringPrintf("print('%s');", input.c_str());
  cmd_line.AppendArg(python_cmd);
  std::string output;
  EXPECT_TRUE(base::GetAppOutput(cmd_line, &output));
  base::TrimWhitespaceASCII(output, base::TRIM_TRAILING, &output);
  EXPECT_EQ(input, output);
}

TEST(PythonUtils, PreservesArguments) {
  base::CommandLine command(base::CommandLine::NO_PROGRAM);
  ASSERT_TRUE(GetPython3Command(&command));
  command.AppendArg("-c");
  command.AppendArg("import sys; print(sys.argv[1], end='')");
  const std::string argument = "spaces \"quotes\" & | ; $ ( ) \\ tail";
  command.AppendArg(argument);
  std::string output;
  ASSERT_TRUE(base::GetAppOutput(command, &output));
  EXPECT_EQ(argument, output);
}

TEST(PythonUtils, PreservesNonzeroExitStatus) {
  base::CommandLine command(base::CommandLine::NO_PROGRAM);
  ASSERT_TRUE(GetPython3Command(&command));
  command.AppendArg("-c");
  command.AppendArg("import sys; print('before exit'); sys.exit(17)");
  std::string output;
  int exit_code = 0;
  ASSERT_TRUE(base::GetAppOutputWithExitCode(command, &output, &exit_code));
  EXPECT_EQ(17, exit_code);
  base::TrimWhitespaceASCII(output, base::TRIM_TRAILING, &output);
  EXPECT_EQ("before exit", output);
}

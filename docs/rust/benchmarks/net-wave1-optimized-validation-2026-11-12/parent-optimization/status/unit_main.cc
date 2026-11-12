#include "testing/gtest/include/gtest/gtest.h"
extern "C" int __llvm_profile_write_file();
int main(int argc,char** argv){::testing::InitGoogleTest(&argc,argv);int r=RUN_ALL_TESTS();__llvm_profile_write_file();return r;}

/*
 * Copyright ©2019 Aaron Johnston.  All rights reserved.  Permission is
 * hereby granted to students registered for University of Washington
 * CSE 333 for use solely during Summer Quarter 2019 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

#include <sys/types.h>
#include <iostream>
#include <sstream>
#include <stdint.h>
#include <string>
#include <unistd.h>

#include "gtest/gtest.h"
extern "C" {
#include "libhw2/filecrawler.h"
#include "libhw2/doctable.h"
#include "libhw2/memindex.h"
}
#include "./fileindexwriter.h"
#include "./test_suite.h"

namespace hw3 {

class Test_FileIndexWriter : public ::testing::Test {
 protected:
  // Code here will be called once for the entire test fixture
  // (ie, before all TEST_Fs).  Note it is a static member function
  // (ie, a class method, not an object instance method).
  static void SetUpTestCase() {
    int res;
    std::cout << "             Crawling ./test_tree/enron_mail..."
              << std::endl;
    res = CrawlFileTree(const_cast<char *>("./test_tree/enron_email"),
                        &Test_FileIndexWriter::dt_,
                        &Test_FileIndexWriter::mi_);
    std::cout << "               ...done crawling." << std::endl;
    ASSERT_NE(0, res);
  }

  // Code here will be called once for the entire test fixture
  // (ie, after all TEST_Fs).  Note it is a static member function
  // (ie, a class method, not an object instance method).
  static void TearDownTestCase() {
    FreeDocTable(dt_);
    FreeMemIndex(mi_);
    dt_ = nullptr;
    mi_ = nullptr;
  }

  // We'll reuse the MemIndex and DocTable across tests.
  static DocTable dt_;
  static MemIndex mi_;
};  // class Test_FileIndexWriter

// statics:
DocTable Test_FileIndexWriter::dt_;
MemIndex Test_FileIndexWriter::mi_;


// Test our ability to write to a file index.
TEST_F(Test_FileIndexWriter, TestFileIndexWrite) {
  uint32_t mypid = (uint32_t) getpid();
  std::stringstream ss;
  ss << "/tmp/test." << mypid << ".index";
  std::string fname = ss.str();

  std::cout << "             " <<
    "Writing index " << mi_ << "out to " << fname << "..." << std::endl;
  HWSize_t res = WriteIndex(mi_, dt_, fname.c_str());
  std::cout << "             " <<
    "...done writing." << std::endl;
  ASSERT_EQ(unlink(fname.c_str()), 0);
  ASSERT_LT((HWSize_t) 100000, res);

  HW3Environment::AddPoints(20);
}

}  // namespace hw3

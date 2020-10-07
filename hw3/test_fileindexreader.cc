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

#include <cstdio>
#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "gtest/gtest.h"
#include "./FileIndexReader.h"
#include "./fileindexutil.h"
#include "./filelayout.h"
#include "./test_suite.h"

namespace hw3 {

class Test_FileIndexReader : public ::testing::Test {
 protected:
  // Code here will be called before each test executes (ie, before
  // each TEST_F).
  virtual void SetUp() {
    // Open up the FILE * for ./unit_test_indices/enron.idx
    fir_ = new FileIndexReader(kIndexName);
  }

  // Code here will be called after each test executes (ie, after
  // each TEST_F)
  virtual void TearDown() {
    delete fir_;
  }

  // These methods proxy our tests' calls to FileIndexReader,
  // allowing tests to access its protected members.
  HWSize_t get_doctable_size() {
    return fir_->header_.doctable_size;
  }
  HWSize_t get_index_size() {
    return fir_->header_.index_size;
  }

  // Objects declared here can be used by all tests in
  // the test case.
  const char* kIndexName = "./unit_test_indices/enron.idx";

  FileIndexReader *fir_;
};  // class Test_FileIndexReader


TEST_F(Test_FileIndexReader, TestFileIndexReaderBasic) {
  // Make sure the header fields line up correctly with the file size.
  struct stat f_stat;
  ASSERT_EQ(stat(kIndexName, &f_stat), 0);
  ASSERT_EQ((HWSize_t) f_stat.st_size,
            (HWSize_t) (get_doctable_size() +
                        get_index_size() +
                        sizeof(IndexFileHeader)));
  HW3Environment::AddPoints(10);

  // Manufacture a DocTableReader.
  DocTableReader dtr = fir_->GetDocTableReader();

  // Try a lookup with it.
  std::string str;
  bool success = dtr.LookupDocID((DocID_t) 10, &str);
  ASSERT_TRUE(success);
  ASSERT_EQ(std::string("test_tree/enron_email/107."), str);

  // Done!
  HW3Environment::AddPoints(20);
}

}  // namespace hw3

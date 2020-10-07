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
#include <list>
#include <unistd.h>

#include "gtest/gtest.h"
extern "C" {
#include "libhw1/HashTable.h"
}
#include "./DocTableReader.h"
#include "./fileindexutil.h"
#include "./fileindexwriter.h"
#include "./filelayout.h"
#include "./test_suite.h"

namespace hw3 {

class Test_DocTableReader : public ::testing::Test {
 protected:
  // Code here will be called before each test executes (ie, before
  // each TEST_F).
  virtual void SetUp() {
    // Open up the FILE * for ./unit_test_indices/enron.idx
    FILE* f = fopen("./unit_test_indices/enron.idx", "rb");
    ASSERT_NE(static_cast<FILE *>(nullptr), f);

    // Prep the DocTableReader; the docid-->docname table is at
    // offset sizeof(IndexFileHeader).
    dtr_ = new DocTableReader(f, sizeof(IndexFileHeader));
  }

  // Code here will be called after each test executes (ie, after
  // each TEST_F)
  virtual void TearDown() {
    delete dtr_;
  }

  // This method proxies our tests' calls to DocTableReader,
  // allowing tests to access its protected members.
  std::list<IndexFileOffset_t> LookupElementPositions(DocID_t hashval) {
    return dtr_->LookupElementPositions(hashval);
  }

  // Objects declared here can be used by all tests in
  // the test case.
  DocTableReader * dtr_;
};  // class Test_DocTableReader


TEST_F(Test_DocTableReader, TestDocTableReaderBasic) {
  // Do a couple of bucket lookups, just to make sure we're
  // inheriting LookupElementPositions correctly.
  auto res = LookupElementPositions(5);
  ASSERT_GT(res.size(), (unsigned int) 0);

  res = LookupElementPositions(6);
  ASSERT_GT(res.size(), (unsigned int) 0);

  // Try some docid-->string lookups.  Start by trying two that
  // should exist.
  std::string str;
  bool success = dtr_->LookupDocID(5, &str);
  ASSERT_TRUE(success);
  ASSERT_EQ(std::string("test_tree/enron_email/102."),
            str);
  success = dtr_->LookupDocID(55, &str);
  ASSERT_TRUE(success);
  ASSERT_EQ(std::string("test_tree/enron_email/149."),
            str);

  // Lookup a docid that shouldn't exist.
  success = dtr_->LookupDocID(100000, &str);
  ASSERT_FALSE(success);

  // Done!
  HW3Environment::AddPoints(30);
}

}  // namespace hw3

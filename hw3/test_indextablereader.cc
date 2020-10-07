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
  #include "libhw1/CSE333.h"
}
#include "./filelayout.h"
#include "./fileindexutil.h"
#include "./fileindexwriter.h"
#include "./IndexTableReader.h"
#include "./test_suite.h"

namespace hw3 {

class Test_IndexTableReader : public ::testing::Test {
 protected:
  // Code here will be called before each test executes (ie, before
  // each TEST_F).
  virtual void SetUp() {
    // Open up the (FILE *) for ./unit_test_indices/enron.idx
    FILE *f = fopen("./unit_test_indices/enron.idx", "rb");
    ASSERT_NE(static_cast<FILE *>(nullptr), f);

    // Read in the size of the doctable.
    ASSERT_EQ(0, fseek(f, DTSIZE_OFFSET, SEEK_SET));
    HWSize_t doctable_size;
    ASSERT_EQ(1U, fread(&doctable_size, 4, 1, f));
    doctable_size = ntohl(doctable_size);

    // Prep the IndexTableReader; the word-->docid_table table is at
    // offset sizeof(IndexFileHeader) + doctable_size.
    itr_ = new IndexTableReader(f, sizeof(IndexFileHeader) + doctable_size);
  }

  // Code here will be called after each test executes (ie, after
  // each TEST_F)
  virtual void TearDown() {
    delete itr_;
  }

  // This method proxies our tests' calls to IndexTableReader,
  // allowing tests to access its protected members.
  std::list<IndexFileOffset_t> LookupElementPositions(HTKey_t hashval) {
    return itr_->LookupElementPositions(hashval);
  }

  // Objects declared here can be used by all tests in
  // the test case.
  IndexTableReader *itr_;
};  // class Test_IndexTableReader


TEST_F(Test_IndexTableReader, TestIndexTableReaderBasic) {
  // Do a couple of bucket lookups to ensure we got data back.
  HTKey_t h1 = FNVHash64((unsigned char *) "anyway", 6);
  auto res = LookupElementPositions(h1);
  ASSERT_GT(res.size(), (unsigned int) 0);

  HTKey_t h2 = FNVHash64((unsigned char *) "attachment", 10);
  res = LookupElementPositions(h2);
  ASSERT_GT(res.size(), (unsigned int) 0);

  // The unit test test_docidtablereader.cc exercises the
  // IndexTableReader's LookupWord() method, so we won't replicate that
  // here.

  // Done!
  HW3Environment::AddPoints(30);
}

}  // namespace hw3

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

#include "./HashTableChecker.h"

#include <iostream>
#include <arpa/inet.h>        // for ntohl().
#include <assert.h>           // for assert().

#include "../fileindexutil.h"
#include "./CheckerUtils.h"

namespace hw3 {

HashTableChecker::HashTableChecker(FILE *f, uint32_t offset, uint32_t len)
  : file_(f), offset_(offset), len_(len) {
}

HashTableChecker::~HashTableChecker() {
  // fclose our (FILE *).
  fclose(file_);
  file_ = NULL;
}

void HashTableChecker::Check(uint32_t offset, uint32_t len) {
  // fread() the number of buckets in this hashtable from its
  // "num_buckets" field. 
  assert(fseek(file_, offset_, SEEK_SET) == 0);
  assert(fread(&numBuckets_, sizeof(numBuckets_), 1, file_) == 1);

  // Convert the num_buckets_ from network (on-disk) representation to
  // host (in-memory) byte ordering using ntohl().
  numBuckets_ = ntohl(numBuckets_);

  // Check that the number of buckets is reasonable.
  CheckLT32(numBuckets_ * kBucketRecordSize, len, "num_buckets < len(table)");

  // Loop through all of the bucket_recs, checking each bucket.
  bool foundFirstNonEmpty = false;
  uint32_t prevChainLen, prevBucket;

  for (uint32_t i = 0; i < numBuckets_; i++) {
    uint32_t curChainLen, curBucket;

    // read the bucket_rec
    assert(fseek(file_, offset_ + kBucketLengthSize + (kBucketRecordSize*i),
                 SEEK_SET) == 0);
    assert(fread(&curChainLen, sizeof(curChainLen), 1, file_) == 1);
    assert(fread(&curBucket, sizeof(curChainLen), 1, file_) == 1);
    curChainLen = ntohl(curChainLen);
    curBucket = ntohl(curBucket);

    // check against prev bucket offset for sanity
    if (foundFirstNonEmpty && (curChainLen > 0)) {
      CheckLT32(prevBucket, curBucket,
                ToString("bucket_rec[%d].position < bucket_rec[%d].position",
			 i, i+1));
      prevChainLen = curChainLen;
      prevBucket = curBucket;
    }

    // if this is the first non-empty bucket, read its offset from the
    // bucket_rec to make sure it lines up with the number of
    // bucket_recs we have.
    if ((curChainLen > 0) && (!foundFirstNonEmpty)) {
      foundFirstNonEmpty = true;

      assert(fseek(file_, offset_ + kBucketLengthSize, SEEK_SET) == 0);
      assert(fread(&prevChainLen, sizeof(prevChainLen), 1, file_) == 1);
      prevChainLen = ntohl(prevChainLen);
      assert(fread(&prevBucket, sizeof(prevBucket), 1, file_) == 1);
      prevBucket = ntohl(prevBucket);

      CheckEQ32(offset_ + kBucketLengthSize + kBucketRecordSize*numBuckets_,
                prevBucket,
                "position of the first non-empty bucket (expected to be "
                "(table start) + (4) + (8*num_buckets))");
    }

    // Seek to the bucket, make sure there are three element position
    // records that make sense.
    uint32_t tableEnd = offset + len;
    CheckLT32(curBucket, tableEnd + 1,
              ToString("bucket_rec[%d].position < table_end+1", i));
    for (int j = 0; j < curChainLen; j++) {
      // read the element position
      uint32_t elementPos;
      assert(fseek(file_, curBucket + j*kElementPositionSize, SEEK_SET) == 0);
      assert(fread(&elementPos, sizeof(elementPos), 1, file_) == 1);
      elementPos = ntohl(elementPos);
      CheckLT32(elementPos, tableEnd + 1,
                ToString("bucket[%d].element[%d].position < table_end+1",
			 i, j));

      // read the element itself
      CheckElement(elementPos, i);
    }
  }  // end loop over all buckets
}

}  // namespace hw3

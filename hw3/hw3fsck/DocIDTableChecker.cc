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

#include "./DocIDTableChecker.h"

#include <assert.h>     // for assert()
#include <arpa/inet.h>  // for ntohl(), etc.
#include <stdint.h>     // for uint32_t, etc.
#include <string>

#include "../fileindexutil.h"
#include "./CheckerUtils.h"

using std::string;

namespace hw3 {

// The constructor for DocIDTableChecker calls the constructor
// of HashTableChecker(), its superclass. The superclass takes
// care of taking ownership of f and using it to extract and
// cache the number of buckets within the table.
DocIDTableChecker::DocIDTableChecker(FILE *f, uint32_t offset, uint32_t len,
                                     const string& context)
  : HashTableChecker(f, offset, len), context_(context) { }

void DocIDTableChecker::CheckElement(uint32_t elementOffset,
                                     uint32_t bucketNumber) {
  // seek to the start of the element
  assert(fseek(file_, elementOffset, SEEK_SET) == 0);

  // read the docID
  uint64_t docID;
  assert(fread(&docID, 8, 1, file_) == 1);
  docID = ntohll(docID);

  // make sure the docID is in the right bucket
  CheckEQ64(bucketNumber, (uint32_t) (docID % numBuckets_),
	    "[DocID table] DocID % numBuckets_ == bucket_number");

  // read in the number of positions
  uint32_t numPos;
  assert(fread(&numPos, 4, 1, file_) == 1);
  numPos = ntohl(numPos);
  CheckLT32(numPos, 1000000, "[DocID table] num_positions");

  // loop through and check the positions
  uint32_t prevPos;
  for (uint32_t i = 0; i < numPos; i++) {
    uint32_t curPos;
    assert(fread(&curPos, 4, 1, file_) == 1);
    curPos = ntohl(curPos);
    if (i > 0) {
      CheckLT32(prevPos,
                curPos,
                "word position[i] < word position[i+1]");
    }
    prevPos = curPos;
  }
}
}  // namespace hw3

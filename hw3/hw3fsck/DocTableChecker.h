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

#ifndef _HW3_HW3FSCK_DOCTABLECHECKER_H_
#define _HW3_HW3FSCK_DOCTABLECHECKER_H_

#include <stdint.h>  // for uint32_t, etc.
#include <cstdio>    // for (FILE *)

#include "../fileindexutil.h"
#include "./HashTableChecker.h"

namespace hw3 {

// A DocTableChecker (a subclass of HashTableChecker) is used to
// read the single docid-->docname "doctable" within the index file.
class DocTableChecker : public HashTableChecker {
 public:
  // Construct a new DocTableChecker at a specified offset within
  // an index file.  Arguments:
  //
  // - f: an open (FILE *) for the underlying index file.  The
  //   constructed  object takes ownership of the (FILE *) and will
  //   fclose() it on destruction.
  //
  // - offset: the "doctable"'s byte offset within the file.
   DocTableChecker(FILE *f, uint32_t offset, uint32_t len);

  // Check a DocTableElement.
  virtual void CheckElement(uint32_t elementOffset,
                            uint32_t bucketNumber);

 private:
  DISALLOW_COPY_AND_ASSIGN(DocTableChecker);
};

}  // namespace hw3

#endif  // _HW3_HW3FSCK_DOCTABLECHECKER_H_

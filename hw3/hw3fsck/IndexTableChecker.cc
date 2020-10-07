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

#include "./IndexTableChecker.h"

#include <assert.h>     // for assert().
#include <arpa/inet.h>  // for ntohl(), ntohs().
#include <cctype>       // for isascii, etc.
#include <stdint.h>     // for uint32_t, etc.
#include <sstream>      // for std::stringstream.
#include <string>       // for std::string.

#include "../fileindexutil.h"
#include "./CheckerUtils.h"
#include "./DocIDTableChecker.h"

using std::string;

namespace hw3 {

// The constructor for IndexTableChecker calls the constructor of
// HashTableChecker(), its superclass. The superclass takes care of
// taking ownership of f and using it to extract and cache the number
// of buckets within the table.
IndexTableChecker::IndexTableChecker(FILE *f, uint32_t offset, uint32_t len)
  : HashTableChecker(f, offset, len) { }

void IndexTableChecker::CheckElement(uint32_t elementOffset,
                                     uint32_t bucketNumber) {
  // Read in the word length.
  uint16_t wordLen;
  assert(fseek(file_, elementOffset, SEEK_SET) == 0);
  assert(fread(&wordLen, sizeof(wordLen), 1, file_) == 1);
  wordLen = ntohs(wordLen);
  CheckLT16(wordLen, 8192,
            ToString("[IndexTable] unreasonably long word in bucket[%d]",
                     bucketNumber));

  // Read in the docID table length.
  uint32_t docIDTableLen;
  assert(fread(&docIDTableLen, sizeof(docIDTableLen), 1, file_) == 1);
  docIDTableLen = ntohl(docIDTableLen);
  CheckLT32(elementOffset + kWordLenSize + kDocIDTableLengthSize + wordLen,
            offset_ + len_,
            ToString("[IndexTable] element_end < indextable_end in bucket[%d]",
                     bucketNumber));

  // Read in the word.
  char word[wordLen+1];
  word[wordLen] = '\0';
  assert(fread(&word[0], wordLen, 1, file_) == 1);

  // Make sure the word is all ascii and lower case.
  for (int i = 0; i < wordLen; i++) {
    uint16_t res = isascii(word[i]) ? 1 : 0;
    CheckEQ16(1, res,
              ToString("[IndexTable] isascii(word)[%d] in bucket[%d]",
                       i, bucketNumber));
    if (!isalpha(word[i]))
      continue;
    res = islower(word[i]) ? 1 : 0;
    CheckEQ16(1, res,
              ToString("[IndexTable] islower(word)[%d] in bucket[%d]",
                     i, bucketNumber));
  }

  DocIDTableChecker ditc(FileDup(file_),
                         elementOffset + kWordLenSize + kDocIDTableLengthSize
                         + wordLen,
                         docIDTableLen,
                         ToString("DocIDtable for word '%s',"
                                  " embedded in bucket[%d]",
                                  word, bucketNumber));
  ditc.Check(elementOffset + kWordLenSize + kDocIDTableLengthSize + wordLen,
             docIDTableLen);
}

}  // namespace hw3

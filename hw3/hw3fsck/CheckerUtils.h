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

#ifndef _HW3_HW3FSCK_FILEINDEXUTIL_H_
#define _HW3_HW3FSCK_FILEINDEXUTIL_H_

#include <stdint.h>     // [C++ doesn't yet standardize <cstdint>.]
#include <arpa/inet.h>  // For htonl(), etc.
#include <unistd.h>     // for dup().
#include <cstdio>       // for fdopen(), (FILE *).
#include <string>       // for string.
#include <sstream>       // for stringstream

// This header file declares a set of useful #defines, macros, utility
// functions, and utility classes that are used broadly within HW3.

namespace hw3 {

// These defines are used to avoid hard-coding the length of the file
// header and the offsets of fields within it.
constexpr int kFileIndexHeaderSize = 16;  // size of header
constexpr int kBucketLengthSize = 4;      // size of the field containing the
                                          // number of bucket records
constexpr int kBucketRecordSize = 8;      // size of a single bucket record
constexpr int kElementPositionSize = 4;   // size of an element position in
                                          //  a bucket chain
constexpr int kMagicNumberOffset = 0;     // offset of magic number
constexpr int kWordLenSize = 2;           // size of the field containing the
                                          //  number of bytes in a word
constexpr int kDocIDTableLengthSize = 4;  // size of the field containing the
                                          //  number of bytes in a docid table


// CheckEQ16 checks a 16-bit field within the index file to compare
// its actual value against an expected value.  If they don't match,
// CheckEQ16 prints a message based on fieldname and optional context.
// As well, CheckEQ16 also tests to see if the field is correct but
// swapped endianness, and adapts the error message if so.
//
// CheckEQ32 and CheckEQ64 are similar, but for 32-bit and 64-bit fields.
void CheckEQ16(uint16_t expected, uint16_t actual,
               const std::string& fieldname);
void CheckEQ32(uint32_t expected, uint32_t actual,
               const std::string& fieldname);
void CheckEQ64(uint64_t expected, uint64_t actual,
               const std::string& fieldname);

// CheckLT16 checks a 16-bit field within the index file to compare
// its actual value against an expected value.  If expected >= actual,
// CheckLT16 prints a message based on fieldname and optional context.
//
// CheckLT32 and CheckLT64 are similar, but for 32-bit and 64-bit fields.
void CheckLT16(uint16_t smaller, uint16_t bigger,
               const std::string& fieldname);
void CheckLT32(uint32_t smaller, uint32_t bigger,
               const std::string& fieldname);
void CheckLT64(uint64_t smaller, uint64_t bigger,
               const std::string& fieldname);

// This helper function uses "varargs" to implement a printf-like interface,
// but for std::string.
std::string ToString(const char* fmt, ...);
}  // namespace hw3

#endif  // _HW3_HW3FSCK_FILEINDEXUTIL_H_

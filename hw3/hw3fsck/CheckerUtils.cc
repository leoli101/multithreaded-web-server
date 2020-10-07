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

#include "./CheckerUtils.h"

#include <cstdarg>
#include <iostream>  // for std::cout / std::endl
#include <string>

#include "../fileindexutil.h"

using std::cout;
using std::endl;
using std::hex;
using std::string;

namespace hw3 {

void CheckEQ16(uint16_t expected, uint16_t actual, const string& fieldname) {
  if (expected == actual)
    return;

  cout << hex;
  cout << fieldname << ": expected " << expected;
  cout << ", but actually is " << actual << ".";
  if (ntohs(actual) == expected) {
    cout << "  Note: you likely ";
    cout << "forgot an endianness conversion, since ntohs(actual) ";
    cout << "== expected.";
  }
  cout << endl;
}

void CheckEQ32(uint32_t expected, uint32_t actual, const string& fieldname) {
  if (expected == actual)
    return;

  cout << hex;
  cout << fieldname << ": expected " << expected;
  cout << ", but actually is " << actual << ".";
  if (ntohl(actual) == expected) {
    cout << "  Note: you likely ";
    cout << "forgot an endianness conversion, since ntohl(actual) ";
    cout << "== expected.";
  }
  cout << endl;
}

void CheckEQ64(uint64_t expected, uint64_t actual, const string& fieldname) {
  if (expected == actual)
    return;

  cout << hex;
  cout << fieldname << ": expected " << expected;
  cout << ", but actually is " << actual << ".";
  if (ntohll(actual) == expected) {
    cout << "  Note: you likely ";
    cout << "forgot an endianness conversion, since ntohll(actual) ";
    cout << "== expected.";
  }
  cout << endl;
}

void CheckLT16(uint16_t smaller, uint16_t bigger, const string& fieldname) {
  if (smaller < bigger)
    return;

  cout << hex;
  cout << fieldname << ": expected " << smaller;
  cout << " < " << bigger << ".";
  if (ntohs(smaller) < bigger) {
    cout << "  Note: there is a chance that you ";
    cout << "forgot an endianness conversion, since ntohs(" << smaller;
    cout << ") = " << ntohs(smaller) << " < " << bigger << ".";
  }
  cout << endl;
}

void CheckLT32(uint32_t smaller, uint32_t bigger, const string& fieldname) {
  if (smaller < bigger)
    return;

  cout << hex;
  cout << fieldname << ": expected " << smaller;
  cout << " < " << bigger << ".";
  if (ntohl(smaller) < bigger) {
    cout << "  Note: it's possible that you ";
    cout << "forgot an endianness conversion, since ntohl(" << smaller;
    cout << ") = " << ntohl(smaller) << " < " << bigger << ".";
  }
  cout << endl;
}

void CheckLT64(uint64_t smaller, uint64_t bigger, const string& fieldname) {
  if (smaller < bigger)
    return;

  cout << hex;
  cout << fieldname << ": expected " << smaller;
  cout << " < " << bigger << ".";
  if (ntohll(smaller) < bigger) {
    cout << "  Note: it's possible that you ";
    cout << "forgot an endianness conversion, since ntohll(" << smaller;
    cout << ") = " << ntohll(smaller) << " < " << bigger << ".";
  }
  cout << endl;
}

string ToString(const char* fmt, ...) {
  constexpr int kBufSize = 512;
  char buf[kBufSize];

  // Initialize the varargs.
  va_list args;
  va_start(args, fmt);

  // Dispatch to the varargs-aware version of printf.
  vsnprintf(buf, kBufSize, fmt, args);

  // Clean up varargs.
  va_end(args);

  return string(buf);
}

}  // namespace hw3

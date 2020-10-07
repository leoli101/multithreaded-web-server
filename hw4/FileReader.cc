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

#include <stdio.h>
#include <string>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <memory>

extern "C" {
  #include "libhw2/fileparser.h"
}

#include "./HttpUtils.h"
#include "./FileReader.h"

namespace hw4 {

bool FileReader::ReadFile(std::string *str) {
  std::string fullfile = basedir_ + "/" + fname_;

  // Read the file into memory, and store the file contents in the
  // output parameter "str."  Be careful to handle binary data
  // correctly; i.e., you probably want to use the two-argument
  // constructor to std::string (the one that includes a length as a
  // second argument).
  //
  // You might find ::ReadFile() from HW2's fileparser.h useful
  // here.  Be careful, though; remember that it uses malloc to
  // allocate memory, so you'll need to use free() to free up that
  // memory.  Alternatively, you can use a unique_ptr with a malloc/free
  // deleter to automatically manage this for you; see the comment in
  // HttpUtils.h above the MallocDeleter class for details.

  // STEP 1: (The only one in this file)
  HWSize_t file_size;
  // Create an unique_ptr to store the content returned
  // from ReadFile().
  std::unique_ptr<char, MallocDeleter<char>> str_ptr(
    ::ReadFile(fullfile.c_str(), &file_size));
  // If the unique_ptr contains a valid ptr,
  // copy the content to a new string and return
  // that string through output parameter.
  if (str_ptr) {
    string ret_str(str_ptr.get(), file_size);
    *str = ret_str;
    return true;
  }
  // unique_ptr contains a non-valid ptr.
  return false;
}

}  // namespace hw4

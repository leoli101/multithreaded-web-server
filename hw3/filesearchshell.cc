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
#include <cstdlib>
#include <iostream>
#include <sstream>  // To use stringstream
#include <algorithm>  // To use transform()

#include "./QueryProcessor.h"

using namespace std;
using namespace hw3;

static void Usage(char *progname) {
  std::cerr << "Usage: " << progname << " [index files+]" << std::endl;
  exit(EXIT_FAILURE);
}

// Your job is to implement the entire filesearchshell.cc
// functionality. We're essentially giving you a blank screen to work
// with; you need to figure out an appropriate design, to decompose
// the problem into multiple functions or classes if that will help,
// to pick good interfaces to those functions/classes, and to make
// sure that you don't leak any memory.
//
// Here are the requirements for a working solution:
//
// The user must be able to run the program using a command like:
//
//   ./filesearchshell ./foo.idx ./bar/baz.idx /tmp/blah.idx [etc]
//
// i.e., to pass a set of filenames of indices as command line
// arguments. Then, your program needs to implement a loop where
// each loop iteration it:
//
//  (a) prints to the console a prompt telling the user to input the
//      next query.
//
//  (b) reads a white-space separated list of query words from
//      std::cin, converts them to lowercase, and constructs
//      a vector of c++ strings out of them.
//
//  (c) uses QueryProcessor.cc/.h's QueryProcessor class to
//      process the query against the indices and get back a set of
//      query results.  Note that you should instantiate a single
//      QueryProcessor  object for the lifetime of the program, rather
//      than  instantiating a new one for every query.
//
//  (d) print the query results to std::cout in the format shown in
//      the transcript on the hw3 web page.
//
// Also, you're required to quit out of the loop when std::cin
// experiences EOF, which a user passes by pressing "control-D"
// on the console.  As well, users should be able to type in an
// arbitrarily long query -- you shouldn't assume anything about
// a maximum line length.  Finally, when you break out of the
// loop and quit the program, you need to make sure you deallocate
// all dynamically allocated memory.  We will be running valgrind
// on your filesearchshell implementation to verify there are no
// leaks or errors.
//
// You might find the following technique useful, but you aren't
// required to use it if you have a different way of getting the
// job done.  To split a std::string into a vector of words, you
// can use a std::stringstream to get the job done and the ">>"
// operator. See, for example, "gnomed"'s post on stackoverflow for
// his example on how to do this:
//
//   http://stackoverflow.com/questions/236129/c-how-to-split-a-string
//
// (Search for "gnomed" on that page.  He uses an istringstream, but
// a stringstream gets the job done too.)
//
// Good luck, and write beautiful code!
int main(int argc, char **argv) {
  if (argc < 2) Usage(argv[0]);

  // Create an list of string to store the index file names.
  list<string> index_list;
  int32_t i;
  for (i = 1; i < argc; i++)
    index_list.push_back(argv[i]);
  // Process the index files using the implemented QueryProcessor.
  QueryProcessor my_qp(index_list, true);
  vector<QueryProcessor::QueryResult> query_results;
  string usr_input;
  while (1) {
    cout << "Enter query:" << endl;
    // Takes in the input string and store it to query.
    getline(cin, usr_input);
    // If user enters EOF, break the infinite loop.
    if (cin.eof())
      break;
    // Use of sstream to break the usr_input into words and
    // store the lower-case words in the vector query.
    string single_word;
    vector<string> query;
    stringstream ss_usr_input(usr_input);
    while (ss_usr_input >> single_word) {
      transform(single_word.begin(),
                single_word.end(),
                single_word.begin(),
                ::tolower);
      query.push_back(single_word);
    }
    // If the query has size of 0, prompt the user about that and
    // move back to the start of loop and prompt user again.
    if (query.size() == 0)
      continue;

    // Call ProcessQuery() to get the list of QueryResult.
    query_results = my_qp.ProcessQuery(query);
    // If the query can not be found, prompt the user.
    if (query_results.size() == 0) {
      cout << "  [no results]" << endl;
    } else {
      // Query is found, print out the file name and the rank.
      for (auto cur_qr : query_results)
        cout << "  " << cur_qr.document_name
        << " " << "(" << cur_qr.rank << ")" << endl;
    }
  }

  return EXIT_SUCCESS;
}

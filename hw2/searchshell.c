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

// Feature test macro for strtok_r (c.f., Linux Programming Interface p. 63)
#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "libhw1/CSE333.h"
#include "memindex.h"
#include "filecrawler.h"
#include <inttypes.h>

static void Usage(void);
// A private function to print the LinkedList of SearchResult.
// It prints the full name of file followed by the rank
// of that file enclosed with parentheses.
//
// Arguments:
//
// -ll: the LinkedList of SearchResult to print
//
// -doctable: DocTable to look up the full name of the file
static void PrintLinkedList(LinkedList ll, DocTable doctable);

int main(int argc, char **argv) {
  if (argc != 2)
    Usage();

  // Implement searchshell!  We're giving you very few hints
  // on how to do it, so you'll need to figure out an appropriate
  // decomposition into functions as well as implementing the
  // functions.  There are several major tasks you need to build:
  //
  //  - Crawl from a directory provided by argv[1] to produce and index
  //  - Prompt the user for a query and read the query from stdin, in a loop
  //  - Split a query into words (check out strtok_r)
  //  - Process a query against the index and print out the results
  //
  // When searchshell detects end-of-file on stdin (cntrl-D from the
  // keyboard), searchshell should free all dynamically allocated
  // memory and any other allocated resources and then exit.
  int res;
  MemIndex index;
  DocTable doctable;
  res = CrawlFileTree(argv[1], &doctable, &index);
  Verify333(res == 1);
  printf("Indexing '%s'\n", argv[1]);
  char usr_input[256];
  while (1) {
    printf("enter query:\n");
    if (fgets(usr_input, sizeof(usr_input), stdin) == NULL)
      break;
    char usr_input_cp[256];
    snprintf(usr_input_cp, strlen(usr_input) + 1, "%s", usr_input);
    // Initialize the rest to point to the start of usr_input.
    char* rest = usr_input_cp;
    int num_split_query = 0;
    while (strtok_r(rest, " ", &rest))
      num_split_query++;
    char* token;
    // reset the rest to point to the start of usr_input.
    rest = usr_input;
    // Allocate space for query which is an array of strings.
    char** query = (char**)malloc(num_split_query * sizeof(char*));
    Verify333(query != NULL);
    // Used for indexing.
    int i = 0;
    // Set the new-line character at the end of usr_input to
    // null terminator.
    usr_input[strlen(usr_input)-1] = '\0';
    // Use function strtok_r to split the words.
    while ((token = strtok_r(rest, " ", &rest))) {
      // Allocate space for each individual word.
      // strlen + 1 since null terminator has to be
      // in the query for further processing.
      query[i] = (char*)malloc((strlen(token) + 1) * sizeof(char));
      Verify333(query[i] != NULL);
      // Copy the token into query[i].
      snprintf(query[i], strlen(token) + 1, "%s", token);
      // Increment the index by one.
      i++;
    }
    // Extract the result list.
    // llres has to manually free later!
    LinkedList llres = MIProcessQuery(index, query, (uint8_t) num_split_query);
    // Only print out the result if the result list
    // is not NULL.
    if (llres != NULL) {
      PrintLinkedList(llres, doctable);
      // Free the LinkedList allocated for llres.
      FreeLinkedList(llres, &free);
    }
    // Clean up the allocated space of each strings inside
    // query and query.
    for (i = 0; i < num_split_query; i++) {
      free(query[i]);
    }
    free(query);
  }
  // Clean up the index and doctable.
  FreeMemIndex(index);
  FreeDocTable(doctable);
  return EXIT_SUCCESS;
}

static void Usage(void) {
  fprintf(stderr, "Usage: ./searchshell <docroot>\n");
  fprintf(stderr,
          "where <docroot> is an absolute or relative " \
          "path to a directory to build an index under.\n");
  exit(EXIT_FAILURE);
}

static void PrintLinkedList(LinkedList ll, DocTable doctable) {
  // Only print the LinkedList if it is not NULL.
  // Allocate an iterator to traverse the LinledList.
  LLIter ll_it = LLMakeIterator(ll, 0);
  Verify333(ll_it != NULL);
  do {
    SearchResultPtr SR_ptr;
    LLIteratorGetPayload(ll_it, (LLPayload_t *)&SR_ptr);
    char* docname = DTLookupDocID(doctable, SR_ptr->docid);
    printf("  %s ", docname);
    printf("(");
    printf("%"PRIu32, SR_ptr->rank);
    printf(")\n");
  } while (LLIteratorNext(ll_it));
  // Free the allocated iterator.
  LLIteratorFree(ll_it);
}

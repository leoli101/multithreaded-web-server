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
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>

#include "libhw1/CSE333.h"
#include "fileparser.h"
#include "memindex.h"

#define ASCII_UPPER_BOUND 0x7F

// A private function for freeing a WordPositions.positions list.
static void NullWPListFree(LLPayload_t payload) { }

// A private function for freeing a WordHT.
static void WordHTFree(HTValue_t payload) {
  WordPositions *pos = (WordPositions *) payload;
  FreeLinkedList(pos->positions, &NullWPListFree);
  free(pos->word);
  free(pos);
}

// A private helper function; once you've parsed out a word from the text
// file, use this helper function to add that word and its position to the
// hash table tab.
static void AddToHashTable(HashTable tab, char *word, DocPositionOffset_t pos);

// A private helper function to parse a file and build up the HashTable of
// WordPosition structures.
static void LoopAndInsert(HashTable tab, char *content);

char *ReadFile(const char *filename, HWSize_t *size) {
  struct stat filestat;
  char *buf;
  int result, fd;
  ssize_t numread;
  size_t left_to_read;

  // STEP 1.
  // Use the stat system call to fetch a "struct stat" that describes
  // properties of the file. ("man 2 stat"). [You can assume we're on a 64-bit
  // system, with a 64-bit off_t field.]
  result = stat(filename, &filestat);
  if (result < 0) {
    fprintf(stderr, "Failed to use stat()");
    return NULL;
  }

  // STEP 2.
  // Make sure this is a "regular file" and not a directory
  // or something else.  (use the S_ISREG macro described
  // in "man 2 stat")

  // Check if this is a regular file, if it is not,
  // retur NULL.
  if (S_ISREG(filestat.st_mode) == 0) {
    fprintf(stderr, "Not a regualr file.");
    return NULL;
  }

  // STEP 3.
  // Attempt to open the file for reading.  (man 2 open)

  // Open the file
  fd = open(filename, O_RDONLY);
  if (fd == -1) {
    fprintf(stderr, "Failed to open the file.");
    return NULL;
  }

  // STEP 4.
  // Allocate space for the file, plus 1 extra byte to
  // NULL-terminate the string.
  buf = (char*)malloc((size_t)filestat.st_size + 1);
  if (buf == NULL) {
    fprintf(stderr, "Failed to allocate memory.");
    return NULL;
  }

  // STEP 5.
  // Read in the file contents.  Use the read system call. (man 2 read.)  Be
  // sure to handle the case that (read returns -1) and errno is (either
  // EAGAIN or EINTR).  Also, note that it is not an error for read to return
  // fewer bytes than what you asked for; you'll need to put read() inside a
  // while loop, looping until you've read to the end of file or a non-
  // recoverable error.  (Read the man page for "read()" carefully, in
  // particular what the return values -1 and 0 imply.)
  left_to_read = filestat.st_size;
  while (left_to_read > 0) {
    // Read the file and store the contents into buf.
    numread = read(fd, buf + filestat.st_size - left_to_read, left_to_read);
    // Read has encountered error.
    if (numread == -1) {
      if (errno != EINTR && errno != EAGAIN) {
        fprintf(stderr, "Fatal error occured while reading.");
        free(buf);
        return NULL;
      }
      continue;
    } else if (numread == 0) {
    // Read has reached end-of-file.
      break;
    }
    // Decrment the left_to_read by number has read.
    left_to_read -= numread;
  }

  // Great, we're done!  We hit the end of the file and we
  // read (filestat.st_size - left_to_read) bytes. Close the
  // file descriptor returned by open(), and return through the
  // "size" output parameter how many bytes we read.
  close(fd);
  *size = (HWSize_t) (filestat.st_size - left_to_read);

  // Add a '\0' after the end of what we read to NULL-terminate
  // the string.
  buf[*size] = '\0';
  return buf;
}

HashTable BuildWordHT(char *filename) {
  char *filecontent;
  HashTable tab;
  HWSize_t filelen, i;

  if (filename == NULL)
    return NULL;

  // STEP 6.
  // Use ReadFile() to slurp in the file contents.  If the
  // file turns out to be empty (i.e., its length is 0),
  // or you couldn't read the file at all, return NULL to indicate
  // failure.
  filecontent = ReadFile(filename, &filelen);
  if (filelen == 0 || filecontent == NULL) {
    fprintf(stderr, "Failed to ReadFile().");
    return NULL;
  }

  // Verify that the file contains only ASCII text.  We won't try to index any
  // files that contain non-ASCII text; unfortunately, this means we aren't
  // Unicode friendly.
  for (i = 0; i < filelen; i++) {
    if ((filecontent[i] == '\0') ||
        ((unsigned char) filecontent[i] > ASCII_UPPER_BOUND)) {
      free(filecontent);
      return NULL;
    }
  }

  // Great!  Let's split the file up into words.  We'll allocate the hash
  // table that will store the WordPositions structures associated with each
  // word.  Since our hash table dynamically grows, we'll start with a small
  // number of buckets.
  tab = AllocateHashTable(64);
  Verify333(tab != NULL);

  // Loop through the file, splitting it into words and inserting a record for
  // each word.
  LoopAndInsert(tab, filecontent);

  // If we found no words, return NULL instead of a
  // zero-sized hashtable.
  if (NumElementsInHashTable(tab) == 0) {
    FreeHashTable(tab, &WordHTFree);
    tab = NULL;
  }

  // Now that we've finished parsing the document, we can free up the
  // filecontent buffer and return our built-up table.
  free(filecontent);
  filecontent = NULL;
  return tab;
}

void FreeWordHT(HashTable table) {
  FreeHashTable(table, &WordHTFree);
}

static void LoopAndInsert(HashTable tab, char *content) {
  char *curptr = content, *wordstart = content;

  // STEP 7.
  // This is the interesting part of Part A!
  //
  // "content" contains a C string with the full contents
  // of the file.  You need to implement a loop that steps through the
  // file content  a character at a time, testing to see whether a
  // character is an alphabetic character or not.  If a character is
  // alphabetic, it's part of a word.  If a character is not
  // alphabetic, it's part of the boundary between words.
  // You can use the string.h "isalpha()" macro to test whether
  // a character is alphabetic or not.  ("man isalpha").
  //
  // So, for example, here's a string with the words within
  // it underlined with "=", and boundary characters underlined
  // with "+":
  //
  // The  Fox  Can't   CATCH the  Chicken.
  // ===++===++===+=+++=====+===++=======+
  //
  // As you loop through, anytime you detect the start of a
  // word, you should use the "wordstart" pointer to remember
  // where the word started.  You should also use the "tolower"
  // macro to convert alphabetic characters to lowercase.
  // (e.g.,  *curptr = tolower(*curptr);  ).  Finally, as
  // a hint, you can overwrite boundary characters with '\0' (null
  // terminators) in place in the buffer to create valid C
  // strings out of each parsed word.
  //
  // Each time you find a word that you want to record in
  // the hashtable, call the AddToHashTable() helper
  // function with appropriate arguments, e.g.,
  //
  //    AddToHashTable(tab, wordstart, pos);
  //

  // A flag indicating whether it is parsing_a_word or not.
  int parsing_a_word;
  // Initialize the flag by check if the start of content is
  // an alphabet or not.
  parsing_a_word = isalpha(*content) ? 1 : 0;
  while (1) {
    // Break when it has reached the end-of-file.
    if (*curptr == '\0')
      break;
    // In the process of parsing a word. The wordstart has
    // been set, just need to find the boundary. Once the
    // boundary is found, set it to '\0' and add the word
    // to hashtable and change the flag to 0.
    if (parsing_a_word == 1) {
      if (!isalpha(*curptr)) {
        *curptr = '\0';
        AddToHashTable(tab, wordstart, (DocPositionOffset_t)
           ((intptr_t)(wordstart - content)));
        parsing_a_word = 0;
      }
    } else {
    // Find a new word now. The wordstart is not valid.
    // Once curptr points to an alphabet, set wordstart to
    // point to that alphabet and set the flag to 0.
      if (isalpha(*curptr)) {
        wordstart = curptr;
        parsing_a_word = 1;
      }
    }
    // If curptr points to an alphabet, set the alphabet
    // to lower-case.
    if (isalpha(*curptr))
      *curptr = tolower(*curptr);
    // Increment the curptr by one.
    curptr++;
  }
}


static void AddToHashTable(HashTable tab, char *word, DocPositionOffset_t pos) {
  HTKey_t hashKey;
  int retval;
  HTKeyValue kv;

  // Hash the string.
  hashKey = FNVHash64((unsigned char *) word, strlen(word));

  // Have we already encountered this word within this file?
  // If so, it's already in the hashtable.
  retval = LookupHashTable(tab, hashKey, &kv);
  if (retval == 1) {
    // Yes; we just need to add a position in using AppendLinkedList(). Note
    // how we're casting the DocPositionOffset_t position variable to an
    // LLPayload_t to store it in the linked list payload without needing to
    // malloc space for it. Ugly, but it works!
    WordPositions *wp = (WordPositions *) kv.value;
    retval = AppendLinkedList(wp->positions, (LLPayload_t) ((intptr_t) pos));
    Verify333(retval != 0);
  } else {
    // STEP 8.
    // No; this is the first time we've seen this word.  Allocate and prepare
    // a new WordPositions structure, and append the new position to its list
    // using a similar ugly hack as right above.

    // If LookupHashTable has error.
    if (retval == -1) {
      fprintf(stderr, "Memory occured trying to look up the key.");
      exit(EXIT_FAILURE);
    }
    WordPositions *wp;
    // Allocate space for the word position.
    wp = (WordPositions*)malloc(sizeof(WordPositions));
    if (wp == NULL) {
      fprintf(stderr, "Failed to allocate space for word position.");
      exit(EXIT_FAILURE);
    }
    // Allocate space for the word including the null-terminator.
    char *word_cp = (char*)malloc(strlen(word) + 1);
    if (word_cp == NULL) {
      fprintf(stderr, "Failed to allocate space for word.");
      exit(EXIT_FAILURE);
    }
    // Copy the word into word_cp.
    snprintf(word_cp, strlen(word) + 1, "%s", word);
    // Initialize the WordPositions structure.
    wp->word = word_cp;
    wp->positions = AllocateLinkedList();
    // Add the pos to LinkedList.
    retval = AppendLinkedList(wp->positions, (LLPayload_t) ((intptr_t)pos));
    Verify333(retval != 0);
    HTKeyValue new_keyvalue;
    // Set the new keyvalue fields.
    new_keyvalue.key = hashKey;
    new_keyvalue.value = wp;
    retval = InsertHashTable(tab, new_keyvalue, &kv);
    // Handle the case that InsertHashTable() fails.
    if (retval == 0) {
      FreeLinkedList(wp->positions, &NullWPListFree);
      free(wp);
      fprintf(stderr, "Failed insert new keyvalue.");
      exit(EXIT_FAILURE);
    }
  }
}

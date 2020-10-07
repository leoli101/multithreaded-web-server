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
#include <string.h>

#include "doctable.h"
#include "libhw1/CSE333.h"
#include "libhw1/HashTable.h"

// This structure represents a DocTable; it contains two hash tables, one
// mapping from docid to docname, and one mapping from docname to docid.
typedef struct doctablerecord_st {
  HashTable docid_to_docname;  // mapping docid to docname
  HashTable docname_to_docid;  // mapping docname to docid
  DocID_t max_id;              // max docID allocated so far
} DocTableRecord;

DocTable AllocateDocTable(void) {
  DocTableRecord *dt = (DocTableRecord *) malloc(sizeof(DocTableRecord));
  Verify333(dt != NULL);

  dt->docid_to_docname = AllocateHashTable(1024);
  dt->docname_to_docid = AllocateHashTable(1024);
  dt->max_id = 0;

  Verify333(dt->docid_to_docname != NULL);
  Verify333(dt->docname_to_docid != NULL);
  return dt;
}

void FreeDocTable(DocTable table) {
  Verify333(table != NULL);

  // STEP 1.
  // Free the two hashtables stored. And use
  // free() to free the values stored inside.
  FreeHashTable(table->docid_to_docname, &free);
  FreeHashTable(table->docname_to_docid, &free);
  free(table);
}

HWSize_t DTNumDocsInDocTable(DocTable table) {
  Verify333(table != NULL);
  return NumElementsInHashTable(table->docid_to_docname);
}

DocID_t DTRegisterDocumentName(DocTable table, char *docname) {
  // Allocate space for the docid.
  char       *doccopy;
  DocID_t   *docid = (DocID_t *) malloc(sizeof(DocID_t));
  HTKeyValue kv, oldkv;

  // Make a copy of the docname.
  doccopy = (char *) malloc(1+strlen(docname));
  if (doccopy != NULL) {
    strncpy(doccopy, docname, 1+strlen(docname));
  }
  Verify333(table != NULL);
  Verify333(doccopy != NULL);
  Verify333(docid != NULL);

  // Check to see if the document already exists; if so,
  // free up the malloc'ed space and return the existing docid

  // STEP 2.
  // Get the hashkey of the docname and store it in
  // kv as they key.
  kv.key = FNVHash64((unsigned char*)docname, strlen(docname));
  // Lookup the key in the docname_to_docid hashtable.
  int ret = LookupHashTable(table->docname_to_docid, kv.key, &oldkv);
  // If they key is already in the hashtable, free the
  // doccopy and docid allocated earlier.
  if (ret == 1) {
    free(doccopy);
    free(docid);
    return *(DocID_t*)oldkv.value;
  }

  // allocate the next docID
  table->max_id += 1;
  *docid = table->max_id;

  // STEP 3.
  // Set up the key/value for the docid_to_docname mapping, and
  // do the insert.
  HTKeyValue kv2;
  kv2.key = *docid;
  kv2.value = doccopy;
  ret = InsertHashTable(DTGetDocidTable(table), kv2, &oldkv);
  Verify333(ret == 1);

  // STEP 4.
  // Set up the key/value for the docname_to_docid mapping, and
  // do the insert.
  kv.value = docid;
  ret = InsertHashTable(table->docname_to_docid, kv, &oldkv);
  Verify333(ret == 1);

  return *docid;
}

DocID_t DTLookupDocumentName(DocTable table, char *docname) {
  HTKey_t key;
  HTKeyValue kv;
  int res;

  Verify333(table != NULL);
  Verify333(docname != NULL);

  // STEP 5.
  // Lookup the FNVHash64() or the docname in the
  // docname_to_docid table within dt, and return
  // either "0" if the docname isn't found or the
  // docID if it is.
  key = FNVHash64((unsigned char*)docname, strlen(docname));
  res = LookupHashTable(table->docname_to_docid, key, &kv);
  if (res == 0)
    return 0;
  return *(DocID_t*)kv.value;
}

char *DTLookupDocID(DocTable table, DocID_t docid) {
  HTKeyValue kv;
  int res;

  Verify333(table != NULL);
  Verify333(docid != 0);

  // STEP 6.
  // Lookup the docid in the docid_to_docname table,
  // and either return the string (i.e., the (char *)
  // saved in the value field for that key) or
  // NULL if the key isn't in the table.
  res = LookupHashTable(DTGetDocidTable(table), docid, &kv);
  if (res == 0)
    return NULL;
  return (char*)kv.value;
}

HashTable DTGetDocidTable(DocTable table) {
  Verify333(table != NULL);
  return table->docid_to_docname;
}

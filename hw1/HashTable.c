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
#include <stdint.h>

#include "CSE333.h"
#include "HashTable.h"
#include "HashTable_priv.h"

// A private utility function to grow the hashtable (increase
// the number of buckets) if its load factor has become too high.
static void ResizeHashtable(HashTable ht);

// a free function that does nothing
static void LLNullFree(LLPayload_t freeme) { }
static void HTNullFree(HTValue_t freeme) { }

HashTable AllocateHashTable(HWSize_t num_buckets) {
  HashTable ht;
  HWSize_t  i;

  // defensive programming
  if (num_buckets == 0) {
    return NULL;
  }

  // allocate the hash table record
  ht = (HashTable) malloc(sizeof(HashTableRecord));
  if (ht == NULL) {
    return NULL;
  }

  // initialize the record
  ht->num_buckets = num_buckets;
  ht->num_elements = 0;
  ht->buckets =
    (LinkedList *) malloc(num_buckets * sizeof(LinkedList));
  if (ht->buckets == NULL) {
    // make sure we don't leak!
    free(ht);
    return NULL;
  }
  for (i = 0; i < num_buckets; i++) {
    ht->buckets[i] = AllocateLinkedList();
    if (ht->buckets[i] == NULL) {
      // allocating one of our bucket chain lists failed,
      // so we need to free everything we allocated so far
      // before returning NULL to indicate failure.  Since
      // we know the chains are empty, we'll pass in a
      // free function pointer that does nothing; it should
      // never be called.
      HWSize_t j;
      for (j = 0; j < i; j++) {
        FreeLinkedList(ht->buckets[j], LLNullFree);
      }
      free(ht->buckets);
      free(ht);
      return NULL;
    }
  }

  return (HashTable) ht;
}

void FreeHashTable(HashTable table,
                   ValueFreeFnPtr value_free_function) {
  HWSize_t i;

  Verify333(table != NULL);  // be defensive

  // loop through and free the chains on each bucket
  for (i = 0; i < table->num_buckets; i++) {
    LinkedList  bl = table->buckets[i];
    HTKeyValue *nextKV;

    // pop elements off the the chain list, then free the list
    while (NumElementsInLinkedList(bl) > 0) {
      Verify333(PopLinkedList(bl, (LLPayload_t*)&nextKV));
      value_free_function(nextKV->value);
      free(nextKV);
    }
    // the chain list is empty, so we can pass in the
    // null free function to FreeLinkedList.
    FreeLinkedList(bl, LLNullFree);
  }

  // free the bucket array within the table record,
  // then free the table record itself.
  free(table->buckets);
  free(table);
}

HWSize_t NumElementsInHashTable(HashTable table) {
  Verify333(table != NULL);
  return table->num_elements;
}

HTKey_t FNVHash64(unsigned char *buffer, HWSize_t len) {
  // This code is adapted from code by Landon Curt Noll
  // and Bonelli Nicola:
  //
  // http://code.google.com/p/nicola-bonelli-repo/
  static const uint64_t FNV1_64_INIT = 0xcbf29ce484222325ULL;
  static const uint64_t FNV_64_PRIME = 0x100000001b3ULL;
  unsigned char *bp = (unsigned char *) buffer;
  unsigned char *be = bp + len;
  uint64_t hval = FNV1_64_INIT;

  /*
   * FNV-1a hash each octet of the buffer
   */
  while (bp < be) {
    /* xor the bottom with the current octet */
    hval ^= (uint64_t) * bp++;
    /* multiply by the 64 bit FNV magic prime mod 2^64 */
    hval *= FNV_64_PRIME;
  }
  /* return our new hash value */
  return hval;
}

HTKey_t FNVHashInt64(HTValue_t hashval) {
  unsigned char buf[8];
  int i;
  uint64_t hashme = (uint64_t)hashval;

  for (i = 0; i < 8; i++) {
    buf[i] = (unsigned char) (hashme & 0x00000000000000FFULL);
    hashme >>= 8;
  }
  return FNVHash64(buf, 8);
}

HWSize_t HashKeyToBucketNum(HashTable ht, HTKey_t key) {
  return key % ht->num_buckets;
}

// helper function
static bool ChainContainsKey(LLIter iter
  , HTKey_t newkey, HTKeyValue **oldkeyvalue) {
  Verify333(iter != NULL);
  while (1) {
    LLIteratorGetPayload(iter, (LLPayload_t*)oldkeyvalue);
    if ((*oldkeyvalue)->key == newkey) {
      return true;
    }
    if (!LLIteratorNext(iter))
      break;
  }
  return false;
}

int InsertHashTable(HashTable table,
                    HTKeyValue newkeyvalue,
                    HTKeyValue *oldkeyvalue) {
  HWSize_t insertbucket;
  LinkedList insertchain;

  Verify333(table != NULL);
  ResizeHashtable(table);

  // calculate which bucket we're inserting into,
  // grab its linked list chain
  insertbucket = HashKeyToBucketNum(table, newkeyvalue.key);
  insertchain = table->buckets[insertbucket];

  // Step 1 -- finish the implementation of InsertHashTable.
  // This is a fairly complex task, so you might decide you want
  // to define/implement a helper function that helps you find
  // and optionally remove a key within a chain, rather than putting
  // all that logic inside here.  You might also find that your helper
  // can be reused in steps 2 and 3.

  // Allocate space for a new key value pair.
  HTKeyValue *new_pair = (HTKeyValue*)malloc(sizeof(HTKeyValue));
  if (new_pair == NULL)
    return 0;
  *new_pair = newkeyvalue;

  // Check if the list is empty. If it is,
  // simply add the keyvalue to the list.
  if (NumElementsInLinkedList(insertchain) == 0) {
    if (!PushLinkedList(insertchain, (LLPayload_t)new_pair)) {
      // Free the space allocated for new_pair
      free(new_pair);
      return 0;  // out of memory
    }
    table->num_elements += 1U;  // increase the table size
    return 1;  // successfully inserted
  }

  // Create an iterator for the insertchain.
  // Starting from the head.
  LLIter chain_iterator = LLMakeIterator(insertchain, 0);
  if (chain_iterator == NULL) {
    // Free the space allocated for new_pair
    free(new_pair);
    return 0;
  }

  HTKeyValue *old_pair;
  // the key is already in the chain
  if (ChainContainsKey(chain_iterator, newkeyvalue.key, &old_pair)) {
    oldkeyvalue->key = old_pair->key;
    oldkeyvalue->value = old_pair->value;
    // Free the space allocated for old keyvalue
    free(old_pair);
    // Delete the current keyvalue iterator points to
    LLIteratorDelete(chain_iterator, &LLNullFree);
    // No need to check if there is enough space since old
    // pair space was just freed
    // Add the new keyvalue
    PushLinkedList(insertchain, (LLPayload_t)new_pair);
    LLIteratorFree(chain_iterator);  // Free the iterator
    return 2;
  }
  LLIteratorFree(chain_iterator);  // Free the iterator
  if (!PushLinkedList(insertchain, (LLPayload_t) new_pair)) {
    free(new_pair);
    return 0;  // Out of memory
  }
  table->num_elements += 1U;  // Increase the table size
  return 1;  // Successfully inserted
}

int LookupHashTable(HashTable table,
                    HTKey_t key,
                    HTKeyValue *keyvalue) {
  Verify333(table != NULL);

  // Step 2 -- implement LookupHashTable.
  HWSize_t bucket_to_check = HashKeyToBucketNum(table, key);
  LinkedList check_chain = table->buckets[bucket_to_check];
  if (NumElementsInLinkedList(check_chain) == 0) {
    return 0;  // key is not found
  }
  // Create the iterator for current bucket starting from head.
  LLIter chain_iterator = LLMakeIterator(check_chain, 0);
  if (chain_iterator == NULL)
    return -1;  // run out of memory
  HTKeyValue *cur_key_value_ptr;
  // Find the given key from given bucket. If the key is found
  // copy the keyvalue to the return parameter.
  if (ChainContainsKey(chain_iterator, key, &cur_key_value_ptr)) {
    keyvalue->key = cur_key_value_ptr->key;
    keyvalue->value = cur_key_value_ptr->value;
    // Free the iterator for linkedlist
    LLIteratorFree(chain_iterator);
    return 1;
  }
  // Free the iterator for linkedlist
  LLIteratorFree(chain_iterator);
  return 0;
}

int RemoveFromHashTable(HashTable table,
                        HTKey_t key,
                        HTKeyValue *keyvalue) {
  Verify333(table != NULL);

  // Step 3 -- implement RemoveFromHashTable.
  HWSize_t bucket_to_check = HashKeyToBucketNum(table, key);
  LinkedList check_chain = table->buckets[bucket_to_check];
  if (NumElementsInLinkedList(check_chain) == 0) {
    return 0;  // key is not found
  }
  // Create an iterator of the LinkedList
  // starting from the head of the list
  LLIter chain_iterator = LLMakeIterator(check_chain, 0);
  if (chain_iterator == NULL)
    return -1;  // run out of memory
  HTKeyValue *keyvalue_to_return;
  // If the bucket contains given key, copy the old keyvalue
  // and remove that old keyvalue from bucket
  if (ChainContainsKey(chain_iterator, key, &keyvalue_to_return)) {
    keyvalue->key = keyvalue_to_return->key;
    keyvalue->value = keyvalue_to_return->value;
    free(keyvalue_to_return);  // Free the keyvalue(payload)
    // Delete the node containing the keyvalue
    LLIteratorDelete(chain_iterator, &LLNullFree);
    // Decrement the size of table
    table->num_elements -= 1U;
    // Free the space allocated for chain_iterator
    LLIteratorFree(chain_iterator);
    return 1;
  }
  // Free the space allocated for chain_iterator
  LLIteratorFree(chain_iterator);
  return 0;
}

HTIter HashTableMakeIterator(HashTable table) {
  HTIterRecord *iter;
  HWSize_t      i;

  Verify333(table != NULL);  // be defensive

  // malloc the iterator
  iter = (HTIterRecord *) malloc(sizeof(HTIterRecord));
  if (iter == NULL) {
    fprintf(stderr, "Failed to allocate spacce for HTIterator!");
    return NULL;
  }

  // if the hash table is empty, the iterator is immediately invalid,
  // since it can't point to anything.
  if (table->num_elements == 0) {
    iter->is_valid = false;
    iter->ht = table;
    iter->bucket_it = NULL;
    return iter;
  }

  // initialize the iterator.  there is at least one element in the
  // table, so find the first element and point the iterator at it.
  iter->is_valid = true;
  iter->ht = table;
  for (i = 0; i < table->num_buckets; i++) {
    if (NumElementsInLinkedList(table->buckets[i]) > 0) {
      iter->bucket_num = i;
      break;
    }
  }
  Verify333(i < table->num_buckets);  // make sure we found it.
  iter->bucket_it = LLMakeIterator(table->buckets[iter->bucket_num], 0UL);
  if (iter->bucket_it == NULL) {
    // out of memory!
    free(iter);
    fprintf(stderr, "Failed to allocate space for LLIterator!");
    return NULL;
  }
  return iter;
}

void HTIteratorFree(HTIter iter) {
  Verify333(iter != NULL);
  // If the bucket_it is not NULL, free the bucket_it
  // and set the bucket_it to NULL
  if (iter->bucket_it != NULL) {
    LLIteratorFree(iter->bucket_it);
    iter->bucket_it = NULL;
  }
  // Set the iterator to invalid
  iter->is_valid = false;
  // Free the iterator
  free(iter);
}

//// Helper function
// Find the next non-empty bucket.
//
// Arguments:
//
// - iter: the iterator to find next non-empty bucket
//
// - bucket_to_return: return parameter in which to non-empty
//   bucket is returned
//
// Returns:
//
// - true: successfully find the next non-empty bucket
//
// - false: failed to find the next non-empty bucket
static bool FindNextNonEmptyBucket(HTIter iter, LinkedList *bucket_to_return) {
  do {
    // Increment the bucket_num in iter
    iter->bucket_num += 1;
    // Get the next bucket
    *bucket_to_return = iter->ht->buckets[iter->bucket_num];
  } while (NumElementsInLinkedList(*bucket_to_return) == 0
      && iter->bucket_num < iter->ht->num_buckets-1);
  return (NumElementsInLinkedList(*bucket_to_return) > 0);
}

int HTIteratorNext(HTIter iter) {
  Verify333(iter != NULL);

  // Step 4 -- implement HTIteratorNext.
  // Iterator is not valid
  // The table is empty
  if (!iter->is_valid) {
    return 0;  // iterator is not valid
  }
  // The LL iterator can be advanced to next element
  if (LLIteratorNext(iter->bucket_it)) {
    return 1;
  } else {
    // The LL iterator can not be advance in the same bucket
    // It is in the last bucket
    if (iter->bucket_num == iter->ht->num_buckets - 1) {
      // Iterator pass the end
      iter->is_valid = false;  // the iter is not valid
      return 0;  // moving forward will pass the end of table
    } else {  // The table has other buckets
      // Move to the first element of the next bucket
      LinkedList next_non_empty_bucket;
      // Check if the non_emptybucket if found
      if (!(FindNextNonEmptyBucket(iter, &next_non_empty_bucket))) {
        // Iterator pass the end
        iter->is_valid = false;
        return 0;  // no next valid element can be found
      }
      // Create iterator for the next bucket
      // Startingf from head of the bucket
      LLIter new_bucket_it = LLMakeIterator(next_non_empty_bucket, 0);
      if (new_bucket_it == NULL) {
        iter->is_valid = false;
        return 0;
      }
      // Free the old bucket
      LLIteratorFree(iter->bucket_it);
      // Update the new bucket_it
      iter->bucket_it = new_bucket_it;
      return 1;
    }
  }
}

int HTIteratorPastEnd(HTIter iter) {
  Verify333(iter != NULL);

  // Step 5 -- implement HTIteratorPastEnd.
  // The table is empty or the iterator is not valid
  // Iterator is invalid if it is pass the end.
  if (!(iter->is_valid) || NumElementsInHashTable(iter->ht) == 0)
    return 1;
  return 0;
}

int HTIteratorGet(HTIter iter, HTKeyValue *keyvalue) {
  Verify333(iter != NULL);

  // Step 6 -- implement HTIteratorGet.
  // When the iterator is not valid
  // or the table has 0 elements
  if (!iter->is_valid || iter->ht->num_elements == 0)
    return 0;
  HTKeyValue* keyvalue_to_return;
  // Get the old keyvalue and copy it to return paramater.
  LLIteratorGetPayload(iter->bucket_it, (LLPayload_t*)&keyvalue_to_return);
  keyvalue->key = keyvalue_to_return->key;
  keyvalue->value = keyvalue_to_return->value;
  return 1;  // sucessfully get the keyvalue
}

int HTIteratorDelete(HTIter iter, HTKeyValue *keyvalue) {
  HTKeyValue kv;
  int res, retval;

  Verify333(iter != NULL);

  // Try to get what the iterator is pointing to.
  res = HTIteratorGet(iter, &kv);
  if (res == 0)
    return 0;

  // Advance the iterator.
  res = HTIteratorNext(iter);
  if (res == 0) {
    retval = 2;
  } else {
    retval = 1;
  }
  res = RemoveFromHashTable(iter->ht, kv.key, keyvalue);
  Verify333(res == 1);
  Verify333(kv.key == keyvalue->key);
  Verify333(kv.value == keyvalue->value);

  return retval;
}

static void ResizeHashtable(HashTable ht) {
  // Resize if the load factor is > 3.
  if (ht->num_elements < 3 * ht->num_buckets)
    return;

  // This is the resize case.  Allocate a new hashtable,
  // iterate over the old hashtable, do the surgery on
  // the old hashtable record and free up the new hashtable
  // record.
  HashTable newht = AllocateHashTable(ht->num_buckets * 9);

  // Give up if out of memory.
  if (newht == NULL)
    return;

  // Loop through the old ht with an iterator,
  // inserting into the new HT.
  HTIter it = HashTableMakeIterator(ht);
  if (it == NULL) {
    // Give up if out of memory.
    FreeHashTable(newht, &HTNullFree);
    return;
  }

  while (!HTIteratorPastEnd(it)) {
    HTKeyValue item, dummy;

    Verify333(HTIteratorGet(it, &item) == 1);
    if (InsertHashTable(newht, item, &dummy) != 1) {
      // failure, free up everything, return.
      HTIteratorFree(it);
      FreeHashTable(newht, &HTNullFree);
      return;
    }
    HTIteratorNext(it);
  }

  // Worked!  Free the iterator.
  HTIteratorFree(it);

  // Sneaky: swap the structures, then free the new table,
  // and we're done.
  {
    HashTableRecord tmp;

    tmp = *ht;
    *ht = *newht;
    *newht = tmp;
    FreeHashTable(newht, &HTNullFree);
  }

  return;
}

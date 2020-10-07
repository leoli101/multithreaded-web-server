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

#include <iostream>
#include <algorithm>

#include "./QueryProcessor.h"

extern "C" {
  #include "./libhw1/CSE333.h"
}

namespace hw3 {

// Helper method to combine the intersected part of the lists.
// The lists passed in contains docid_element_header. This method
// traverse these two lists, extract docids that are contianed in
// both lists. It then adds up the rank and create a new docid_element_header
// to store them. The new docid_element_header then gets appended
// to the return list.
//
// Arguments:
// -ret_list: A reference to a list to combine
//
// -partial_list: Another reference to a list to combine
//
// Return:
// - A list of docid_element_header containing docids that can be
// found from both lists and the rank is the combination of that of
// both lists passed in.
static list<docid_element_header> CombineDocIDList
                              (const list<docid_element_header> &ret_list,
                               const list<docid_element_header> &partial_list);

// Helper method to add the desired QueryResult to the finalresult.
// It traverses the final_docid_list. It then looks up the docid
// from DocTableReader. If the docid is found, it creates
// a new QueryResult and set the file name and rank. The QueryResult
// is pushed into the finalresult.
//
// Arguments:
// -doc_table_reader_ptr: A pointer to a DocTableReader
//
// -final_docid_list: A constant reference to a list of docid_element_header
//
// -finalresult_ptr: A pointer to a vector of QueryResult
static void AddQueryResult(DocTableReader *doc_table_reader_ptr,
                  const list<docid_element_header> &final_docid_list,
                  vector<QueryProcessor::QueryResult> *finalresult_ptr);
QueryProcessor::QueryProcessor(list<string> indexlist, bool validate) {
  // Stash away a copy of the index list.
  indexlist_ = indexlist;
  arraylen_ = indexlist_.size();
  Verify333(arraylen_ > 0);

  // Create the arrays of DocTableReader*'s. and IndexTableReader*'s.
  dtr_array_ = new DocTableReader *[arraylen_];
  itr_array_ = new IndexTableReader *[arraylen_];

  // Populate the arrays with heap-allocated DocTableReader and
  // IndexTableReader object instances.
  list<string>::iterator idx_iterator = indexlist_.begin();
  for (HWSize_t i = 0; i < arraylen_; i++) {
    FileIndexReader fir(*idx_iterator, validate);
    dtr_array_[i] = new DocTableReader(fir.GetDocTableReader());
    itr_array_[i] = new IndexTableReader(fir.GetIndexTableReader());
    idx_iterator++;
  }
}

QueryProcessor::~QueryProcessor() {
  // Delete the heap-allocated DocTableReader and IndexTableReader
  // object instances.
  Verify333(dtr_array_ != nullptr);
  Verify333(itr_array_ != nullptr);
  for (HWSize_t i = 0; i < arraylen_; i++) {
    delete dtr_array_[i];
    delete itr_array_[i];
  }

  // Delete the arrays of DocTableReader*'s and IndexTableReader*'s.
  delete[] dtr_array_;
  delete[] itr_array_;
  dtr_array_ = nullptr;
  itr_array_ = nullptr;
}

vector<QueryProcessor::QueryResult>
QueryProcessor::ProcessQuery(const vector<string> &query) {
  Verify333(query.size() > 0);

  // STEP 1:
  // (the only step in this file)
  vector<QueryProcessor::QueryResult> finalresult;
  HWSize_t i;
  for (i = 0; i < arraylen_; i++) {
    // Extract the DocTableReader of this index file.
    DocTableReader *doc_table_reader_ptr = dtr_array_[i];
    // Look up the first query word in this index file.
    DocIDTableReader *docid_table_reader_ptr =
                            (itr_array_[i])->LookupWord(query[0]);
    // If the current IndexTableReader can not find the query word,
    // skip to the next iteration.
    if (docid_table_reader_ptr == nullptr)
      continue;
    // Found the query word in IndexTableReader, extract the list of
    // docid_element_header from DocIDTableReader.
    list<docid_element_header> final_docid_list =
                            docid_table_reader_ptr->GetDocIDList();
    // Free the docid_table_reader_ptr.
    delete docid_table_reader_ptr;
    // If the query has only one word. Then just traver this list and
    // add the corresponding QueryResults to finalresult.
    if (query.size() == 1) {
      // Traverse thfinal and extract the actual file name from
      // DocTableReader and rank. Create a new QueryResult for each docid
      // that can be found and append it to finalresult.
      std::string file_name;
      AddQueryResult(doc_table_reader_ptr, final_docid_list, &finalresult);
      // Done for the single query word case. Process the next index file
      continue;
    }
    // Query contains multiple words. Handle these words one by one.
    list<docid_element_header> cur_word_docid_list;
    // Starting from the second query word, Look up each word.
    for (auto query_it = query.begin() + 1;
        query_it != query.end();
        query_it++) {
      // Look up the current query word in this index file.
      docid_table_reader_ptr = (itr_array_[i])->LookupWord(*query_it);
      // If it can not find the word from this index file, then the query
      // can not be found either. Skip to process the next index file.
      if (docid_table_reader_ptr == nullptr) {
        final_docid_list.clear();
        break;
      }
      // Extract the list of docid_element_header from docid_table_reader.
      cur_word_docid_list = docid_table_reader_ptr->GetDocIDList();
      // Free the docid_table_reader_ptr.
      delete docid_table_reader_ptr;
      // The final_docid_list only contains the docids contains
      // both query words.
      final_docid_list = CombineDocIDList(final_docid_list,
                                          cur_word_docid_list);
    }
    // Obtained the final_docid_list.
    if (final_docid_list.size() != 0) {
      AddQueryResult(doc_table_reader_ptr, final_docid_list, &finalresult);
    }
  }

  // Sort the final results.
  std::sort(finalresult.begin(), finalresult.end());
  return finalresult;
}

static list<docid_element_header> CombineDocIDList
       (const list<docid_element_header> &ret_list,
       const list<docid_element_header> &partial_list) {
  list<docid_element_header> final_list;
  docid_element_header combine;
  for (auto partial_list_it : partial_list) {
    for (auto ret_list_it : ret_list) {
      if (ret_list_it.docid == partial_list_it.docid) {
        combine.docid = ret_list_it.docid;
        combine.num_positions = ret_list_it.num_positions +
                                partial_list_it.num_positions;
        final_list.push_back(combine);
      }
    }
  }
  return final_list;
}

static void AddQueryResult(DocTableReader *doc_table_reader_ptr,
                           const list<docid_element_header> &final_docid_list,
                           vector<QueryProcessor::QueryResult>
                           *finalresult_ptr) {
  QueryProcessor::QueryResult final_qr;
  std::string file_name;
  for (auto final_docid_list_it : final_docid_list) {
    if (doc_table_reader_ptr->LookupDocID(final_docid_list_it.docid,
                                          &file_name)) {
      final_qr.document_name = file_name;
      final_qr.rank = final_docid_list_it.num_positions;
      finalresult_ptr->push_back(final_qr);
    }
  }
}

}  // namespace hw3

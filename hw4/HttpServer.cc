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

#include <boost/algorithm/string.hpp>
#include <iostream>
#include <memory>
#include <vector>
#include <sstream>

#include "./FileReader.h"
#include "./HttpConnection.h"
#include "./HttpRequest.h"
#include "./HttpUtils.h"
#include "./HttpServer.h"
#include "./libhw3/QueryProcessor.h"

using std::cerr;
using std::cout;
using std::endl;

namespace hw4 {

// This is the function that threads are dispatched into
// in order to process new client connections.
void HttpServer_ThrFn(ThreadPool::Task *t);

// Given a request, produce a response.
HttpResponse ProcessRequest(const HttpRequest &req,
                            const std::string &basedir,
                            const std::list<std::string> *indices);

// Process a file request.
HttpResponse ProcessFileRequest(const std::string &uri,
                                const std::string &basedir);

// Process a query request.
HttpResponse ProcessQueryRequest(const std::string &uri,
                                 const std::list<std::string> *indices);

bool HttpServer::Run(void) {
  // Create the server listening socket.
  int listen_fd;
  cout << "  creating and binding the listening socket..." << endl;
  if (!ss_.BindAndListen(AF_INET6, &listen_fd)) {
    cerr << endl << "Couldn't bind to the listening socket." << endl;
    return false;
  }

  // Spin, accepting connections and dispatching them.  Use a
  // threadpool to dispatch connections into their own thread.
  cout << "  accepting connections..." << endl << endl;
  ThreadPool tp(kNumThreads);
  while (1) {
    HttpServerTask *hst = new HttpServerTask(HttpServer_ThrFn);
    hst->basedir = staticfile_dirpath_;
    hst->indices = &indices_;
    if (!ss_.Accept(&hst->client_fd,
                    &hst->caddr,
                    &hst->cport,
                    &hst->cdns,
                    &hst->saddr,
                    &hst->sdns)) {
      // The accept failed for some reason, so quit out of the server.
      // (Will happen when kill command is used to shut down the server.)
      break;
    }
    // The accept succeeded; dispatch it.
    tp.Dispatch(hst);
  }
  return true;
}

void HttpServer_ThrFn(ThreadPool::Task *t) {
  // Cast back our HttpServerTask structure with all of our new
  // client's information in it.
  std::unique_ptr<HttpServerTask> hst(static_cast<HttpServerTask *>(t));
  cout << "  client " << hst->cdns << ":" << hst->cport << " "
       << "(IP address " << hst->caddr << ")" << " connected." << endl;

  // Use the HttpConnection class to read in the next request from
  // this client, process it by invoking ProcessRequest(), and then
  // use the HttpConnection class to write the response.  If the
  // client sent a "Connection: close\r\n" header, then shut down
  // the connection.

  // STEP 1:
  HttpConnection hc(hst->client_fd);
  HttpRequest req;
  bool res;
  bool done = false;
  // Connection is not shut down.
  while (!done) {
    // Get the next requese.
    res = hc.GetNextRequest(&req);
    // No request, close connection.
    if (!res) {
      done = true;
    }
    // Process the request.
    HttpResponse hp = ProcessRequest(req, hst->basedir, hst->indices);
    res = hc.WriteResponse(hp);
    // Failed to write or client told to close connection.
    if (!res || (hp.headers["connection"] == "close")) {
      done = true;
    }
  }
}

HttpResponse ProcessRequest(const HttpRequest &req,
                            const std::string &basedir,
                            const std::list<std::string> *indices) {
  // Is the user asking for a static file?
  if (req.URI.substr(0, 8) == "/static/") {
    return ProcessFileRequest(req.URI, basedir);
  }

  // The user must be asking for a query.
  return ProcessQueryRequest(req.URI, indices);
}


HttpResponse ProcessFileRequest(const std::string &uri,
                                const std::string &basedir) {
  // The response we'll build up.
  HttpResponse ret;

  // Steps to follow:
  //  - use the URLParser class to figure out what filename
  //    the user is asking for.
  //
  //  - use the FileReader class to read the file into memory
  //
  //  - copy the file content into the ret.body
  //
  //  - depending on the file name suffix, set the response
  //    Content-type header as appropriate, e.g.,:
  //      --> for ".html" or ".htm", set to "text/html"
  //      --> for ".jpeg" or ".jpg", set to "image/jpeg"
  //      --> for ".png", set to "image/png"
  //      etc.
  //
  // be sure to set the response code, protocol, and message
  // in the HttpResponse as well.
  std::string fname = "";

  // STEP 2:
  URLParser url_parser;
  // Parse the uri.
  url_parser.Parse(uri);
  // File name should not contain "/static/"
  fname.append(url_parser.get_path().substr(8));
  FileReader file_reader(basedir, fname);
  std::string fullname = "";
  fullname = basedir + "/" + fname;
  // If the path is safe and successfully read in the file.
  if (IsPathSafe(basedir, fullname) &&
    file_reader.ReadFile(&ret.body)) {
    // Assign protocol, message and response code.
    ret.protocol = "HTTP/1.1";
    ret.message = "OK";
    ret.response_code = 200;
    vector<string> vals;
    // Split the file name into string using delimiter of "."
    boost::split(vals, fname, boost::is_any_of("."));
    // Assign different content-type header.
    if (vals[1] == "html" || vals[1] == "htm") {
      ret.headers["Content-type"] = "text/html";
    } else if (vals[1] == "jpeg" || vals[1] == "jpg") {
      ret.headers["Content-type"] = "image/jpeg";
    } else if (vals[1] == "png") {
      ret.headers["Content-type"] = "image/png";
    } else if (vals[1] == "doc") {
      ret.headers["Content-type"] = "text.doc";
    } else if (vals[1] == "odt") {
      ret.headers["Content-type"] = "text.odt";
    } else if (vals[1] == "pdf") {
      ret.headers["Content-type"] = "text.pdf";
    } else if (vals[1] == "txt") {
      ret.headers["Content-type"] = "text.txt";
    } else if (vals[1] == "wks") {
      ret.headers["Content-type"] = "text.wks";
    } else {
      ret.headers["Content-type"] = "others";
    }
    return ret;
  }

  // If you couldn't find the file, return an HTTP 404 error.
  ret.protocol = "HTTP/1.1";
  ret.response_code = 404;
  ret.message = "Not Found";
  ret.body = "<html><body>Couldn't find file \"";
  ret.body +=  EscapeHTML(fname);
  ret.body += "\"</body></html>";
  return ret;
}

HttpResponse ProcessQueryRequest(const std::string &uri,
                                 const std::list<std::string> *indices) {
  // The response we're building up.
  HttpResponse ret;

  // Your job here is to figure out how to present the user with
  // the same query interface as our solution_binaries/http333d server.
  // A couple of notes:
  //
  //  - no matter what, you need to present the 333gle logo and the
  //    search box/button
  //
  //  - if the user had previously typed in a search query, you also
  //    need to display the search results.
  //
  //  - you'll want to use the URLParser to parse the uri and extract
  //    search terms from a typed-in search query.  convert them
  //    to lower case.
  //
  //  - you'll want to create and use a hw3::QueryProcessor to process
  //    the query against the search indices
  //
  //  - in your generated search results, see if you can figure out
  //    how to hyperlink results to the file contents, like we did
  //    in our solution_binaries/http333d.

  // STEP 3:
  // 333gle logo and the search box/button.
  ret.body = "<html><head><title>333gle</title></head>\r\n";
  ret.body.append("<body>\r\n");
  ret.body.append("<center style=\"font-size:500%;\">\r\n");
  ret.body.append("<span style=\"position:relative;bottom:-0.33em;");
  ret.body.append("color:orange;\">3</span><span style=\"color:");
  ret.body.append("red;\">3</span><span style=\"color:gold;\">3</span>");
  ret.body.append("<span style=\"color:blue;\">g</span>");
  ret.body.append("<span style=\"color:green;\">l</span>");
  ret.body.append("<span style=\"color:red;\">e</span>\r\n");
  ret.body.append("</center>\r\n");
  ret.body.append("<p>\r\n");
  ret.body.append("<div style=\"height:20px;\"></div>\r\n");
  ret.body.append("<center>\r\n");
  ret.body.append("<form action=\"/query\" method=\"get\">\r\n");
  ret.body.append("<input type=\"text\" size=30 name=\"terms\" />\r\n");
  ret.body.append("<input type=\"submit\" value=\"Search\" />\r\n");
  ret.body.append("</form>\r\n");
  ret.body.append("</center><p>\r\n");
  // check if user isssued an query.
  if (uri.find("query?terms") != string::npos) {
    URLParser url_parser;
    url_parser.Parse(uri);
    map<string, string> args = url_parser.get_args();
    vector<string> queries;
    // Extract the term user want to search
    string search_terms = args.at("terms");
    // Split the term into strings with delimiter of "+".
    boost::split(queries, search_terms,
                boost::is_any_of("+"),
                boost::token_compress_on);
    // A string to store the concatenation of queries seperate
    // by space.(No trailing space)
    string queries_str = "";
    if (queries.size() > 0) {
      // Convert each query to lower case.
      for (auto &query : queries) {
        boost::to_lower(query);
        queries_str.append(query);
        // query = EscapeHTML(query);
      }
      boost::trim(queries_str);
    }
    hw3::QueryProcessor qp(*indices);
    vector<hw3::QueryProcessor::QueryResult> ret_qr;
    if (queries.size() > 0)
      ret_qr = qp.ProcessQuery(queries);
    ret.body.append("<p><br>\r\n");
    // Check if the query can be found.
    if (ret_qr.size() > 0) {
      ret.body.append(std::to_string(ret_qr.size()));
      ret.body.append(" results found for <b>");
      ret.body.append(EscapeHTML(queries_str));
      ret.body.append("</b>\r\n");
      ret.body.append("<p>\r\n");
      ret.body.append("\r\n");
      ret.body.append("<ul>\r\n");
      // Hyperlink results to the file content.
      for (auto cur_qr : ret_qr) {
        ret.body.append(" <li> <a href=\"");
        if (cur_qr.document_name.substr(0, 4) != "http")
          ret.body.append("/static/");
        ret.body.append(cur_qr.document_name);
        ret.body.append("\">");
        ret.body.append(cur_qr.document_name);
        ret.body.append("</a> [");
        ret.body.append(std::to_string(cur_qr.rank));
        ret.body.append("]<br>\r\n");
      }
      ret.body.append("</ul>\r\n");

    } else {
      ret.body.append("No results found for <b>");
      ret.body.append(EscapeHTML(queries_str));
      ret.body.append("</b>\r\n");
      ret.body.append("<p>\r\n");
      ret.body.append("\r\n");
    }
  }
  ret.body.append("</body>\r\n");
  ret.body.append("</html\r\n");
  ret.protocol = "HTTP/1.1";
  ret.response_code = 200;
  ret.message = "OK";
  return ret;
}

}  // namespace hw4

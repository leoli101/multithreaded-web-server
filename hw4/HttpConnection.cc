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

#include <stdint.h>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <map>
#include <string>
#include <vector>

#include "./HttpRequest.h"
#include "./HttpUtils.h"
#include "./HttpConnection.h"

#define BUFSIZE 256

using std::map;
using std::string;

namespace hw4 {

bool HttpConnection::GetNextRequest(HttpRequest *request) {
  // Use "WrappedRead" to read data into the buffer_
  // instance variable.  Keep reading data until either the
  // connection drops or you see a "\r\n\r\n" that demarcates
  // the end of the request header.
  //
  // Once you've seen the request header, use ParseRequest()
  // to parse the header into the *request argument.
  //
  // Very tricky part:  clients can send back-to-back requests
  // on the same socket.  So, you need to preserve everything
  // after the "\r\n\r\n" in buffer_ for the next time the
  // caller invokes GetNextRequest()!

  // STEP 1:
  unsigned char ret_buf[BUFSIZE];
  int res;
  size_t pos;
  // If there is already a "\r\n\r\n" in the buffer_,
  // no need to read in more data.
  if ((pos = buffer_.find("\r\n\r\n")) == std::string::npos) {
    // Read until either the connection drop or a "\r\n\r\n".
    while (1) {
      res = WrappedRead(fd_, ret_buf, BUFSIZE);
      // EOF, done reading.
      if (res == 0) {
        // If "\r\n\r\n" is still not found after finishing
        // reading, no request is available.
        if ((pos = buffer_.find("\r\n\r\n")) == std::string::npos)
          return false;
        break;
      }
      // fatal error.
      if (res == -1)
        return false;
      // Succesfully read in some data, append the data into buffer_.
      buffer_ .append(string(reinterpret_cast<char *>(ret_buf), res));
      // Found "\r\n\r\n", stop reading.
      if ((pos = buffer_.find("\r\n\r\n")) != std::string::npos) {
        break;
      }
    }
  }
  // The request contains all info plus "\r\n\r\n".
  *request = ParseRequest(pos + 4);
  // Preserve everything in the buffer after "\r\n\r\n".
  buffer_ = buffer_.substr(pos + 4);

  return true;
}

bool HttpConnection::WriteResponse(const HttpResponse &response) {
  std::string str = response.GenerateResponseString();
  int res = WrappedWrite(fd_,
                         (unsigned char *) str.c_str(),
                         str.length());
  if (res != static_cast<int>(str.length()))
    return false;
  return true;
}

HttpRequest HttpConnection::ParseRequest(size_t end) {
  HttpRequest req;
  req.URI = "/";  // by default, get "/".

  // Get the header.
  std::string str = buffer_.substr(0, end);

  // Split the header into lines.  Extract the URI from the first line
  // and store it in req.URI.  For each additional line beyond the
  // first, extract out the header name and value and store them in
  // req.headers (i.e., req.headers[headername] = headervalue).
  // You should look at HttpResponse.h for details about the HTTP header
  // format that you need to parse.
  //
  // You'll probably want to look up boost functions for (a) splitting
  // a string into lines on a "\r\n" delimiter, (b) trimming
  // whitespace from the end of a string, and (c) converting a string
  // to lowercase.

  // STEP 2:
  std::vector<string> lines;
  // Split the str into lines using "\r\n" delimiter.
  boost::split(lines, str, boost::is_any_of("\r\n"),
              boost::token_compress_on);
  // Trim the trailing space of each line.
  for (auto& line : lines)
    boost::trim_right(line);
  // Extract the URI from first line.
  std::string cur_line = lines.front();
  std::vector<string> cur_line_elements;
  // Split first line into strings using " " delimiter.
  boost::split(cur_line_elements, cur_line,
              boost::is_any_of(" "), boost::token_compress_on);
  // Assign the req.URI.
  req.URI = cur_line_elements.at(1);
  // Process each line after the first one.
  for (auto it = lines.begin() + 1; it < lines.end() - 1; it++) {
    // Split the line into strings using ": " delimiter.
    boost::split(cur_line_elements, *it,
                boost::is_any_of(": "),
                boost::token_compress_on);
    string header = cur_line_elements[0];
    // Convert header to lower-case.
    boost::to_lower(header);
    req.headers[header] = cur_line_elements[1];
  }

  return req;
}

}  // namespace hw4

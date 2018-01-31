// Sahil Gandhi and Arpit Jasapara
// CS 118 Winter 2018
// Project 1
// This is the server code for the first project.

#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <strings.h>
#include <signal.h>
#include <string>
#include <time.h>

using namespace std;

#define PORT 5000
#define BACKLOG 10

string HTM = "Content-Type: text/html\r\n";
string HTML = "Content-Type: text/html\r\n";
string JPEG = "Content-Type: image/jpeg\r\n";
string JPG = "Content-Type: image/jpeg\r\n";
string GIF = "Content-Type: image/gif\r\n";
string TXT = "Content-Type: text/plain\r\n";

string STATUS_ERROR = "HTTP/1.1 404 Not Found\r\n";
string STATUS_OK = "HTTP/1.1 200 OK\r\n";

/**
 * This method throws the perror and exits the program
 * @param s A string that is the error message
 **/
void throwError(string s) {
  perror(s.c_str());
  exit(1);
}

/**
 * This method parses the HTML request for a file. We MUST be able to handle
 *files with spaces.
 * @param buffer A character array
 * @return string A string representing the file that was found, or an empty
 *string if no file was found.
 **/
string parseFileName(char *buffer);

/**
 * This method will write the return response back to the browser (and show the
 *ERROR code or the
 * correct file/image that needs to be shown).
 * Look into chapter 2 for writing proper HTTP responses (apparently people get
 *points off here!)
 * @param new_fd  The file descriptor of the connected socket
 **/
void writeResponse(int new_fd);

/**
 * This method will get the file type of the requested file.
 * @param file The string which contains the full file name
 * @return string The filetype of the requested file
 **/
string parseFileType(string file);

int main() {
  int sockfd, new_fd;
  struct sockaddr_in my_addr;
  struct sockaddr_in their_addr;
  socklen_t sin_size;

  if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    throwError("socket");
  }

  memset((char *)&my_addr, 0, sizeof(my_addr));

  my_addr.sin_family = AF_INET;
  my_addr.sin_port = htons(PORT);
  my_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) < 0) {
    throwError("bind");
  }

  if (listen(sockfd, BACKLOG) < 0) {
    throwError("listen");
  }

  sin_size = sizeof(struct sockaddr_in);

  // TODO: reap child process and register signal handler
  while (1) {
    if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) <
        0) {
      throwError("accept");
    }

    pid_t pid = fork();

    if (pid == 0) {
      // child process
      close(sockfd);
      writeResponse(new_fd);
      close(new_fd);
      exit(0);
    } else if (pid > 0) {
      // parent process
      close(new_fd);
      continue;
    } else {
      throwError("Fork failed, closing the program");
    }
  }
  return 0;
}

void writeResponse(int new_fd) {
  int n, file_fd, bytesRead;
  struct stat fileInfo;
  time_t t;
  struct tm* timeTm, modifyTm;
  char currTime[34];
  char modifyTime[34];
  char * fileBuffer;
  char buffer[8192]; // 8192 is usually the largest size that we may have to
                     // handle
  memset(buffer, 0, 8192);
  n = read(new_fd, buffer, 8192);
  if (n < 0)
   	perror("Error reading from the socket");
  cout << buffer << endl;
  string fileName = parseFileName(buffer);
  cout << fileName << endl;

  // sometimes we get random requests that don't actually have any input headers
  if (fileName == "") {
    return;
  }

  file_fd = open(fileName, O_RDONLY);
  if (file_fd < 0){
  	write(new_fd, STATUS_ERROR.c_str(), STATUS_ERROR.length());
  	return;
  }

  if(fstat(file_fd, &fileInfo) < 0){
  	throwError("bad file")
  }

  ifstream reqFile (fileName, ios::in|ios::binary|ios::ate);
  streampos fileSize;
  if(reqFile.is_open()){
  	fileSize = reqFile.tellg();
  	fileBuffer = new char [fileSize];
  	reqFile.seekg (0, ios::beg);
    reqFile.read (fileBuffer, fileSize);
    reqFile.close();
  }

  t = time(NULL);
  timeTm = gmtime(&t);
  modifyTm = gmtime(fileInfo.st_mtime);
  strftime(currTime, 34, "%a, %d %b %G %T GMT\r\n", timeTm); // Sun, 26 Sep 2010 20:09:20 GMT\r\n format
  strftime(modifyTime, 34, "%a, %d %b %G %T GMT\r\n", modifyTm);

  // TODO: Use fstat for reading in file facts
  string responseStatus = "";
  string date = currTime;
  string server = "Gandhi-Jasapara Server";
  string lastModified = "modifyTime";
  string contentLength = string(fileInfo.st_size);
  string closeConnection = "";
  string body = fileBuffer;
  string contentType = parseFileType(fileName);

  string respHeader = responseStatus + date + server + lastModified +
                      contentLength + contentType + closeConnection + "\r\n";

  write(new_fd, STATUS_OK.c_str(), STATUS_OK.length());
  // write(new_fd, respHeader.c_str(), respHeader.length());
  // write(new_fd, body.c_str(), body.length();
  delete[] fileBuffer;
}

string parseFileName(char *buffer) {
  string request(buffer);
  if (request == "") {
    return "";
  }
  size_t found = request.find(" ");
  cout << found << endl;
  size_t found2 = request.find(" HTTP", found + 1, 5); // TODO: make more ROBUST
  cout << found2 << endl;
  return found2 - found - 2 > 0 ? request.substr(found + 2, found2 - found - 2)
                                : "";
}

string parseFileType(string file) {
  if (file.find(".html") != string::npos) {
    return HTML;
  } else if (file.find(".htm") != string::npos) {
    return HTML;
  } else if (file.find(".gif") != string::npos) {
    return GIF;
  } else if (file.find(".jpeg") != string::npos) {
    return JPEG;
  } else if (file.find(".jpg") != string::npos) {
    return JPG;
  }
  return TXT; // by default we will assume everything else is a text file
}
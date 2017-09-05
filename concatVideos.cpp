#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <array>
#include <algorithm>
#include <vector>

#include "Windows.h"
#include "Winbase.h"

/** Name of temporary file **/
#define TEMP_FILE_NAME "temp_input_file.txt"

/** Execution buffer size **/
#define EXEC_BUFFER_SIZE 1024

/** Print debugging output **/
#define DEBUG

using namespace std;

string exec (const char*);

int main (int argc, char* argv[]) 
{
  
  if (argc < 4) {
  	cout << "Usage: concatVideos <path/to/FFMPEG> <path/to/Videos> <output-file.mp4>" << endl;

  	exit(1);
  }

  string pathToFFMPEG        = argv[1];
  string pathToVideos        = argv[2];
  string outputFileName      = argv[3];

  string tempFileName = TEMP_FILE_NAME;

  // Get input files, store in vector
  vector<string> inputFiles;
  HANDLE hFind;
  WIN32_FIND_DATA data;

  stringstream ss;
  ss << pathToVideos << "\\*.*";

  hFind = FindFirstFile(ss.str().c_str(), &data);
  if (hFind != INVALID_HANDLE_VALUE) {
  	do {

  		if (strcmp(data.cFileName,".") == 0 ||
  			strcmp(data.cFileName,"..") == 0)
  			continue;

    	inputFiles.push_back(data.cFileName);

    } while (FindNextFile(hFind, &data));
    FindClose(hFind);
  }

  // Sort the string vector
  sort(inputFiles.begin(), inputFiles.end());

  // Open the temp file
  fstream fs (tempFileName.c_str(), fstream::out);

  int i;
  for (i=0; i<inputFiles.size(); i++)
  	fs << "file '" << pathToVideos << "\\" << inputFiles[i] << "'" << endl;

  fs.close();

#ifdef DEBUG
  cout << "Wrote " << tempFileName << " with " << inputFiles.size() << " video files!" << endl;
#endif  

  // Make the call!
  ss.str("");
  ss << "\"" << pathToFFMPEG << "\" -f concat -safe 0 -i " 
     << tempFileName << " -c copy " << outputFileName;

  string result = exec(ss.str().c_str());

  // Output results
#ifdef DEBUG  
  cout << result << endl;
#endif

  // Delete temporary file
  if (DeleteFile(tempFileName.c_str()) == FALSE) {
  	cerr << "Error: could not delete temp file: " << tempFileName << endl;
  
  } else {

#ifdef DEBUG
    cout << "Deleted temp file name!" << endl;
#endif  	
  }

  exit(0);
}

/**
 * Executes a command and returns the results to the string variable.
 */
string exec (const char* cmd) {
    array<char, EXEC_BUFFER_SIZE> buffer;
    string result;
    shared_ptr<FILE> pipe(_popen(cmd, "r"), _pclose);
    
    if (!pipe) 
    	throw runtime_error("popen() failed!");

    while (!feof(pipe.get())) {
        if (fgets(buffer.data(), EXEC_BUFFER_SIZE, pipe.get()) != NULL)
            result += buffer.data();
    }

    return result;
}
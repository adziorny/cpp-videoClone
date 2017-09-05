#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
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
//#define DEBUG

using namespace std;

/** Function definitions **/
string exec (const char*);
void gatherInputFiles (vector<string>*, string*);
void writeTempFile (vector<string>*, string*, string*);

/**
 * Concatenates individual video chunks into a single large MP4 video by
 * creating a list, saving the temporary name of the list, and calling
 * the concatenation command in FFMPEG.
 */
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

  vector<string> inputFiles;

  // Gather the input files by scanning the directory and storing in a vector
  gatherInputFiles(&inputFiles, &pathToVideos);

  // Sort the string vector
  sort(inputFiles.begin(), inputFiles.end());

  // Write out temp file
  writeTempFile(&inputFiles, &pathToVideos, &tempFileName);

  // Make the call!
  stringstream ss;
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

/**
 * Scans the folder `pathToVideos` and finds all files not '.' or '..', 
 * and stores in the passed vector.,
 */
void gatherInputFiles (vector<string>* inputFiles, string* pathToVideos) {

  HANDLE hFind;
  WIN32_FIND_DATA data;

  stringstream ss;
  ss << (*pathToVideos) << "\\*.*";

  hFind = FindFirstFile(ss.str().c_str(), &data);
  if (hFind != INVALID_HANDLE_VALUE) {
    do {

      if (strcmp(data.cFileName,".") == 0 ||
        strcmp(data.cFileName,"..") == 0)
        continue;

      inputFiles->push_back(data.cFileName);

    } while (FindNextFile(hFind, &data));
    FindClose(hFind);
  }
}

/**
 * Writes out the contents of the inputFiles vector, with prefix pathToVideos,
 * to the temporary file name specified by tempFileName.
 */
void writeTempFile (vector<string>* inputFiles, string* pathToVideos, string* tempFileName) {

  // Open the temp file
  fstream fs (tempFileName->c_str(), fstream::out);

  int i;
  for (i=0; i<inputFiles->size(); i++)
    fs << "file '" << (*pathToVideos) << "\\" << inputFiles->at(i) << "'" << endl;

  fs.close();

#ifdef DEBUG
  cout << "Wrote " << (*tempFileName) << " with " << inputFiles->size() << " video files!" << endl;
#endif  
}
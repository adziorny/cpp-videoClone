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

/** Duration to sleep between checks in milliseconds **/
#define SLEEP_DURATION_MSEC 1000

/** Number of durations in a row of zero copies before breaking **/
#define MAX_ZERO_COPIES 15

/** Execution buffer size **/
#define EXEC_BUFFER_SIZE 1024

/** Print debugging output **/
//#define DEBUG
//#define DEBUG1

using namespace std;

/** Function definitions **/
std::string exec(const char*);
int findFirstInNameSizeArrays (vector<string>*, vector<DWORDLONG>*,
	    string*, DWORDLONG, int*);
int findFirstStringInVector (vector<string>*, string*);
void getBaseFileNames (string*, vector<string>*);
bool isValidVideoAudio (string*, const char*);
int readCompareCopy (string*, string*, 
	    string*, vector<string>*,
	    vector<string>*, vector<DWORDLONG>*,
	    vector<string>*, vector<DWORDLONG>*);
void waitForUserResponse (char*);

/**
 * Stitch together multiple MP4 files from a video service such as Panopto
 * into a single MP4.
 *
 * Google Cache Directory:
 *   ~\AppData\Local\Google\Chrome\User Data\Default\Cache
 *
 * MP4 Joiner:
 *   http://www.mp4joiner.org
 *
 * ffmpeg:
 *   https://www.ffmpeg.org/download.html
 *   (downloaded static x64 build)
 *
 * Algorithm
 * -=-=-=-=-
 *
 * -- Validate inputs
 * -- Obtain directory listing of Cache directory
 * -- Run 'ffmpeg' on every file, determine if valid video file
 * -- Copy valid files into output directory, then manually add to 
 *    MP4Joiner in proper (ASC) order to join
 *
 * References
 * -=-=-=-=-=-
 *
 * Obtaining stream information from ffmpeg:
 *   https://askubuntu.com/questions/303454/get-information-about-a-video-from-command-line-tool
 * 
 * StackOverflow: Execute a command and return results to the var:
 *   https://stackoverflow.com/questions/478898/how-to-execute-a-command-and-get-output-of-command-within-c-using-posix
 */
int main (int argc, char* argv[]) 
{
	// Check usage and assign input variables
    if (argc < 4) {
		cout << "Usage: parseMP4 <path/to/ffmpeg.exe> <path/to/Cache> <path/to/Cache-Copy>"
		     << endl;

		exit(1);

	} 

	string pathToFFMPEG        = argv[1];
	string pathToCache         = argv[2];
    string pathToCacheCopy     = argv[3];

    // To start, get the files in the Cache directory now
    vector<string> baseNames;
    getBaseFileNames(&pathToCache,&baseNames);

    waitForUserResponse("Press enter and then start video ...");

    vector<string> copiedNames;
    vector<DWORDLONG> copiedSizes;
    vector<string> wasCheckedName;
    vector<DWORDLONG> wasCheckedSize;
    int numCopied, countInARow = 0;

    while (true) {

#ifdef DEBUG
    	cout << "Sleeping for " << (SLEEP_DURATION_MSEC / 1000) << " seconds" << endl;
#endif
    	Sleep(SLEEP_DURATION_MSEC);

    	numCopied = readCompareCopy (&pathToFFMPEG, &pathToCache, 
    		            &pathToCacheCopy, &baseNames, 
    		            &copiedNames, &copiedSizes,
    		            &wasCheckedName, &wasCheckedSize );

    	if (numCopied == 0)
    		countInARow++;
    	else
    		countInARow = 0;

    	if (countInARow == MAX_ZERO_COPIES)
    		break;
    }

    exit(0);
}

/**
 * Reads files in Cache directory, compares to baseline.  For each:
 *   If not in baseline, checks name / size against copied files.
 *   - If both match, continue (already copied).  
 *   - Check name / size against already-checked files.
 *     - If both match, continue (already checked).
 *     - Copy file to local directory and check for VALID_FILE
 *       - If VALID_FILE, add to copied-files vector (or update size) and next
 *       - If NOT VALID_FILE, delete file, add to already-checked vector and next
 * Returns the number of VALID_FILE files copied.
 */
int readCompareCopy (string* pathToFFMPEG, string* pathToCache, 
	    string* pathToCacheCopy, vector<string>* baseNames,
	    vector<string>* copiedNames, vector<DWORDLONG>* copiedSizes,
	    vector<string>* wasCheckedName, vector<DWORDLONG>* wasCheckedSize) {

	int numCopied = 0;

	stringstream ss, ss_out;
    ss << (*pathToCache) << "\\*.*";

    bool isValid;
    int pos, wasCopied, wasChecked, matchedIndex;
    string fName;
	HANDLE hFind;
    WIN32_FIND_DATA data;
    DWORDLONG fileSize;

    // Loop over all files in the Cache directory
    hFind = FindFirstFile(ss.str().c_str(), &data);
    if (hFind != INVALID_HANDLE_VALUE) {
      do {

      	// Skip these two first
      	if (strcmp(data.cFileName,".") == 0 ||
      		strcmp(data.cFileName,"..") == 0)
      		continue;

      	fName = data.cFileName;

#ifdef DEBUG
      	cout << "#";
#endif

        // If in baseNames vector, continue
      	pos = findFirstStringInVector(baseNames,&fName);
      	if (pos != -1) continue;

        fileSize = (data.nFileSizeHigh * (MAXDWORD+1)) + data.nFileSizeLow;

#ifdef DEBUG
        cout << endl << "File: " << data.cFileName
        	 << " File Size: " << fileSize << endl;
#endif

        // Look in copiedFiles arrays for name / size match
       	wasCopied = findFirstInNameSizeArrays(copiedNames, copiedSizes, 
       		            &fName, fileSize, &matchedIndex);

        if (wasCopied == 0) continue;

        // Look in wasChecked arrays for name / size match
        wasChecked = findFirstInNameSizeArrays(wasCheckedName, wasCheckedSize,
        	             &fName, fileSize, NULL);

        // If this was already copied and checked, and names / sizes match 
        // exactly, no need to copy and check again!!
        if (wasChecked == 0) continue;

        /**
         * First we copy, then we check validity.  This seems stupid, 
         * but it has to do with the path to the Cache having a space
         * and Windows _popen() not handling spaces correctly.  So we
         * have to copy to a local (non-spaced) folder.
         */
        ss.str("");
        ss << (*pathToCache) << "\\\\" << fName;

#ifdef DEBUG
      	cout << "  Copying file " << fName << endl;
#endif
       	ss_out.str("");
       	ss_out << (*pathToCacheCopy) << "\\\\" << fName;
        CopyFile(ss.str().c_str(), ss_out.str().c_str(), FALSE );

        isValid = isValidVideoAudio(pathToFFMPEG,ss_out.str().c_str());

        // Add to the WasChecked vectors here
        wasCheckedName->push_back(fName);
        wasCheckedSize->push_back(fileSize);

        if (isValid) {

        	numCopied++;

        	if (wasCopied == -1) {
        		copiedNames->push_back(fName);
        		copiedSizes->push_back(fileSize);
        	} else {
#ifdef DEBUG1        		
        		cout << "Set new file " << fName 
        		     << " size [" << matchedIndex << "]: old: "
        		     << copiedSizes->at(matchedIndex) << " new: " 
        		     << fileSize << " after update: ";
#endif
        		copiedSizes->at(matchedIndex) = fileSize;
#ifdef DEBUG1
        		cout << copiedSizes->at(matchedIndex) << endl;
#endif
        	} 

        } else { // If it's not a valid video file ... delete!

#ifdef DEBUG
          cout << "  Deleting file " << fName << endl;
#endif

          if (DeleteFile(ss_out.str().c_str()) == FALSE)
          	cerr << "Error deleting " << fName << endl;

        } // end IF (isValid)

      } while (FindNextFile(hFind, &data));
      FindClose(hFind);
    }

#ifdef DEBUG
    cout << endl << "NumCopied: " << numCopied << endl;
#endif

	return numCopied;
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
 * Linear search to see if name and size are in copiedNames / copiedSizes vectors.
 * Returns either -1 for not present in either, 0 for present in both or 1 for present
 * in the copiedNames vector but size mismatched.
 */
int findFirstInNameSizeArrays (vector<string>* copiedNames, vector<DWORDLONG>* copiedSizes,
	    string* name, DWORDLONG size, int* matchedIndex) {

	int i;

    for (i = 0; i < copiedNames->size(); i++) {
    	if (copiedNames->at(i).compare(*name) == 0) {

    	    if (matchedIndex != NULL) (*matchedIndex) = i;

    		if (copiedSizes->at(i) == size) 
    			return 0;
    		else 
    			return 1;
    	} 
    }

    return -1;
}

/**
 * Linear O(n) search through a vector<string> for a matching
 * string.  Returns the first matching index, or -1 if not matching.
 */
int findFirstStringInVector (vector<string>* v, string* str) {
	int i;

    for (i = 0; i < v->size(); i++) {
    	if (v->at(i).compare(*str) == 0) return i;
    }

    return -1;
}

/**
 * Populates a vector<string> with the files in the Cache directory
 * at the start of the execution.
 */
void getBaseFileNames (string* path, vector<string>* baseNames) {
	HANDLE hFind;
    WIN32_FIND_DATA data;

    stringstream ss;
    ss << (*path) << "\\*.*";

    hFind = FindFirstFile(ss.str().c_str(), &data);
    if (hFind != INVALID_HANDLE_VALUE) {
      do {
      	baseNames->push_back(data.cFileName);

      } while (FindNextFile(hFind, &data));
      FindClose(hFind);
    }

#ifdef DEBUG
    cout << "Baseline vector contains " << baseNames->size() << " elements" << endl;
#endif

	return;
}

/**
 * Returns TRUE if file contains valid Audio and Video streams,
 * as specified by FFMPEG output.  Returns FALSE otherwise.
 */
bool isValidVideoAudio (string* execFile, const char* cmd) {
    stringstream ss;
	ss << "\"" << (*execFile) << "\" -i " << cmd << " 2>&1";

#ifdef DEBUG
    cout << "  CMD: " << cmd << endl
         << "  ExecFile: " << (*execFile) << endl
         << "  ss.str().c_str(): " << ss.str().c_str() << endl;
#endif

    //ss.str("\"c:\\program files (x86)\\mp4tools\\bin\\ffmpeg.exe\" -i Cache\\f_011eb5 2>&1");

    // Execute and return results into a string
    string res = exec(ss.str().c_str());

#ifdef DEBUG
    cout << "Command Execution Output: " << endl << res << endl;
#endif

    bool isValidFile = (res.find("Input #0, mpegts") != string::npos);

    bool hasVideoStream = (res.find("Video: h264") != string::npos);
    bool hasAudioStream = (res.find("Audio: aac") != string::npos);

#ifdef DEBUG
    cout << "  isValidFile: " << ((isValidFile) ? "true" : "false") 
         << " hasVideoStream: " << ((hasVideoStream) ? "true" : "false")
         << " hasAudioStream: " << ((hasAudioStream) ? "true" : "false")
         << endl;
#endif         

    return isValidFile & hasVideoStream & hasAudioStream;
}

/**
 * Prints a message and waits for the user to press enter.
 */
void waitForUserResponse (char* msg) {
	do {
		cout << msg;
	} while (cin.get() != '\n');

	return;
}

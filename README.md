# cpp-videoClone

Poll Google cache folder, pull out video files with a video and audio stream, copy to a cached directory and stitch together.  This can be used on the UPenn Panopto system, for example, to capture video output.  

### Algorithm
- Using ParseMP4:
  - Validate inputs
  - Obtain baseline directory listing of Cache directory
  - Poll every `x` seconds, find files not in baseline directory
  - If they were already acted upon, skip; otherwise, copy them to target directory
  - Check if they are video files by running FFMPEG on them and parsing results
  - If they are not, delete them; if they are, save them and add to copied list
- Using concatVideos:
  - Create a list of valid video files in the target directory
  - Use FFMPEG to concat into a target output file
  
## Dependencies

- [MP4 Joiner](http://www.mp4joiner.org) Initially thought this would concat files, however it could not handle loading the large number of individual files. However it does contain the bin\FFMPEG.exe executable, which is required.
- [FFMPEG](https://www.ffmpeg.org/download.html) Alternatively, one may download the FFMPEG static build instead

 ### References
 
 - Obtaining stream information from ffmpeg:
   - <https://askubuntu.com/questions/303454/get-information-about-a-video-from-command-line-tool>
 - StackOverflow: Execute a command and return results to the var:
   - <https://stackoverflow.com/questions/478898/how-to-execute-a-command-and-get-output-of-command-within-c-using-posix>

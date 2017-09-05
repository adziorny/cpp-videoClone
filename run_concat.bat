@echo off

echo Assumes that MP4Tools was installed in default location, 
echo that the directory with stored files is .\Cache2, and that 
echo you would like the output file named Output.mp4

concatVideos "c:\\program files (x86)\\mp4tools\\bin\\ffmpeg.exe" Cache2 output.mp4

@echo off

echo Assumes that MP4Tools was installed in the default location, the Google Cache
echo file is located at specified directory, the chunks will be copied to 
echo the Cache2 directory (fully qualified).

parseMP4 "c:\\program files (x86)\\mp4tools\\bin\\ffmpeg.exe" "C:\\Users\\Adam\\AppData\\Local\\Google\\Chrome\\User Data\\Default\\Cache" "c:\\usr\\cpp\\joinMP4\\Cache2"
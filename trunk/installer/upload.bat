REM @echo off
copy DSharinguSetup.exe "DSharinguSetup_"%3".exe"

echo %3 dsharingu.googlecode.com /files/DSharinguSetup_%3.exe > ..\web\update_info2.txt

D:\Python25\python.exe googlecode-upload.py -s "DSharingu %2 installer"  -p dsharingu -u %1 DSharinguSetup_%3.exe -l "Type-Installer,OpSys-Windows,Featured"
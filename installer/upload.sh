#!/bin/bash

cp DSharinguSetup.exe "DSharinguSetup_$3.exe"

echo $3 dsharingu.googlecode.com /files/DSharinguSetup_$3.exe > ../web/update_info2.txt

echo googlecode-upload.py -s "\"DSharingu $2 installer\""  -p dsharingu -u $1 -l "\"Type-Installer,OpSys-Windows,Featured\"" DSharinguSetup_$3.exe > dude.txt
python googlecode-upload.py -s "\"DSharingu $2 installer\""  -p dsharingu -u $1 -l "\"Type-Installer,OpSys-Windows,Featured\"" DSharinguSetup_$3.exe
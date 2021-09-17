#!/bin/bash
set -e
dir='e:/husarnet/tmp/'

version=$(printf %s $(cat ../../core/husarnet_config.h | grep -P '#define HUSARNET_VERSION "\d+.\d+.\d+.\d+"' | cut -d'"' -f2))
perl -pi -e 's/#define MyAppVersion .+/#define MyAppVersion "'$version'"/' script.iss
unix2dos script.iss
unix2dos license.txt

mkdir -p build
wget --continue https://cdn.atomshare.net/db9d441c209fb28b7c07286a74fe000738304dac/tap-windows.exe -O build/tap-windows.exe
wget --continue https://cdn.atomshare.net/472232ca821b5c2ef562ab07f53638bc2cc82eae84cea13fbe674d6022b6481c/nssm.exe -O build/nssm.exe

chmod u+w build/*

# ok let's stop here...
exit

cp $(nix-build --no-out-link ../../gui/ -A win32.husarnet-gui)/bin/* build/
cp $(nix-build --no-out-link ../../ -A win32.husarnet)/bin/* build/

scp build/*.exe build/*.o build/*.png "win:/$dir"
scp script.iss license.txt uninstallservice.bat installservice.bat addtap.bat "win:/$dir"

ssh win '"c:\Program Files (x86)\Inno Setup 6\iscc"' $dir/script.iss
scp win:/$dir/Output/husarnet-setup.exe build/

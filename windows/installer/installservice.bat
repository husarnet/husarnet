mkdir "%PROGRAMDATA%\Husarnet"
icacls "%PROGRAMDATA%\Husarnet" /grant:r "*S-1-5-18:(F)" "*S-1-3-0:(F)" "*S-1-5-32-544:(F)" /inheritance:r
icacls "%PROGRAMDATA%\Husarnet" /grant:r "*S-1-5-18:(OI)(CI)F" "*S-1-3-0:(OI)(CI)F" "*S-1-5-32-544:(OI)(CI)F" /inheritance:r

nssm install husarnet "%PROGRAMFILES%\Husarnet\bin\husarnet.exe"
nssm set husarnet Application "%PROGRAMFILES%\Husarnet\bin\husarnet.exe"
nssm set husarnet AppParameters "daemon"
nssm set husarnet AppDirectory "%PROGRAMFILES%\Husarnet"
nssm set husarnet AppNoConsole 1
nssm set husarnet DisplayName "Husarnet"

nssm set husarnet AppRotateOnline 1
nssm set husarnet AppRotateFiles 1
nssm set husarnet AppRotateBytes 100000
nssm set husarnet AppStdout "%PROGRAMDATA%\Husarnet\husarnet.log"
nssm set husarnet AppStderr "%PROGRAMDATA%\Husarnet\husarnet.log"

nssm set husarnet Start SERVICE_AUTO_START
nssm start husarnet

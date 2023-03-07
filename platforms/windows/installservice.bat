mkdir "%PROGRAMDATA%\Husarnet"
icacls "%PROGRAMDATA%\Husarnet" /grant:r "*S-1-5-18:(F)" "*S-1-3-0:(F)" "*S-1-5-32-544:(F)" /inheritance:r
icacls "%PROGRAMDATA%\Husarnet" /grant:r "*S-1-5-18:(OI)(CI)F" "*S-1-3-0:(OI)(CI)F" "*S-1-5-32-544:(OI)(CI)F" /inheritance:r

bin\nssm install husarnet "%PROGRAMFILES%\Husarnet\bin\husarnet-daemon.exe"
bin\nssm set husarnet Application "%PROGRAMFILES%\Husarnet\bin\husarnet-daemon.exe"
bin\nssm set husarnet AppParameters ""
bin\nssm set husarnet AppDirectory "%PROGRAMFILES%\Husarnet"
bin\nssm set husarnet AppNoConsole 1
bin\nssm set husarnet DisplayName "Husarnet"

bin\nssm set husarnet AppRotateOnline 1
bin\nssm set husarnet AppRotateFiles 1
bin\nssm set husarnet AppRotateBytes 100000
bin\nssm set husarnet AppRotateBytes 1000000
bin\nssm set husarnet AppRotateSeconds 86400
bin\nssm set husarnet AppStdout "%PROGRAMDATA%\Husarnet\husarnet.log"
bin\nssm set husarnet AppStderr "%PROGRAMDATA%\Husarnet\husarnet.log"

bin\nssm set husarnet Start SERVICE_AUTO_START
bin\nssm start husarnet

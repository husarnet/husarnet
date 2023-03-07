Copy-Item ..\..\build\release\husarnet-daemon-windows-win64.exe -Destination C:\Program Files\Husarnet\husarnet-daemon.exe -Force
Copy-Item ..\..\build\release\husarnet-windows-win64.exe -Destination C:\Program Files\Husarnet\husarnet.exe -Force
Restart-Service -Name husarnet -Force
husarnet join $joincode
husarnet status | Select-String 'Is joined?                 yes' -Quiet -SimpleMatch
if ($LASTEXITCODE -eq 0) {
    Write-Host "OK."
    exit 0
} else {
    Write-Host "Not ok."
    exit 1
}

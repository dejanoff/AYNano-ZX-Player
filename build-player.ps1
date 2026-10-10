param([switch]$Upload, [string]$Port = 'COM6')
$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
  $ayTools = Join-Path $PSScriptRoot '..\AY-PLAYER\.tools'
  $ayPackages = Join-Path $PSScriptRoot '..\AY-PLAYER\.arduino\data\packages\arduino\tools'
  Copy-Item '.\diagnostics\ButtonCalibration.h' '.\firmware\AYPlayerNano\ButtonCalibration.h'
  & "$ayTools\arduino-cli.exe" --config-file "$ayTools\arduino-cli.yaml" compile --fqbn arduino:avr:nano:cpu=atmega328old --libraries '.\LIB\libraries' --build-path '.\build\player' '.\firmware\AYPlayerNano'
  if ($LASTEXITCODE -ne 0) { throw 'Compilation failed' }
  if ($Upload) {
    $ayPort = [System.IO.Ports.SerialPort]::new($Port,57600)
    try {
      $ayPort.Open()
      $ayPort.DtrEnable = $true
      $ayPort.RtsEnable = $true
      Start-Sleep -Milliseconds 100
      $ayPort.DtrEnable = $false
      $ayPort.RtsEnable = $false
      $ayPort.Close()
      & "$ayPackages\avrdude\8.0.0-arduino1\bin\avrdude.exe" -C "$ayPackages\avrdude\8.0.0-arduino1\etc\avrdude.conf" -p atmega328p -c arduino -x noautoreset -x attempts=3 -P $Port -b 57600 -D -U 'flash:w:build/player/AYPlayerNano.ino.hex:i'
      if ($LASTEXITCODE -ne 0) { throw 'Upload failed' }
    } finally {
      if ($ayPort.IsOpen) { $ayPort.Close() }
      $ayPort.Dispose()
    }
  }
} finally { Pop-Location }

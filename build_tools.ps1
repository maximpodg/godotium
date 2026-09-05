# Run in PowerShell from the repository root: .\build_tools.ps1
$toolsScript = Join-Path $PSScriptRoot "src/build/scripts/setup_tools.py"
python $toolsScript
if ($LASTEXITCODE -ne 0) { throw "Build tool setup failed. Python 3.11.8+ and curl are required." }
$toolsBin = Join-Path $PSScriptRoot ".tools/bin"
$env:Path = "$toolsBin;$env:Path"
Write-Host "GN and Ninja are ready in $toolsBin."

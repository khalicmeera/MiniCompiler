
$ErrorActionPreference = 'Stop'

Write-Host '[1/5] Compiling MiniCompiler...'
& g++ -std=c++17 -Wall -Wextra -Wpedantic src/main.cpp src/ir.cpp -o minic.exe
if ($LASTEXITCODE -ne 0) { throw 'Compiler build failed' }

Write-Host '[2/5] Testing basic program...'
& .\minic.exe examples/basic.mc test_basic.c
if ($LASTEXITCODE -ne 0) { throw 'Basic compilation failed' }

& gcc test_basic.c -o test_basic.exe
if ($LASTEXITCODE -ne 0) { throw 'GCC failed for basic program' }

$basicOutput = ((& .\test_basic.exe | Out-String) -replace "`r`n", "`n").Trim()
if ($basicOutput -ne "50`n15`n14") {
    throw "Unexpected basic output: $basicOutput"
}
Write-Host 'Basic program test passed.' -ForegroundColor Green

Write-Host '[3/5] Testing optimization program...'
& .\minic.exe examples/optimization.mc test_optimization.c
if ($LASTEXITCODE -ne 0) { throw 'Optimization compilation failed' }

& gcc test_optimization.c -o test_optimization.exe
if ($LASTEXITCODE -ne 0) { throw 'GCC failed for optimization program' }

$optOutput = ((& .\test_optimization.exe | Out-String) -replace "`r`n", "`n").Trim()
if ($optOutput -ne "42`n50`n100") {
    throw "Unexpected optimization output: $optOutput"
}
Write-Host 'Optimization program test passed.' -ForegroundColor Green

Write-Host '[4/5] Testing division-by-zero diagnostic...'
$divisionErrorFile = Join-Path $env:TEMP "minic_division_$PID.txt"

try {
    # Redirect stderr to a file so PowerShell does not interpret the
    # expected compiler diagnostic as a terminating native-command error.
    $process = Start-Process `
        -FilePath (Join-Path (Get-Location) 'minic.exe') `
        -ArgumentList @('examples/errors_division_by_zero.mc', 'should_not_generate.c') `
        -NoNewWindow -Wait -PassThru `
        -RedirectStandardError $divisionErrorFile `
        -RedirectStandardOutput (Join-Path $env:TEMP "minic_stdout_$PID.txt")

    $errorText = Get-Content $divisionErrorFile -Raw -ErrorAction SilentlyContinue

    if ($process.ExitCode -eq 0 -or
        $errorText -notmatch 'Compile-time error: division by zero') {
        throw "Division-by-zero test failed. Exit code: $($process.ExitCode). Error: $errorText"
    }

    Write-Host 'Division-by-zero test passed.' -ForegroundColor Green
}
finally {
    Remove-Item $divisionErrorFile -Force -ErrorAction SilentlyContinue
    Remove-Item (Join-Path $env:TEMP "minic_stdout_$PID.txt") -Force -ErrorAction SilentlyContinue
}

Write-Host '[5/5] Testing undeclared-variable diagnostic...'
$undeclaredErrorFile = Join-Path $env:TEMP "minic_undeclared_$PID.txt"
$undeclaredStdoutFile = Join-Path $env:TEMP "minic_undeclared_stdout_$PID.txt"

try {
    $process = Start-Process `
        -FilePath (Join-Path (Get-Location) 'minic.exe') `
        -ArgumentList @('examples/errors_undeclared.mc', 'should_not_generate.c') `
        -NoNewWindow -Wait -PassThru `
        -RedirectStandardError $undeclaredErrorFile `
        -RedirectStandardOutput $undeclaredStdoutFile

    $errorText = Get-Content $undeclaredErrorFile -Raw -ErrorAction SilentlyContinue

    if ($process.ExitCode -eq 0 -or $errorText -notmatch 'undeclared variable') {
        throw "Undeclared-variable test failed. Exit code: $($process.ExitCode). Error: $errorText"
    }

    Write-Host 'Undeclared-variable test passed.' -ForegroundColor Green
}
finally {
    Remove-Item $undeclaredErrorFile -Force -ErrorAction SilentlyContinue
    Remove-Item $undeclaredStdoutFile -Force -ErrorAction SilentlyContinue
}

Write-Host ''
Write-Host 'All tests passed!' -ForegroundColor Green

[CmdletBinding()]
param(
    [string]$InstallPath = (Join-Path $env:USERPROFILE 'actions-runner-rustium'),
    [switch]$InstallAtLogon,
    [switch]$CheckOnce
)

$ErrorActionPreference = 'Stop'
$target = (Resolve-Path -LiteralPath $InstallPath).Path
$registration = Get-Content -LiteralPath (Join-Path $target '.runner') -Raw | ConvertFrom-Json
if ($env:COMPUTERNAME -ne 'HQC-VAN_KALIBER' -or
    $registration.gitHubUrl.TrimEnd('/') -ne 'https://github.com/manvankaliber/Rustium' -or
    $registration.agentName -ne 'HQC-VAN_KALIBER-rustium') {
    throw 'This launcher requires the registered Rustium laptop runner.'
}
if (Test-Path -LiteralPath (Join-Path $target '.service')) {
    throw 'Use the installed Windows service to manage this runner.'
}
$startup = [Environment]::GetFolderPath('Startup')
$shortcut = Join-Path $startup 'Rustium build runner.lnk'
$installedScript = Join-Path $target 'Start-WindowsRunner.ps1'
$powershell = Join-Path $env:SystemRoot 'System32\WindowsPowerShell\v1.0\powershell.exe'
if ($InstallAtLogon) {
    if ($PSCommandPath -ne $installedScript) {
        Copy-Item -LiteralPath $PSCommandPath -Destination $installedScript
    }
    $arguments = '-NoProfile -NonInteractive -WindowStyle Hidden -File "{0}" -InstallPath "{1}"' -f $installedScript, $target
    $link = (New-Object -ComObject WScript.Shell).CreateShortcut($shortcut)
    $link.TargetPath = $powershell
    $link.Arguments = $arguments
    $link.WorkingDirectory = $target
    $link.Save()
    Start-Process -FilePath $powershell -ArgumentList $arguments -WorkingDirectory $target -WindowStyle Hidden
    return
}
$listener = Join-Path $target 'bin\Runner.Listener.exe'
$worker = Join-Path $target 'bin\Runner.Worker.exe'
function Ensure-Runner {
    if ($script:launcher -and -not $script:launcher.HasExited) { return 'launcher active' }
    $process = Get-CimInstance Win32_Process -Filter "Name='Runner.Listener.exe' OR Name='Runner.Worker.exe'" |
        Where-Object { $_.ExecutablePath -eq $listener -or $_.ExecutablePath -eq $worker }
    if ($process) { return 'already running' }
    if (-not (Test-Path -LiteralPath (Join-Path $target 'run.cmd') -PathType Leaf)) {
        throw 'Registered runner launcher is missing.'
    }
    $script:launcher = Start-Process -FilePath $env:ComSpec -ArgumentList '/d /c .\run.cmd' -WorkingDirectory $target -WindowStyle Hidden -PassThru
    return 'launched'
}
$mutex = New-Object Threading.Mutex($false, 'Local\RustiumBuildRunnerSupervisor')
try {
    try { $owner = $mutex.WaitOne(0) }
    catch [Threading.AbandonedMutexException] { $owner = $true }
    if (-not $owner) { Write-Output 'Supervisor already active.'; return }
    if ($CheckOnce) { Ensure-Runner; return }
    # Removing the startup shortcut stops supervision without stopping an active job.
    while (Test-Path -LiteralPath $shortcut -PathType Leaf) {
        if (Test-Path -LiteralPath (Join-Path $target '.service')) { break }
        try { $state = Ensure-Runner } catch { $state = $_.Exception.Message }
        if ($state -ne $previousState) {
            @{ utc = [DateTime]::UtcNow.ToString('o'); state = $state; supervisor_pid = $PID } |
                ConvertTo-Json | Set-Content -LiteralPath (Join-Path $target 'supervisor-status.json') -Encoding UTF8
            $previousState = $state
        }
        Start-Sleep -Seconds 60
    }
} finally {
    if ($owner) { $mutex.ReleaseMutex() }
    $mutex.Dispose()
}

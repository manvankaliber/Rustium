[CmdletBinding()]
param(
    [string]$InstallPath = 'C:\actions-runner',
    [string]$RunnerName = "$env:COMPUTERNAME-rustium",
    [string]$RegistrationTokenFile,
    [switch]$PrepareOnly,
    [switch]$RunAsService
)

$ErrorActionPreference = 'Stop'
$runnerVersion = '2.338.2'
$packageHash = 'a1f17aa987fc8158bc7521d284912267a0d01b7c2c4684b33c86783b5e8221cc'
$repositoryUrl = 'https://github.com/manvankaliber/Rustium'
$packageUrl = "https://github.com/actions/runner/releases/download/v$runnerVersion/actions-runner-win-x64-$runnerVersion.zip"

if (-not [Environment]::Is64BitOperatingSystem -or $env:OS -ne 'Windows_NT') {
    throw 'This package requires x64 Windows.'
}
$target = [IO.Path]::GetFullPath($InstallPath)
if ($target -eq [IO.Path]::GetPathRoot($target)) {
    throw 'Choose a dedicated runner directory, not a drive root.'
}
if (Test-Path -LiteralPath (Join-Path $target '.runner')) {
    throw 'A runner is already configured here. Inspect it before changing registration.'
}
$marker = Join-Path $target 'rustium-install.json'
if (Test-Path -LiteralPath $target) {
    if (-not (Test-Path -LiteralPath $marker)) {
        if (@(Get-ChildItem -LiteralPath $target -Force).Count -ne 0) {
            throw 'The existing directory is not an empty or identified Rustium runner installation.'
        }
    } else {
        $prior = Get-Content -LiteralPath $marker -Raw | ConvertFrom-Json
        if ($prior.repository -ne $repositoryUrl -or $prior.package_sha256 -ne $packageHash) {
            throw 'Existing installer identity differs; reconcile it before proceeding.'
        }
    }
} else {
    New-Item -ItemType Directory -Path $target | Out-Null
}
if ($RunAsService) {
    $principal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
    if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
        throw 'Service installation requires an elevated PowerShell session on the builder.'
    }
}

$package = Join-Path $target "actions-runner-win-x64-$runnerVersion.zip"
if (-not (Test-Path -LiteralPath $package)) {
    Invoke-WebRequest -Uri $packageUrl -OutFile $package -UseBasicParsing
}
if ((Get-FileHash -LiteralPath $package -Algorithm SHA256).Hash.ToLowerInvariant() -ne $packageHash) {
    throw 'Runner package SHA-256 differs from the official pinned release.'
}
@{ repository = $repositoryUrl; version = $runnerVersion; package_sha256 = $packageHash; source = $packageUrl } |
    ConvertTo-Json | Set-Content -LiteralPath $marker -Encoding UTF8
if (-not (Test-Path -LiteralPath (Join-Path $target 'config.cmd'))) {
    Expand-Archive -LiteralPath $package -DestinationPath $target
}
if ($PrepareOnly) {
    Write-Output "Verified runner package prepared at $target; registration has not run."
    return
}
if (-not $RegistrationTokenFile -or -not (Test-Path -LiteralPath $RegistrationTokenFile -PathType Leaf)) {
    throw 'Supply the short-lived registration token through RegistrationTokenFile. Do not put tokens in chat or command text.'
}
$registrationToken = (Get-Content -LiteralPath $RegistrationTokenFile -Raw).Trim()
if (-not $registrationToken -or $registrationToken -match '\s') {
    throw 'Invalid registration token file.'
}
$arguments = @('--unattended', '--url', $repositoryUrl, '--token', $registrationToken,
               '--name', $RunnerName, '--labels', 'rustium-build', '--work', '_work')
if ($RunAsService) { $arguments += '--runasservice' }
Push-Location -LiteralPath $target
try {
    & (Join-Path $target 'config.cmd') @arguments
    if ($LASTEXITCODE -ne 0) { throw "Runner registration failed with exit code $LASTEXITCODE." }
} finally {
    $registrationToken = $null
    $arguments = $null
    Pop-Location
}
Write-Output "Runner $RunnerName registered for Rustium with label rustium-build."
if (-not $RunAsService) { Write-Output "Start it on the builder with: & '$target\run.cmd'" }

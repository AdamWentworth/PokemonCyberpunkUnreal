[CmdletBinding()]
param(
    [ValidateSet('open', 'build', 'generate', 'doctor')]
    [string]$Action = 'open'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if ($env:OS -ne 'Windows_NT') {
    throw 'This script requires Windows. On macOS, use Scripts/Unreal.sh.'
}

$projectDir = Split-Path -Parent $PSScriptRoot
$project = Join-Path $projectDir 'PokemonCyberpunk.uproject'
$association = (Get-Content -LiteralPath $project -Raw | ConvertFrom-Json).EngineAssociation
if ($association -notmatch '^\d+\.\d+$') {
    throw 'PokemonCyberpunk.uproject must use a portable major.minor EngineAssociation, not a local GUID.'
}

function Get-EngineVersion([string]$Root) {
    $file = Join-Path $Root 'Engine\Build\Build.version'
    if (Test-Path -LiteralPath $file -PathType Leaf) {
        return Get-Content -LiteralPath $file -Raw | ConvertFrom-Json
    }
    return $null
}

$engineRoot = $env:UE_ENGINE_ROOT
if ([string]::IsNullOrWhiteSpace($engineRoot)) {
    $candidates = @()
    # Launcher installations may be on any drive.
    $manifest = Join-Path $env:ProgramData 'Epic\UnrealEngineLauncher\LauncherInstalled.dat'
    if (Test-Path -LiteralPath $manifest -PathType Leaf) {
        $data = Get-Content -LiteralPath $manifest -Raw | ConvertFrom-Json
        $candidates += @($data.InstallationList |
            Where-Object { $_.AppName -eq "UE_$association" } |
            ForEach-Object { $_.InstallLocation })
    }
    $registered = Get-ItemProperty -LiteralPath "HKLM:\SOFTWARE\EpicGames\Unreal Engine\$association" -ErrorAction SilentlyContinue
    if ($null -ne $registered -and $registered.PSObject.Properties['InstalledDirectory']) {
        $candidates += $registered.InstalledDirectory
    }
    foreach ($folder in @("UE_$association", "UE_$association.0", "UE_$association.0_SI")) {
        $candidates += Join-Path $env:ProgramFiles "Epic Games\$folder"
    }
    foreach ($candidate in $candidates) {
        if ([string]::IsNullOrWhiteSpace($candidate)) { continue }
        $candidateVersion = Get-EngineVersion $candidate
        if ($null -ne $candidateVersion -and
            "$($candidateVersion.MajorVersion).$($candidateVersion.MinorVersion)" -eq $association) {
            $engineRoot = $candidate
            break
        }
    }
}
if ([string]::IsNullOrWhiteSpace($engineRoot)) {
    throw "UE $association.x not found. Set UE_ENGINE_ROOT to the installation folder containing Engine\."
}
$version = Get-EngineVersion $engineRoot
if ($null -eq $version) {
    throw "No Engine\Build\Build.version at $engineRoot. Correct UE_ENGINE_ROOT."
}
$engineRoot = (Resolve-Path -LiteralPath $engineRoot).Path
$versionNumber = "$($version.MajorVersion).$($version.MinorVersion).$($version.PatchVersion)"
if ("$($version.MajorVersion).$($version.MinorVersion)" -ne $association) {
    throw "UE $association.x is required by PokemonCyberpunk.uproject; found $versionNumber at $engineRoot."
}

$buildTool = Join-Path $engineRoot 'Engine\Build\BatchFiles\Build.bat'
$editor = Join-Path $engineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
foreach ($required in @($buildTool, $editor)) {
    if (!(Test-Path -LiteralPath $required -PathType Leaf)) {
        throw "Incomplete Windows engine installation: missing $required."
    }
}
Write-Host "Project: $project"
Write-Host "Using Unreal Engine $versionNumber (association $association): $engineRoot"
if ($Action -eq 'doctor') {
    Write-Host 'Engine and project checks passed. Run build to validate the native compiler and SDK.'
    exit 0
}
if ($Action -eq 'generate') {
    & $buildTool -projectfiles "-Project=$project" -game -engine
    exit $LASTEXITCODE
}

# Build the native editor module before opening this checkout directly.
& $buildTool PokemonCyberpunkEditor Win64 Development "-Project=$project" -WaitMutex -NoHotReloadFromIDE
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
if ($Action -eq 'open') {
    Start-Process -FilePath $editor -ArgumentList ('"{0}"' -f $project)
}

param(
    [ValidateSet("Resolve", "Build", "Launch", "BuildAndLaunch")]
    [string]$Action = "Build",

    [ValidateSet("Development", "DebugGame", "Shipping")]
    [string]$Configuration = "Development",

    [string]$Platform = "Win64",
    [string]$Project = "",
    [string]$EngineRoot = "",
    [string]$ExtraEditorArgs = "-log",
    [switch]$NoUBA,
    [switch]$Wait
)

$ErrorActionPreference = "Stop"

function Get-ProjectPath {
    param([string]$ExplicitProject)

    if ($ExplicitProject) {
        return (Resolve-Path -LiteralPath $ExplicitProject).Path
    }

    $Root = Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..")
    $Projects = Get-ChildItem -LiteralPath $Root -Filter "*.uproject" -File
    if ($Projects.Count -ne 1) {
        throw "Expected exactly one .uproject under $Root, found $($Projects.Count). Pass -Project explicitly."
    }

    return $Projects[0].FullName
}

function Get-EngineAssociation {
    param([string]$ProjectPath)

    $Json = Get-Content -LiteralPath $ProjectPath -Raw | ConvertFrom-Json
    if (-not $Json.EngineAssociation) {
        throw "No EngineAssociation found in $ProjectPath."
    }

    return [string]$Json.EngineAssociation
}

function Test-EngineRoot {
    param([string]$Root)

    if (-not $Root) {
        return $false
    }

    $Editor = Join-Path $Root "Engine\Binaries\Win64\UnrealEditor.exe"
    $BuildBat = Join-Path $Root "Engine\Build\BatchFiles\Build.bat"
    return (Test-Path -LiteralPath $Editor) -and (Test-Path -LiteralPath $BuildBat)
}

function Resolve-EngineRoot {
    param(
        [string]$ProjectPath,
        [string]$ExplicitEngineRoot
    )

    $Association = Get-EngineAssociation -ProjectPath $ProjectPath
    $ProjectRoot = Split-Path -Parent $ProjectPath
    $Candidates = New-Object System.Collections.Generic.List[string]

    if ($ExplicitEngineRoot) {
        $Candidates.Add($ExplicitEngineRoot)
    }

    if ($env:UE_ROOT) {
        $Candidates.Add($env:UE_ROOT)
    }

    $Solution = Join-Path $ProjectRoot "SGS.sln"
    if (Test-Path -LiteralPath $Solution) {
        $EngineMatch = Select-String -LiteralPath $Solution -Pattern "(?<path>.*?UE_$([regex]::Escape($Association))\\Engine)\\" | Select-Object -First 1
        if ($EngineMatch -and $EngineMatch.Matches.Count -gt 0) {
            $RelativeEnginePath = $EngineMatch.Matches[0].Groups["path"].Value
            $RelativeEnginePath = $RelativeEnginePath.Trim('"')
            $ResolvedEngine = Resolve-Path -LiteralPath (Join-Path $ProjectRoot $RelativeEnginePath) -ErrorAction SilentlyContinue
            if ($ResolvedEngine) {
                $Candidates.Add((Split-Path -Parent $ResolvedEngine.Path))
            }
        }
    }

    $CommonRoots = @(
        "C:\Program Files\Epic Games\UE_$Association",
        "D:\Program Files\Epic Games\UE_$Association",
        "E:\Program Files\Epic Games\UE_$Association",
        "D:\Epic Games\UE_$Association"
    )

    foreach ($Root in $CommonRoots) {
        $Candidates.Add($Root)
    }

    foreach ($Candidate in $Candidates) {
        $Resolved = Resolve-Path -LiteralPath $Candidate -ErrorAction SilentlyContinue
        if ($Resolved -and (Test-EngineRoot -Root $Resolved.Path)) {
            return $Resolved.Path
        }
    }

    throw "Unable to locate Unreal Engine $Association. Set UE_ROOT or pass -EngineRoot."
}

function Get-TargetName {
    param([string]$ProjectPath)

    $ProjectRoot = Split-Path -Parent $ProjectPath
    $EditorTarget = Get-ChildItem -LiteralPath (Join-Path $ProjectRoot "Source") -Filter "*Editor.Target.cs" -File | Select-Object -First 1
    if (-not $EditorTarget) {
        throw "Could not find Source/*Editor.Target.cs."
    }

    return [System.IO.Path]::GetFileNameWithoutExtension([System.IO.Path]::GetFileNameWithoutExtension($EditorTarget.Name))
}

function Invoke-Build {
    param(
        [string]$Root,
        [string]$ProjectPath,
        [string]$TargetName,
        [string]$BuildPlatform,
        [string]$BuildConfiguration,
        [bool]$DisableUBA
    )

    $BuildBat = Join-Path $Root "Engine\Build\BatchFiles\Build.bat"
    $Args = @(
        $TargetName,
        $BuildPlatform,
        $BuildConfiguration,
        "-Project=`"$ProjectPath`"",
        "-WaitMutex",
        "-FromMsBuild"
    )
    if ($DisableUBA) {
        $Args += "-NoUBA"
    }

    Write-Host "Building $TargetName $BuildPlatform $BuildConfiguration"
    Write-Host "$BuildBat $($Args -join ' ')"

    & $BuildBat @Args
    if ($LASTEXITCODE -ne 0) {
        throw "Unreal build failed with exit code $LASTEXITCODE."
    }
}

function Invoke-Editor {
    param(
        [string]$Root,
        [string]$ProjectPath,
        [string]$EditorConfiguration,
        [string]$EditorArgs,
        [bool]$WaitForExit
    )

    $EditorExe = Join-Path $Root "Engine\Binaries\Win64\UnrealEditor.exe"
    if ($EditorConfiguration -eq "DebugGame") {
        $DebugGameEditor = Join-Path $Root "Engine\Binaries\Win64\UnrealEditor-Win64-DebugGame.exe"
        if (Test-Path -LiteralPath $DebugGameEditor) {
            $EditorExe = $DebugGameEditor
        }
    }

    $Arguments = @("`"$ProjectPath`"")
    if ($EditorArgs) {
        $Arguments += $EditorArgs
    }

    Write-Host "Launching $EditorExe"
    Write-Host "Args: $($Arguments -join ' ')"

    $Process = Start-Process -FilePath $EditorExe -ArgumentList $Arguments -WorkingDirectory (Split-Path -Parent $ProjectPath) -PassThru
    if ($WaitForExit) {
        $Process.WaitForExit()
        exit $Process.ExitCode
    }
}

$ProjectPath = Get-ProjectPath -ExplicitProject $Project
$EngineRootPath = Resolve-EngineRoot -ProjectPath $ProjectPath -ExplicitEngineRoot $EngineRoot
$TargetName = Get-TargetName -ProjectPath $ProjectPath

Write-Host "Project: $ProjectPath"
Write-Host "EngineRoot: $EngineRootPath"
Write-Host "EditorTarget: $TargetName"

switch ($Action) {
    "Resolve" {
        return
    }
    "Build" {
        Invoke-Build -Root $EngineRootPath -ProjectPath $ProjectPath -TargetName $TargetName -BuildPlatform $Platform -BuildConfiguration $Configuration -DisableUBA:$NoUBA.IsPresent
    }
    "Launch" {
        Invoke-Editor -Root $EngineRootPath -ProjectPath $ProjectPath -EditorConfiguration $Configuration -EditorArgs $ExtraEditorArgs -WaitForExit:$Wait.IsPresent
    }
    "BuildAndLaunch" {
        Invoke-Build -Root $EngineRootPath -ProjectPath $ProjectPath -TargetName $TargetName -BuildPlatform $Platform -BuildConfiguration $Configuration -DisableUBA:$NoUBA.IsPresent
        Invoke-Editor -Root $EngineRootPath -ProjectPath $ProjectPath -EditorConfiguration $Configuration -EditorArgs $ExtraEditorArgs -WaitForExit:$Wait.IsPresent
    }
}

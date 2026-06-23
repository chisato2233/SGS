param(
    [ValidateSet("Staged", "Tracked", "WorkingTree")]
    [string]$Scope = "Staged",

    [long]$ThresholdBytes = 52428800,

    [switch]$Fix
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Invoke-Git {
    param(
        [Parameter(Mandatory = $true)]
        [string[]]$Arguments
    )

    $Output = & git -c core.quotePath=false @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "git $($Arguments -join ' ') failed with exit code $LASTEXITCODE"
    }

    return $Output
}

function Format-ByteSize {
    param([long]$Bytes)

    if ($Bytes -ge 1GB) {
        return "{0:N2} GiB" -f ($Bytes / 1GB)
    }

    if ($Bytes -ge 1MB) {
        return "{0:N2} MiB" -f ($Bytes / 1MB)
    }

    if ($Bytes -ge 1KB) {
        return "{0:N2} KiB" -f ($Bytes / 1KB)
    }

    return "$Bytes B"
}

function Get-RepoRelativePath {
    param([string]$Path)

    return ($Path -replace "\\", "/")
}

function Get-CandidatePaths {
    switch ($Scope) {
        "Staged" {
            return @(Invoke-Git @("diff", "--cached", "--name-only", "--diff-filter=ACMR"))
        }
        "Tracked" {
            return @(Invoke-Git @("ls-files"))
        }
        "WorkingTree" {
            $Tracked = @(Invoke-Git @("ls-files"))
            $Untracked = @(Invoke-Git @("ls-files", "--others", "--exclude-standard"))
            return @($Tracked + $Untracked | Sort-Object -Unique)
        }
    }
}

function Test-LfsAttribute {
    param([string]$Path)

    $Output = & git -c core.quotePath=false check-attr filter -- $Path
    if ($LASTEXITCODE -ne 0) {
        throw "git check-attr failed for $Path"
    }

    return (($Output -join "`n") -match ":\s+filter:\s+lfs$")
}

function Test-StagedLfsPointer {
    param([string]$Path)

    $ObjectSpec = ":$Path"
    $SizeText = & git -c core.quotePath=false cat-file -s $ObjectSpec 2>$null
    if ($LASTEXITCODE -ne 0) {
        return $false
    }

    $BlobSize = [long]($SizeText | Select-Object -First 1)
    if ($BlobSize -gt 1024) {
        return $false
    }

    $Blob = & git -c core.quotePath=false cat-file -p $ObjectSpec
    if ($LASTEXITCODE -ne 0) {
        return $false
    }

    $Text = ($Blob -join "`n")
    return (
        $Text -match "^version https://git-lfs.github.com/spec/v1" -and
        $Text -match "(?m)^oid sha256:[0-9a-f]{64}$" -and
        $Text -match "(?m)^size \d+$"
    )
}

$RepoRoot = (Invoke-Git @("rev-parse", "--show-toplevel") | Select-Object -First 1)
Set-Location -LiteralPath $RepoRoot

$ThresholdLabel = Format-ByteSize -Bytes $ThresholdBytes
$Paths = @(Get-CandidatePaths | Where-Object { $_ } | ForEach-Object { Get-RepoRelativePath $_ })

if ($Paths.Count -eq 0) {
    Write-Host "Large-file LFS check: no $Scope files to inspect. Threshold: $ThresholdLabel."
    exit 0
}

try {
    & git -c core.quotePath=false lfs version | Out-Null
    if ($LASTEXITCODE -ne 0) {
        throw "git lfs version failed"
    }
}
catch {
    Write-Error "Git LFS is not available. Install Git LFS before committing files that may cross $ThresholdLabel."
    exit 1
}

$LargeFiles = @()
foreach ($Path in $Paths) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        continue
    }

    $Item = Get-Item -LiteralPath $Path
    if ($Item.Length -ge $ThresholdBytes) {
        $LargeFiles += [pscustomobject]@{
            Path = $Path
            Size = $Item.Length
            SizeLabel = Format-ByteSize -Bytes $Item.Length
            HasLfsAttribute = Test-LfsAttribute -Path $Path
            IsStagedPointer = if ($Scope -eq "Staged") { Test-StagedLfsPointer -Path $Path } else { $false }
        }
    }
}

if ($LargeFiles.Count -eq 0) {
    Write-Host "Large-file LFS check: OK. Checked $($Paths.Count) $Scope file(s); none are >= $ThresholdLabel."
    exit 0
}

if ($Fix) {
    foreach ($File in $LargeFiles) {
        if (-not $File.HasLfsAttribute) {
            Write-Host "Adding explicit LFS rule for $($File.Path) ($($File.SizeLabel))"
            & git -c core.quotePath=false lfs track --filename $($File.Path) | Out-Null
            if ($LASTEXITCODE -ne 0) {
                throw "git lfs track failed for $($File.Path)"
            }
        }
    }

    & git -c core.quotePath=false add .gitattributes
    if ($LASTEXITCODE -ne 0) {
        throw "git add .gitattributes failed"
    }

    foreach ($File in $LargeFiles) {
        & git -c core.quotePath=false add -- $($File.Path)
        if ($LASTEXITCODE -ne 0) {
            throw "git add failed for $($File.Path)"
        }
    }

    Write-Host "Large-file LFS check: fixed and re-staged large file(s). Re-running check..."
    & powershell -NoProfile -ExecutionPolicy Bypass -File $PSCommandPath -Scope $Scope -ThresholdBytes $ThresholdBytes
    exit $LASTEXITCODE
}

$Failures = @()
foreach ($File in $LargeFiles) {
    $IsOk = $File.HasLfsAttribute
    if ($Scope -eq "Staged") {
        $IsOk = $IsOk -and $File.IsStagedPointer
    }

    if (-not $IsOk) {
        $Failures += $File
    }
}

if ($Failures.Count -eq 0) {
    Write-Host "Large-file LFS check: OK. $($LargeFiles.Count) large file(s) are covered by LFS."
    exit 0
}

Write-Error "Large-file LFS check failed. Files >= $ThresholdLabel must be stored with Git LFS."
foreach ($File in $Failures) {
    Write-Host (" - {0} ({1}); lfs attribute: {2}; staged pointer: {3}" -f `
        $File.Path, $File.SizeLabel, $File.HasLfsAttribute, $File.IsStagedPointer)
}

Write-Host ""
Write-Host "To fix staged files, run:"
Write-Host "  powershell -NoProfile -ExecutionPolicy Bypass -File .\Tools\CheckLargeFiles.ps1 -Scope Staged -Fix"
Write-Host ""
Write-Host "Then review .gitattributes and re-run the commit."
exit 1

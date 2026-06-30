param(
    [ValidateSet("Collapse", "Remove")]
    [string]$Mode = "Collapse",

    [string]$InputHtml = "graphify-out/graph.html",

    [string]$OutputHtml = "graphify-out/graph.filtered.html",

    [string[]]$CoreTypeLabels = @(
        "int8",
        "uint8",
        "int16",
        "uint16",
        "int32",
        "uint32",
        "int64",
        "uint64",
        "bool",
        "float",
        "double",
        "FName",
        "FString",
        "FText",
        "TArray",
        "TMap",
        "TSet",
        "TObjectPtr",
        "UObject",
        "AActor",
        "FGameplayTag",
        "FGameplayTa"
    )
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Get-ScriptArray {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Html,

        [Parameter(Mandatory = $true)]
        [string]$StartMarker,

        [Parameter(Mandatory = $true)]
        [string]$EndMarker
    )

    $Start = $Html.IndexOf($StartMarker)
    if ($Start -lt 0) {
        throw "Could not find marker: $StartMarker"
    }

    $JsonStart = $Start + $StartMarker.Length
    $EndMatch = Find-Marker -Text $Html -Marker $EndMarker -StartIndex $JsonStart
    if ($EndMatch.Index -lt 0) {
        throw "Could not find marker after $StartMarker`: $EndMarker"
    }

    return $Html.Substring($JsonStart, $EndMatch.Index - $JsonStart).Trim()
}

function Set-ScriptArray {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Html,

        [Parameter(Mandatory = $true)]
        [string]$StartMarker,

        [Parameter(Mandatory = $true)]
        [string]$EndMarker,

        [Parameter(Mandatory = $true)]
        [string]$Json
    )

    $Start = $Html.IndexOf($StartMarker)
    if ($Start -lt 0) {
        throw "Could not find marker: $StartMarker"
    }

    $JsonStart = $Start + $StartMarker.Length
    $EndMatch = Find-Marker -Text $Html -Marker $EndMarker -StartIndex $JsonStart
    if ($EndMatch.Index -lt 0) {
        throw "Could not find marker after $StartMarker`: $EndMarker"
    }

    return $Html.Substring(0, $JsonStart) + $Json + $Html.Substring($EndMatch.Index)
}

function Find-Marker {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Text,

        [Parameter(Mandatory = $true)]
        [string]$Marker,

        [int]$StartIndex = 0
    )

    $Candidates = @(
        $Marker,
        $Marker.Replace("`n", "`r`n"),
        $Marker.Replace("`r`n", "`n")
    ) | Select-Object -Unique

    foreach ($Candidate in $Candidates) {
        $Index = $Text.IndexOf($Candidate, $StartIndex)
        if ($Index -ge 0) {
            return [pscustomobject]@{
                Index = $Index
                Length = $Candidate.Length
            }
        }
    }

    return [pscustomobject]@{
        Index = -1
        Length = 0
    }
}

function ConvertTo-GraphJson {
    param([Parameter(Mandatory = $true)]$Value)

    return ($Value | ConvertTo-Json -Depth 100 -Compress)
}

function Get-NormalizedLabel {
    param([string]$Label)

    if (-not $Label) {
        return ""
    }

    $Base = $Label.Trim()
    $GenericStart = $Base.IndexOf("<")
    if ($GenericStart -ge 0) {
        $Base = $Base.Substring(0, $GenericStart)
    }

    return $Base.ToLowerInvariant()
}

function New-CollapsedEdgeKey {
    param(
        [string]$From,
        [string]$To
    )

    if ([string]::CompareOrdinal($From, $To) -le 0) {
        return "$From|$To"
    }

    return "$To|$From"
}

$InputPath = Resolve-Path -LiteralPath $InputHtml
$Html = Get-Content -LiteralPath $InputPath -Raw

$RawNodesJson = Get-ScriptArray -Html $Html -StartMarker "const RAW_NODES = " -EndMarker ";`nconst RAW_EDGES = "
$RawEdgesJson = Get-ScriptArray -Html $Html -StartMarker "const RAW_EDGES = " -EndMarker ";`nconst LEGEND = "

$Nodes = $RawNodesJson | ConvertFrom-Json
$Edges = $RawEdgesJson | ConvertFrom-Json
if ($null -eq $Nodes) {
    $Nodes = @()
}
elseif ($Nodes -isnot [array]) {
    $Nodes = @($Nodes)
}
if ($null -eq $Edges) {
    $Edges = @()
}
elseif ($Edges -isnot [array]) {
    $Edges = @($Edges)
}

$CoreTypeNames = @($CoreTypeLabels | ForEach-Object { Get-NormalizedLabel -Label $_ })

$CoreNodeIds = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
$CollapsedLabels = [System.Collections.Generic.SortedSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
foreach ($Node in $Nodes) {
    $Label = [string]$Node.label
    $Id = [string]$Node.id
    if (($CoreTypeNames -contains (Get-NormalizedLabel -Label $Label)) -or ($CoreTypeNames -contains (Get-NormalizedLabel -Label $Id))) {
        [void]$CoreNodeIds.Add($Id)
        [void]$CollapsedLabels.Add($Label)
    }
}

if ($CoreNodeIds.Count -eq 0) {
    Write-Host "Graphify visual filter: no configured core type nodes found in $InputHtml."
    Copy-Item -LiteralPath $InputPath -Destination $OutputHtml -Force
    exit 0
}

$OutputNodes = New-Object System.Collections.Generic.List[object]
foreach ($Node in $Nodes) {
    if (-not $CoreNodeIds.Contains([string]$Node.id)) {
        $OutputNodes.Add($Node)
    }
}

$OutputEdges = New-Object System.Collections.Generic.List[object]
if ($Mode -eq "Remove") {
    foreach ($Edge in $Edges) {
        if (-not $CoreNodeIds.Contains([string]$Edge.from) -and -not $CoreNodeIds.Contains([string]$Edge.to)) {
            $OutputEdges.Add($Edge)
        }
    }
}
else {
    $GroupNodeId = "ue_core_types"
    $GroupNeighbors = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
    $CollapsedEdgeKeys = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)

    foreach ($Edge in $Edges) {
        $From = [string]$Edge.from
        $To = [string]$Edge.to
        $FromIsCore = $CoreNodeIds.Contains($From)
        $ToIsCore = $CoreNodeIds.Contains($To)

        if ($FromIsCore -and $ToIsCore) {
            continue
        }

        if (-not $FromIsCore -and -not $ToIsCore) {
            $OutputEdges.Add($Edge)
            continue
        }

        $Neighbor = if ($FromIsCore) { $To } else { $From }
        if (-not $Neighbor -or $Neighbor -eq $GroupNodeId) {
            continue
        }

        [void]$GroupNeighbors.Add($Neighbor)
        $Key = New-CollapsedEdgeKey -From $GroupNodeId -To $Neighbor
        if (-not $CollapsedEdgeKeys.Add($Key)) {
            continue
        }

        $OutputEdges.Add([pscustomobject]@{
            from = $GroupNodeId
            to = $Neighbor
            label = "uses"
            title = "uses UE/Core Types [collapsed]"
            dashes = $false
            width = 1
            color = [pscustomobject]@{
                opacity = 0.35
            }
            confidence = "EXTRACTED"
        })
    }

    $OutputNodes.Add([pscustomobject]@{
        id = $GroupNodeId
        label = "UE/Core Types"
        color = [pscustomobject]@{
            background = "#6B7280"
            border = "#4B5563"
            highlight = [pscustomobject]@{
                background = "#ffffff"
                border = "#4B5563"
            }
        }
        size = 18
        font = [pscustomobject]@{
            size = 12
            color = "#ffffff"
        }
        title = "Collapsed core type nodes: $($CollapsedLabels -join ', ')"
        community = -1
        community_name = "UE/Core Types"
        source_file = ""
        file_type = "code"
        degree = $GroupNeighbors.Count
    })
}

$FilteredNodeIds = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
foreach ($Node in $OutputNodes) {
    [void]$FilteredNodeIds.Add([string]$Node.id)
}

$FinalEdges = @($OutputEdges | Where-Object {
    $FilteredNodeIds.Contains([string]$_.from) -and $FilteredNodeIds.Contains([string]$_.to)
})

$OutputNodesJson = ConvertTo-GraphJson -Value $OutputNodes.ToArray()
$FinalEdgesJson = ConvertTo-GraphJson -Value @($FinalEdges)

$FilteredHtml = Set-ScriptArray -Html $Html -StartMarker "const RAW_NODES = " -EndMarker ";`nconst RAW_EDGES = " -Json $OutputNodesJson
$FilteredHtml = Set-ScriptArray -Html $FilteredHtml -StartMarker "const RAW_EDGES = " -EndMarker ";`nconst LEGEND = " -Json $FinalEdgesJson

$StatsText = "$($OutputNodes.Count) nodes &middot; $($FinalEdges.Count) edges &middot; filtered view ($Mode UE/Core Types)"
$FilteredHtml = [regex]::Replace($FilteredHtml, '<div id="stats">.*?</div>', "<div id=`"stats`">$StatsText</div>")

$OutputDirectory = Split-Path -Parent $OutputHtml
if ($OutputDirectory -and -not (Test-Path -LiteralPath $OutputDirectory)) {
    New-Item -ItemType Directory -Path $OutputDirectory | Out-Null
}

Set-Content -LiteralPath $OutputHtml -Value $FilteredHtml -Encoding UTF8

Write-Host "Graphify visual filter: $Mode mode."
Write-Host "Input:  $InputHtml ($($Nodes.Count) nodes, $($Edges.Count) edges)"
Write-Host "Output: $OutputHtml ($($OutputNodes.Count) nodes, $($FinalEdges.Count) edges)"
Write-Host "Matched core type nodes: $($CoreNodeIds.Count) [$($CollapsedLabels -join ', ')]"

<#
.SYNOPSIS
Puts the entries of the merc JSON files in the order of the profile IDs.

.DESCRIPTION
mercs-profile-info.json and mercs-profile-names-<Language>.json are sorted by "profileID",
mercs-relations.json follows the order of the profiles in mercs-profile-info.json (its entries
are named by the internal name). The entries themselves are copied unchanged, the sort is stable.

Usage:
  powershell -ExecutionPolicy Bypass -File tools\sort-merc-profiles.ps1
#>
param(
    [string] $AssetsDir = "",
    [string] $Language = "eng"
)

$ErrorActionPreference = "Stop"
if (-not $AssetsDir) { $AssetsDir = Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) "..\assets\externalized" }
$utf8 = New-Object System.Text.UTF8Encoding($false)

# Cuts the top level objects out of a JSON array file: the text before the '[' and the object texts.
function Split-ArrayFile([string] $text) {
    $start = $text.IndexOf('[')
    $header = $text.Substring(0, $start)
    $objects = New-Object System.Collections.Generic.List[string]
    $depth = 0; $inString = $false; $escape = $false; $objStart = -1
    for ($i = $start + 1; $i -lt $text.Length; $i++) {
        $c = $text[$i]
        if ($inString) {
            if ($escape) { $escape = $false }
            elseif ($c -eq '\') { $escape = $true }
            elseif ($c -eq '"') { $inString = $false }
            continue
        }
        if ($c -eq '"') { $inString = $true }
        elseif ($c -eq '{') { if ($depth -eq 0) { $objStart = $i }; $depth++ }
        elseif ($c -eq '}') { $depth--; if ($depth -eq 0) { $objects.Add($text.Substring($objStart, $i - $objStart + 1)) } }
    }
    return @{ Header = $header; Objects = $objects }
}

function Write-ArrayFile([string] $path, [string] $header, $objects) {
    $indented = foreach ($o in $objects) { "  " + $o }
    [System.IO.File]::WriteAllText($path, $header + "[`n" + ($indented -join ",`n") + "`n]`n", $utf8)
}

# stable sort by an integer key
function Sort-Objects($objects, [scriptblock] $key) {
    $i = 0
    $rows = foreach ($o in $objects) { [pscustomobject]@{ Key = (& $key $o); Pos = $i++; Text = $o } }
    return @($rows | Sort-Object Key, Pos | ForEach-Object { $_.Text })
}

function Get-Id([string] $o) { return [int]([regex]::Match($o, '"profileID":\s*(\d+)').Groups[1].Value) }

$infoPath = Join-Path $AssetsDir "mercs-profile-info.json"
$namesPath = Join-Path $AssetsDir "mercs-profile-names-$Language.json"
$relPath = Join-Path $AssetsDir "mercs-relations.json"

$info = Split-ArrayFile ([System.IO.File]::ReadAllText($infoPath))
$sortedInfo = Sort-Objects $info.Objects { param($o) Get-Id $o }
Write-ArrayFile $infoPath $info.Header $sortedInfo

$names = Split-ArrayFile ([System.IO.File]::ReadAllText($namesPath))
Write-ArrayFile $namesPath $names.Header (Sort-Objects $names.Objects { param($o) Get-Id $o })

$idOfName = @{}
foreach ($o in $sortedInfo) { $idOfName[[regex]::Match($o, '"internalName":\s*"([^"]*)"').Groups[1].Value] = Get-Id $o }
$rel = Split-ArrayFile ([System.IO.File]::ReadAllText($relPath))
$sortedRel = Sort-Objects $rel.Objects {
    param($o)
    $n = [regex]::Match($o, '"profile":\s*"([^"]*)"').Groups[1].Value
    if ($idOfName.ContainsKey($n)) { $idOfName[$n] } else { 100000 }
}
Write-ArrayFile $relPath $rel.Header $sortedRel

Write-Host "Sorted: $($sortedInfo.Count) profiles, $($names.Objects.Count) names, $($rel.Objects.Count) relation entries"

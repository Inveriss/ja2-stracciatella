<#
.SYNOPSIS
Exports the short texts (quotes) of the mercs from the mercedt\NNN.edt files to
mercs-dialogue-<Language>.json.

.DESCRIPTION
Every NNN.edt file holds the quotes of profile NNN: rows of 240 characters, "encrypted" like
all the EDT files (the same decoding as the game does). The JSON has one entry per file:

  { "profileID": 12, "quotes": [ "text of quote 0", "text of quote 1", "", "", "text of quote 4", ... ] }

The number of the quote is its place in the list. The gaps of the file (rows without a text) are
kept as empty strings, and so is the number of rows of the file, so that a text can be written
into a gap later. Every profile gets
-MinQuotes entries (120): a shorter file is filled up with empty strings. The voice files (speech\NNN_QQQ.wav) are not part of this.

Usage:
  powershell -ExecutionPolicy Bypass -File tools\export-merc-dialogue.ps1 `
      -EdtDir "E:\path\Mercedt" -OutFile assets\externalized\mercs-dialogue-eng.json
#>
param(
    [Parameter(Mandatory = $true)] [string] $EdtDir,
    [string] $OutFile = "",
    [string] $Language = "eng",
    [int] $MinQuotes = 120
)

$ErrorActionPreference = "Stop"
if (-not $OutFile) { $OutFile = Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) "..\assets\externalized\mercs-dialogue-$Language.json" }
$utf8 = New-Object System.Text.UTF8Encoding($false)
$rowLength = 240

function Format-JsonString([string] $s) {
    $sb = New-Object System.Text.StringBuilder
    [void]$sb.Append('"')
    foreach ($c in $s.ToCharArray()) {
        switch ($c) {
            '"'  { [void]$sb.Append('\"') }
            '\'  { [void]$sb.Append('\') }
            default {
                if ([int]$c -lt 32) { [void]$sb.Append(('\u{0:x4}' -f [int]$c)) } else { [void]$sb.Append($c) }
            }
        }
    }
    [void]$sb.Append('"')
    return $sb.ToString()
}

$files = Get-ChildItem -LiteralPath $EdtDir -File | Where-Object { $_.Name -match '^\d{3}\.edt$' } | Sort-Object Name
if (-not $files) { throw "No NNN.edt file in $EdtDir" }

$entries = New-Object System.Collections.Generic.List[string]
$totalQuotes = 0; $totalTexts = 0
foreach ($f in $files) {
    $id = [int]$f.BaseName
    $bytes = [System.IO.File]::ReadAllBytes($f.FullName)
    $count = [int]($bytes.Length / 2)
    if ($count % $rowLength -ne 0) { throw "$($f.Name): the size is not a multiple of $rowLength characters" }
    $rows = $count / $rowLength

    $chars = New-Object 'char[]' $count
    for ($i = 0; $i -lt $count; $i++) {
        $c = [int]$bytes[2 * $i] + 256 * [int]$bytes[2 * $i + 1]
        if ($c -gt 33) { $c -= 1 }   # the "ROT-1" encryption
        if ($Language -eq "rus") {
            # the Russian data files were converted from CP1251 as if it were CP1252
            if ($c -ge 0xC0 -and $c -le 0xFF) { $c += 0x0350 }
        } else {
            # the English data files were converted from CP437 (a few lines of Malice)
            if ($Language -eq "eng") {
                if ($c -eq 128) { $c = 0x00C7 } elseif ($c -eq 130) { $c = 0x00E9 } elseif ($c -eq 135) { $c = 0x00E7 }
            }
            # the Cyrillic texts (by Ivan Dolvich) of the versions other than the Russian one are
            # encoded in a wild manner; the same undoing as in EncryptedString.cc
            if ($c -ge 0x044D -and $c -le 0x0452) { $c = $c - 0x044D + 0x0410 }
            elseif ($c -eq 0x0453) { $c = 0x0401 }
            elseif ($c -ge 0x0454 -and $c -le 0x0467) { $c = $c - 0x0454 + 0x0416 }
            elseif ($c -ge 0x0468 -and $c -le 0x046C) { $c = $c - 0x0468 + 0x042B }
        }
        $chars[$i] = [char]$c
    }

    $quotes = New-Object System.Collections.Generic.List[string]
    for ($r = 0; $r -lt $rows; $r++) {
        $text = (New-Object string (,$chars[($r * $rowLength)..($r * $rowLength + $rowLength - 1)])).Split([char]0)[0]
        $quotes.Add((Format-JsonString $text))
        if ($text -ne "") { $totalTexts++ }
    }
    while ($quotes.Count -lt $MinQuotes) { $quotes.Add('""') }
    $totalQuotes += $quotes.Count
    $lines = ($quotes | ForEach-Object { "      $_" }) -join ",`n"
    $entries.Add("  {`n    `"profileID`": $id,`n    `"quotes`": [`n$lines`n    ]`n  }")
}

$header = "/* The short texts of the mercs, one entry per profile: the number of a quote is its place in the list; empty strings are\n   the gaps of the original mercedt files, kept so that texts can be written into them. See tools/export-merc-dialogue.ps1. */`n"
$header = $header.Replace('\n', "`n")
[System.IO.File]::WriteAllText($OutFile, $header + "[`n" + ($entries -join ",`n") + "`n]`n", $utf8)
Write-Host "Wrote ${OutFile}: $($files.Count) profiles, $totalQuotes quotes ($totalTexts with a text)"

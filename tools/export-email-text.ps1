<#
.SYNOPSIS
Exports the texts of email.edt to strings/email-text-<Language>.json.

.DESCRIPTION
email.edt is one flat file of fixed-size rows (MAIL_STRING_SIZE, see EMail.cc), "encrypted"
like all the EDT files (the same decoding as the game does). The JSON is a flat array, one
entry per row, in the same order: the number of a text is its place in the list, matching
the offsets EMail.cc already uses to address email.edt (AddEmail()/LoadEMailText()). An
empty string means the game falls back to reading that row from email.edt directly (see
DefaultContentManager::loadEmailText()) -- so a language without an exported file, or with
fewer entries than email.edt has rows, still shows every text correctly.

Usage:
  powershell -ExecutionPolicy Bypass -File tools\export-email-text.ps1 `
      -EdtFile "E:\path\EMAIL.EDT" -OutFile assets\externalized\strings\email-text-eng.json
#>
param(
    [Parameter(Mandatory = $true)] [string] $EdtFile,
    [string] $OutFile = "",
    [string] $Language = "eng",
    [int] $RowLength = 320
)

$ErrorActionPreference = "Stop"
if (-not $OutFile) { $OutFile = Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) "..\assets\externalized\strings\email-text-$Language.json" }
$utf8 = New-Object System.Text.UTF8Encoding($false)

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

$bytes = [System.IO.File]::ReadAllBytes($EdtFile)
$count = [int]($bytes.Length / 2)
if ($count % $RowLength -ne 0) { throw "$($EdtFile): the size is not a multiple of $RowLength characters" }
$rows = $count / $RowLength

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

$lines = New-Object System.Collections.Generic.List[string]
$totalTexts = 0
for ($r = 0; $r -lt $rows; $r++) {
    $text = (New-Object string (,$chars[($r * $RowLength)..($r * $RowLength + $RowLength - 1)])).Split([char]0)[0]
    $lines.Add("  " + (Format-JsonString $text))
    if ($text -ne "") { $totalTexts++ }
}

$header = "/* The texts of email.edt, one entry per row: the number of a text is its place in the list, the same\n   offset EMail.cc uses to address email.edt. An empty string falls back to reading that row from\n   email.edt directly. See tools/export-email-text.ps1. */`n"
$header = $header.Replace('\n', "`n")
[System.IO.File]::WriteAllText($OutFile, $header + "[`n" + ($lines -join ",`n") + "`n]`n", $utf8)
Write-Host "Wrote ${OutFile}: $rows texts ($totalTexts with a text)"

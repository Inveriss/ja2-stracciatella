<#
.SYNOPSIS
Converts profiles from a JA2 1.13 style MercProfiles.xml into this project's merc JSON files.

.DESCRIPTION
For every <PROFILE> with an <uiIndex> in [FirstId, LastId] and a merc type (1 = AIM, 2 = MERC in
the XML; both become "AIM" here) the script writes

  * an entry in mercs-profile-info.json (everything except the names),
  * an entry in mercs-profile-names-<Language>.json (full name and nickname),
  * an entry in mercs-relations.json (friends and enemies that can be expressed).

Entries of these IDs that are already in the files are replaced, so the script can be run again.
Existing entries of other profiles are copied through untouched.

Only what has a counterpart in this project is converted:
  - inventory: the XML has none, nothing is written
  - bDisability 1-7 -> personalityTrait; the STOMP traits (character trait, new skill traits) are skipped
  - bOldSkillTrait/bOldSkillTrait2 -> skillTrait/skillTrait2
  - bAttitude -> attitude; bSexist -> sexismMode
  - friends/enemies with an ID above 127 cannot be stored (INT8) and are skipped

Usage:
  powershell -ExecutionPolicy Bypass -File tools\convert-merc-profiles-xml.ps1 `
      -XmlPath "E:\path\MercProfiles.xml" -FirstId 165 -LastId 199
#>
param(
    [Parameter(Mandatory = $true)] [string] $XmlPath,
    [int] $FirstId = 165,
    [int] $LastId = 199,
    [string] $AssetsDir = "",
    [string] $Language = "eng"
)

$ErrorActionPreference = "Stop"
if (-not $AssetsDir) { $AssetsDir = Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) "..\assets\externalized" }
$utf8 = New-Object System.Text.UTF8Encoding($false)

# ---- name tables (the numbers are the same in the XML and in this project) ---------------------
$bodyTypes  = @("REGMALE", "BIGMALE", "STOCKYMALE", "REGFEMALE")
$attitudes  = @("NORMAL", "FRIENDLY", "LONER", "OPTIMIST", "PESSIMIST", "AGGRESSIVE", "ARROGANT", "BIG_SHOT", "ASSHOLE", "COWARD")
$personality = @("NONE", "HEAT_INTOLERANT", "NERVOUS", "CLAUSTROPHOBIC", "NONSWIMMER", "FEAR_OF_INSECTS", "FORGETFUL", "PSYCHO")
$skills     = @("NONE", "LOCKPICKING", "HANDTOHAND", "ELECTRONICS", "NIGHTOPS", "THROWING", "TEACHING", "HEAVY_WEAPS",
                "AUTO_WEAPS", "STEALTHY", "AMBIDEXT", "THIEF", "MARTIALARTS", "KNIFING", "ONROOF", "CAMOUFLAGED")
$sexism     = @($null, "SOMEWHAT_SEXIST", "VERY_SEXIST", "GENTLEMAN")

$report = New-Object System.Collections.Generic.List[string]
function Note($text) { $script:report.Add($text) }

# ---- JSON output helpers (2 space indentation, like the files this project writes) -------------
function Format-JsonString([string] $s) {
    $sb = New-Object System.Text.StringBuilder
    [void]$sb.Append('"')
    foreach ($c in $s.ToCharArray()) {
        switch ($c) {
            '"'  { [void]$sb.Append('\"') }
            '\'  { [void]$sb.Append('\\') }
            "`n" { [void]$sb.Append('\n') }
            "`r" { [void]$sb.Append('\r') }
            "`t" { [void]$sb.Append('\t') }
            default {
                if ([int]$c -lt 32) { [void]$sb.Append(('\u{0:x4}' -f [int]$c)) } else { [void]$sb.Append($c) }
            }
        }
    }
    [void]$sb.Append('"')
    return $sb.ToString()
}

function ConvertTo-Json2($value, [int] $depth) {
    $pad = "  " * $depth
    $padIn = "  " * ($depth + 1)
    if ($value -is [System.Collections.IDictionary]) {
        $parts = foreach ($k in $value.Keys) { "$padIn$(Format-JsonString $k): $(ConvertTo-Json2 $value[$k] ($depth + 1))" }
        return "{`n" + ($parts -join ",`n") + "`n$pad}"
    }
    if ($value -is [System.Array]) {
        $parts = foreach ($v in $value) { "$padIn$(ConvertTo-Json2 $v ($depth + 1))" }
        return "[`n" + ($parts -join ",`n") + "`n$pad]"
    }
    if ($value -is [bool]) { return $(if ($value) { "true" } else { "false" }) }
    if ($value -is [string]) { return Format-JsonString $value }
    return [string]$value
}

# Cuts the top level objects out of a JSON array file. Returns the text before the '[' (comments)
# and the object texts, unchanged.
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
    $indented = foreach ($o in $objects) { "  " + ($o -replace "`n", "`n") }
    $text = $header + "[`n" + ($indented -join ",`n") + "`n]`n"
    [System.IO.File]::WriteAllText($path, $text, $utf8)
}

# ---- read the files this script updates -------------------------------------------------------
$infoPath = Join-Path $AssetsDir "mercs-profile-info.json"
$namesPath = Join-Path $AssetsDir "mercs-profile-names-$Language.json"
$relPath = Join-Path $AssetsDir "mercs-relations.json"

$info = Split-ArrayFile ([System.IO.File]::ReadAllText($infoPath))
$names = Split-ArrayFile ([System.IO.File]::ReadAllText($namesPath))
$rel = Split-ArrayFile ([System.IO.File]::ReadAllText($relPath))

# internal names and IDs already in use by the profiles that stay
$internalNames = @{}      # internal name -> id
$idToName = @{}
$keptInfo = New-Object System.Collections.Generic.List[string]
$removedNames = @{}
foreach ($o in $info.Objects) {
    $id = [int]([regex]::Match($o, '"profileID":\s*(\d+)').Groups[1].Value)
    $name = [regex]::Match($o, '"internalName":\s*"([^"]*)"').Groups[1].Value
    if ($id -ge $FirstId -and $id -le $LastId) { $removedNames[$name] = $true; continue }
    $keptInfo.Add($o)
    $internalNames[$name] = $id
    $idToName[$id] = $name
}

# ---- read the XML ------------------------------------------------------------------------------
[xml] $xml = [System.IO.File]::ReadAllText($XmlPath, $utf8)
$profiles = @{}
foreach ($p in $xml.SelectNodes("//PROFILE")) {
    $id = [int]$p.uiIndex
    if ($id -ge $FirstId -and $id -le $LastId) { $profiles[$id] = $p }
}
if ($profiles.Count -eq 0) { throw "No profile with an index from $FirstId to $LastId in $XmlPath" }

# The XML adapter of PowerShell returns the text of a leaf element as a string.
function Text($node) { if ($node -eq $null) { return "" } elseif ($node -is [System.Xml.XmlNode]) { return $node.InnerText } else { return [string]$node } }
function Int($node, $default = 0) { $s = (Text $node).Trim(); if ($s -eq "") { return $default } else { return [int]$s } }

$newInfo = New-Object System.Collections.Generic.List[string]
$newNames = New-Object System.Collections.Generic.List[string]
$converted = @()

foreach ($id in ($profiles.Keys | Sort-Object)) {
    $p = $profiles[$id]
    $type = Int $p.Type
    $nick = Text $p.zNickname
    if ($type -ne 1 -and $type -ne 2) {
        Note "skipped $id '$nick': type $type is not a merc (1 = AIM, 2 = MERC)"
        continue
    }

    # internal name: the nickname in capitals, made unique
    $internal = ($nick.ToUpper() -replace '[^A-Z0-9]', '_').Trim('_')
    if ($internal -eq "") { $internal = "MERC_$id" }
    if ($internalNames.ContainsKey($internal)) {
        $unique = "${internal}_$id"
        Note "$id '$nick': the internal name $internal is taken, using $unique"
        $internal = $unique
    }
    $internalNames[$internal] = $id
    $idToName[$id] = $internal

    if ((Int $p.ubFaceIndex $id) -ne $id) { Note "$id '$nick': face index $(Int $p.ubFaceIndex) differs from the ID, the game uses face $id" }
    if ($nick.Length -gt 12) { Note "$id '$nick': the nickname is longer than 12 characters" }

    $e = [ordered]@{}
    $e["profileID"] = $id
    $e["internalName"] = $internal
    $e["type"] = "AIM"
    $e["sex"] = $(if ((Int $p.bSex) -eq 1) { "F" } else { "M" })
    $bt = Int $p.ubBodyType
    if ($bt -lt $bodyTypes.Count) { $e["bodyType"] = $bodyTypes[$bt] } else { $e["bodyType"] = "REGMALE"; Note "$id '$nick': body type $bt unknown, REGMALE used" }
    $sub = Int $p.uiBodyTypeSubFlags
    if ($sub -band 1) { $e["bodyTypeSubstitution"] = "SUB_ANIM_BIGGUYSHOOT2" }
    elseif ($sub -band 2) { $e["bodyTypeSubstitution"] = "SUB_ANIM_BIGGUYTHREATENSTANCE" }
    if (($sub -band 3) -eq 3) { Note "$id '$nick': both body type substitutions are set, only BIGGUYSHOOT2 is kept" }

    $face = [ordered]@{}
    $face["eyesXY"] = @((Int $p.usEyesX), (Int $p.usEyesY))
    $face["mouthXY"] = @((Int $p.usMouthX), (Int $p.usMouthY))
    if ((Int $p.uiBlinkFrequency 3000) -ne 3000) { $face["blinkFrequency"] = Int $p.uiBlinkFrequency }
    if ((Int $p.uiExpressionFrequency 2000) -ne 2000) { $face["expressionFrequency"] = Int $p.uiExpressionFrequency }
    $e["face"] = $face
    $e["skinColor"] = Text $p.SKIN
    $e["hairColor"] = Text $p.HAIR
    $e["vestColor"] = Text $p.VEST
    $e["pantsColor"] = Text $p.PANTS

    $stats = [ordered]@{}
    if ((Int $p.fRegresses) -eq 1) { $stats["evolution"] = "REVERSED" }
    $stats["agility"] = Int $p.bAgility
    $stats["dexterity"] = Int $p.bDexterity
    $stats["experience"] = Int $p.bExpLevel
    $stats["explosive"] = Int $p.bExplosive
    $stats["health"] = Int $p.bLifeMax
    $stats["leadership"] = Int $p.bLeadership
    $stats["marksmanship"] = Int $p.bMarksmanship
    $stats["mechanical"] = Int $p.bMechanical
    $stats["medical"] = Int $p.bMedical
    $stats["sleepiness"] = Int $p.ubNeedForSleep
    $stats["strength"] = Int $p.bStrength
    $stats["wisdom"] = Int $p.bWisdom
    $e["stats"] = $stats

    $dis = Int $p.bDisability
    if ($dis -ge 1 -and $dis -lt $personality.Count) { $e["personalityTrait"] = $personality[$dis] }
    elseif ($dis -ge $personality.Count) { Note "$id '$nick': disability $dis has no counterpart, skipped" }

    foreach ($pair in @(@("bOldSkillTrait", "skillTrait"), @("bOldSkillTrait2", "skillTrait2"))) {
        $s = Int $p.($pair[0])
        if ($s -ge 1 -and $s -lt $skills.Count) { $e[$pair[1]] = $skills[$s] }
        elseif ($s -ge $skills.Count) { Note "$id '$nick': $($pair[0]) $s has no counterpart, skipped" }
    }

    $att = Int $p.bAttitude
    if ($att -ge 1 -and $att -lt $attitudes.Count) { $e["attitude"] = $attitudes[$att] }
    $sx = Int $p.bSexist
    if ($sx -ge 1 -and $sx -lt $sexism.Count) { $e["sexismMode"] = $sexism[$sx] }
    if ((Int $p.fGoodGuy) -eq 1) { $e["isGoodGuy"] = $true }

    $e["toleranceForPlayersReputation"] = Int $p.bReputationTolerance
    $e["toleranceForPlayersDeathRate"] = Int $p.bDeathRate
    $contract = [ordered]@{}
    $contract["dailySalary"] = Int $p.sSalary
    $contract["weeklySalary"] = Int $p.uiWeeklySalary
    $contract["biWeeklySalary"] = Int $p.uiBiWeeklySalary
    $contract["isMedicalDepositRequired"] = ((Int $p.bMedicalDeposit) -ne 0)
    $e["contract"] = $contract

    $dialogue = @()
    foreach ($a in @(@("FRIENDLY", "usApproachFactorFriendly"), @("DIRECT", "usApproachFactorDirect"),
                     @("THREATEN", "usApproachFactorThreaten"), @("RECRUIT", "usApproachFactorRecruit"))) {
        $dialogue += [ordered]@{ approach = $a[0]; effectiveness = (Int $p.($a[1])) }
    }
    $e["dialogue"] = $dialogue

    $newInfo.Add((ConvertTo-Json2 $e 1).TrimStart())

    $n = [ordered]@{}
    $n["profileID"] = $id
    if ((Text $p.zName) -ne "") { $n["fullName"] = Text $p.zName }
    if ($nick -ne "") { $n["nickname"] = $nick }
    $newNames.Add((ConvertTo-Json2 $n 1).TrimStart())

    $converted += $id
}

# ---- relations: needs the internal names of all converted profiles -------------------------
$newRel = New-Object System.Collections.Generic.List[string]
foreach ($id in $converted) {
    $p = $profiles[$id]
    $nick = Text $p.zNickname
    $targets = [ordered]@{}   # target id -> relation object
    function Get-Target($tid) {
        $k = [string]$tid   # string key: an int would be taken as an index by OrderedDictionary
        if (-not $targets.Contains($k)) { $targets[$k] = [ordered]@{ target = $idToName[$tid]; opinion = 0 } }
        return $targets[$k]
    }
    function Can-Use($tid, $what) {
        if ($tid -eq 255 -or $tid -lt 0) { return $false }
        if ($tid -gt 127) { Note "$id '$nick': $what $tid is above 127 and cannot be stored, skipped"; return $false }
        if (-not $idToName.ContainsKey($tid)) { Note "$id '$nick': $what $tid has no profile, skipped"; return $false }
        return $true
    }
    foreach ($slot in 1, 2) {
        $b = Int $p.("bBuddy$slot") 255
        if (Can-Use $b "friend$slot") { $t = Get-Target $b; $t["friend$slot"] = $true }
        $h = Int $p.("bHated$slot") 255
        if (Can-Use $h "enemy$slot") { $t = Get-Target $h; $t["enemy$slot"] = $true; $t["tolerance"] = Int $p.("bHatedTime$slot") }
    }
    $ltl = Int $p.bLearnToLike 255
    if (Can-Use $ltl "eventual friend") { $t = Get-Target $ltl; $t["eventualFriend"] = $true; $t["resistanceToBefriending"] = Int $p.bLearnToLikeTime }
    $lth = Int $p.bLearnToHate 255
    if (Can-Use $lth "eventual enemy") { $t = Get-Target $lth; $t["eventualEnemy"] = $true; $t["resistanceToMakingEnemy"] = Int $p.bLearnToHateTime }

    if ($targets.Count -gt 0) {
        $r = [ordered]@{ profile = $idToName[$id]; relations = @($targets.Values) }
        $newRel.Add((ConvertTo-Json2 $r 1).TrimStart())
    }
}

# ---- write ------------------------------------------------------------------------------------------
$allInfo = @($keptInfo) + @($newInfo)
Write-ArrayFile $infoPath $info.Header $allInfo

$keptNames = foreach ($o in $names.Objects) {
    $id = [int]([regex]::Match($o, '"profileID":\s*(\d+)').Groups[1].Value)
    if ($id -lt $FirstId -or $id -gt $LastId) { $o }
}
Write-ArrayFile $namesPath $names.Header (@($keptNames) + @($newNames))

$replacedNames = @{}
foreach ($k in $removedNames.Keys) { $replacedNames[$k] = $true }
foreach ($id in $converted) { $replacedNames[$idToName[$id]] = $true }
$keptRel = foreach ($o in $rel.Objects) {
    $name = [regex]::Match($o, '"profile":\s*"([^"]*)"').Groups[1].Value
    if (-not $replacedNames.ContainsKey($name)) { $o }
}
Write-ArrayFile $relPath $rel.Header (@($keptRel) + @($newRel))

Write-Host "Converted $($converted.Count) profiles: $($converted -join ', ')"
foreach ($line in $report) { Write-Host "  note: $line" }

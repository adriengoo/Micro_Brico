[CmdletBinding()]
param(
    [string]$TestRegex = '^CAMP-001\.'
)

$ErrorActionPreference = 'Stop'

function Get-GitValue {
    param([string[]]$Arguments, [string]$Fallback = '(indisponible)')

    try {
        $value = (& git @Arguments 2>$null | Out-String).Trim()
        if ($LASTEXITCODE -eq 0 -and -not [string]::IsNullOrWhiteSpace($value)) {
            return $value
        }
    }
    catch {
        # A test report must still be produced outside a Git checkout.
    }
    return $Fallback
}

function Convert-CaseValue {
    param([string]$Value)

    $value = $Value.Trim()
    if ($value.Length -ge 2 -and (($value.StartsWith('"') -and $value.EndsWith('"')) -or
                                  ($value.StartsWith("'") -and $value.EndsWith("'")))) {
        return $value.Substring(1, $value.Length - 2)
    }
    return $value
}

function Get-CampaignCase {
    param(
        [string]$CasesDirectory,
        [string]$CaseId
    )

    $path = Get-ChildItem -LiteralPath $CasesDirectory -Filter "$CaseId-*.yaml" |
        Select-Object -First 1 -ExpandProperty FullName
    if (-not $path) {
        return [pscustomobject]@{ Id = $CaseId; Title = '(fiche introuvable)'; Steps = @() }
    }

    $title = '(titre non renseigne)'
    $steps = [System.Collections.Generic.List[object]]::new()
    $step = $null
    foreach ($line in [System.IO.File]::ReadAllLines($path, [System.Text.UTF8Encoding]::new($false))) {
        if ($line -match '^title:\s*(.*)$') {
            $title = Convert-CaseValue $Matches[1]
        }
        elseif ($line -match '^\s{2}- number:\s*(\d+)') {
            if ($null -ne $step) { $steps.Add($step) }
            $step = [ordered]@{ Number = [int]$Matches[1]; Purpose = ''; Action = ''; Expected = '' }
        }
        elseif ($null -ne $step -and $line -match '^\s{4}purpose:\s*(.*)$') {
            $step.Purpose = Convert-CaseValue $Matches[1]
        }
        elseif ($null -ne $step -and $line -match '^\s{4}action:\s*(.*)$') {
            $step.Action = Convert-CaseValue $Matches[1]
        }
        elseif ($null -ne $step -and $line -match '^\s{4}expected:\s*(.*)$') {
            $step.Expected = Convert-CaseValue $Matches[1]
        }
    }
    if ($null -ne $step) { $steps.Add($step) }
    return [pscustomobject]@{ Id = $CaseId; Title = $title; Steps = $steps }
}

function Get-CaseIdFromCTestName {
    param([string]$CTestName)

    if ($CTestName -match 'TC-(\d{3})') { return "TC-$($Matches[1])" }
    if ($CTestName -match '\.tc(\d{3})_') { return "TC-$($Matches[1])" }
    return 'TC-INCONNU'
}

function Get-LastReportedStep {
    param([string]$Output)

    $matches = [regex]::Matches($Output, 'CAMP001_STEP=(\d+)')
    if ($matches.Count -eq 0) { return $null }
    return [int]$matches[$matches.Count - 1].Groups[1].Value
}

function Write-JUnitElement {
    param(
        [System.Xml.XmlWriter]$Writer,
        [string]$Name,
        [string]$Text
    )

    $Writer.WriteStartElement($Name)
    $Writer.WriteString($Text)
    $Writer.WriteEndElement()
}

function New-CampaignJUnitReport {
    param(
        [string]$NativeReportPath,
        [string]$ReportPath,
        [string]$CasesDirectory,
        [datetime]$StartedAt,
        [datetime]$FinishedAt,
        [string]$Commit,
        [string]$Tags,
        [string]$Branch,
        [string[]]$DirtyChanges,
        [string]$CTestOutput,
        [int]$CTestExitCode
    )

    $nativeCases = @()
    if (Test-Path -LiteralPath $NativeReportPath) {
        try {
            [xml]$native = [System.IO.File]::ReadAllText($NativeReportPath, [System.Text.UTF8Encoding]::new($false))
            $nativeCases = @($native.testsuite.testcase)
        }
        catch {
            # Keep the runner failure in the report below.
        }
    }
    if ($nativeCases.Count -eq 0) {
        $runnerFailure = $null
        if ($CTestExitCode -ne 0) {
            $runnerFailure = [pscustomobject]@{ message = 'CTest did not produce JUnit output.' }
        }
        $nativeCases = @([pscustomobject]@{
            name = 'CAMP-001.ctest-runner'
            classname = 'CAMP-001'
            time = '0'
            failure = $runnerFailure
            skipped = $null
            'system-out' = ''
        })
    }

    $failed = @($nativeCases | Where-Object { $null -ne $_.failure }).Count
    $skipped = @($nativeCases | Where-Object { $null -ne $_.skipped }).Count
    $errors = if ($CTestExitCode -ne 0 -and $failed -eq 0) { '1' } else { '0' }
    $settings = [System.Xml.XmlWriterSettings]::new()
    $settings.Encoding = [System.Text.UTF8Encoding]::new($false)
    $settings.Indent = $true
    $settings.NewLineChars = "`n"
    $writer = [System.Xml.XmlWriter]::Create($ReportPath, $settings)

    try {
        $writer.WriteStartDocument()
        $writer.WriteStartElement('testsuite')
        $writer.WriteAttributeString('name', 'CAMP-001')
        $writer.WriteAttributeString('tests', [string]$nativeCases.Count)
        $writer.WriteAttributeString('failures', [string]$failed)
        $writer.WriteAttributeString('errors', $errors)
        $writer.WriteAttributeString('skipped', [string]$skipped)
        $writer.WriteAttributeString('timestamp', $StartedAt.ToString('yyyy-MM-ddTHH:mm:ssK'))
        $writer.WriteAttributeString('time', [string][Math]::Round(($FinishedAt - $StartedAt).TotalSeconds, 3))

        $writer.WriteStartElement('properties')
        $properties = [ordered]@{
            'campaign.id' = 'CAMP-001'
            'executed.at.local' = $StartedAt.ToString('yyyy-MM-dd HH:mm:ss K')
            'git.commit' = $Commit
            'git.tags.at.commit' = $Tags
            'git.branch' = $Branch
            'git.working_tree' = if ($DirtyChanges.Count -eq 0) { 'clean' } else { 'dirty' }
            'git.changed_files.count' = [string]$DirtyChanges.Count
            'ctest.exit_code' = [string]$CTestExitCode
        }
        foreach ($property in $properties.GetEnumerator()) {
            $writer.WriteStartElement('property')
            $writer.WriteAttributeString('name', $property.Key)
            $writer.WriteAttributeString('value', [string]$property.Value)
            $writer.WriteEndElement()
        }
        $writer.WriteEndElement()

        foreach ($nativeCase in $nativeCases) {
            $ctestName = [string]$nativeCase.name
            $caseId = Get-CaseIdFromCTestName $ctestName
            $case = Get-CampaignCase -CasesDirectory $CasesDirectory -CaseId $caseId
            $rawOutput = [string]$nativeCase.'system-out'
            $isFailure = $null -ne $nativeCase.failure
            $isSkipped = $null -ne $nativeCase.skipped
            $lastStep = Get-LastReportedStep $rawOutput

            $writer.WriteStartElement('testcase')
            $writer.WriteAttributeString('name', $ctestName)
            $writer.WriteAttributeString('classname', "CAMP-001.$caseId")
            $writer.WriteAttributeString('time', [string]$nativeCase.time)

            if ($isSkipped) {
                $writer.WriteStartElement('skipped')
                $writer.WriteAttributeString('message', 'CTest marked this test as skipped.')
                $writer.WriteEndElement()
            }
            elseif ($isFailure) {
                $message = [string]$nativeCase.failure.message
                if ([string]::IsNullOrWhiteSpace($message)) { $message = 'CTest reported a failure.' }
                $writer.WriteStartElement('failure')
                $writer.WriteAttributeString('message', $message)
                $writer.WriteString($rawOutput)
                $writer.WriteEndElement()
            }

            $stepLines = [System.Collections.Generic.List[string]]::new()
            $stepLines.Add("Fiche $($case.Id) - $($case.Title)")
            if ($isSkipped) {
                $stepLines.Add('VERDICT: SKIPPED - les etapes ne sont pas executees.')
            }
            elseif (-not $isFailure) {
                $stepLines.Add('VERDICT: PASS - toutes les etapes ci-dessous sont validees.')
            }
            else {
                $stepLines.Add('VERDICT: FAIL - voir l''etape marquee en echec ou non determinee.')
            }
            foreach ($step in $case.Steps) {
                $stepStatus = if ($isSkipped) { 'SKIPPED' } elseif (-not $isFailure) { 'OK' } elseif ($null -eq $lastStep) { 'NOT_RUN_OR_UNDETERMINED' } elseif ($step.Number -lt $lastStep) { 'OK' } elseif ($step.Number -eq $lastStep) { 'FAIL' } else { 'NOT_RUN' }
                $stepLines.Add("[$stepStatus] Etape $($step.Number) - $($step.Purpose)")
                $stepLines.Add("  Action : $($step.Action)")
                $stepLines.Add("  Attendu : $($step.Expected)")
            }
            if ($isFailure -and $null -eq $lastStep) {
                $stepLines.Add('ECHEC A LOCALISER : le pilote n''a pas emis de marqueur d''etape avant l''echec ; consulter la sortie CTest ci-dessous.')
            }
            if (-not [string]::IsNullOrWhiteSpace($rawOutput)) {
                $stepLines.Add('Sortie Qt/CTest :')
                $stepLines.Add($rawOutput.Trim())
            }
            Write-JUnitElement -Writer $writer -Name 'system-out' -Text ($stepLines -join "`n")
            $writer.WriteEndElement()
        }

        $suiteOutput = [System.Collections.Generic.List[string]]::new()
        $suiteOutput.Add("CAMP-001 executee le $($StartedAt.ToString('yyyy-MM-dd HH:mm:ss K')).")
        $suiteOutput.Add("Git : commit $Commit ; tag(s) : $Tags ; branche : $Branch.")
        if ($DirtyChanges.Count -eq 0) {
            $suiteOutput.Add('Arbre de travail Git : propre.')
        }
        else {
            $suiteOutput.Add("ARBRE DE TRAVAIL GIT MODIFIE ($($DirtyChanges.Count) entree(s)) :")
            $suiteOutput.AddRange($DirtyChanges)
        }
        if ($CTestExitCode -ne 0) {
            $suiteOutput.Add("CTest a termine avec le code $CTestExitCode.")
            $suiteOutput.Add($CTestOutput.Trim())
        }
        Write-JUnitElement -Writer $writer -Name 'system-out' -Text ($suiteOutput -join "`n")
        $writer.WriteEndElement()
        $writer.WriteEndDocument()
    }
    finally {
        $writer.Dispose()
    }
}

function ConvertTo-HtmlText {
    param([object]$Value)

    return [System.Net.WebUtility]::HtmlEncode([string]$Value)
}

function New-CampaignDashboard {
    param(
        [string]$JUnitPath,
        [string]$DashboardPath
    )

    [xml]$report = [System.IO.File]::ReadAllText($JUnitPath, [System.Text.UTF8Encoding]::new($false))
    $suite = $report.testsuite
    $properties = @{}
    foreach ($property in @($suite.properties.property)) {
        $properties[[string]$property.name] = [string]$property.value
    }

    $casesHtml = [System.Text.StringBuilder]::new()
    foreach ($testcase in @($suite.testcase)) {
        $isFailure = $null -ne $testcase.failure
        $isSkipped = $null -ne $testcase.skipped
        $status = if ($isFailure) { 'FAIL' } elseif ($isSkipped) { 'SKIPPED' } else { 'PASS' }
        $statusClass = $status.ToLowerInvariant()
        $caseName = ConvertTo-HtmlText $testcase.name
        $className = ConvertTo-HtmlText $testcase.classname
        $duration = ConvertTo-HtmlText ("{0}s" -f $testcase.time)
        $output = [string]$testcase.'system-out'
        $stepHtml = [System.Text.StringBuilder]::new()
        $lines = @($output -split "`r?`n")

        for ($index = 0; $index -lt $lines.Count; $index++) {
            if ($lines[$index] -notmatch '^\[(OK|FAIL|SKIPPED|NOT_RUN|NOT_RUN_OR_UNDETERMINED)\] Etape (\d+) - (.*)$') {
                continue
            }
            $stepStatus = $Matches[1]
            $stepNumber = $Matches[2]
            $purpose = ConvertTo-HtmlText $Matches[3]
            $action = ''
            $expected = ''
            if ($index + 1 -lt $lines.Count -and $lines[$index + 1] -match '^  Action : (.*)$') {
                $action = ConvertTo-HtmlText $Matches[1]
            }
            if ($index + 2 -lt $lines.Count -and $lines[$index + 2] -match '^  Attendu : (.*)$') {
                $expected = ConvertTo-HtmlText $Matches[1]
            }
            $stepClass = $stepStatus.ToLowerInvariant().Replace('_', '-')
            $stepMarkup = '<li class="step {0}"><span class="step-status {0}">{1}</span><div><strong>Etape {2} - {3}</strong><p><b>Action :</b> {4}</p><p><b>Attendu :</b> {5}</p></div></li>' -f $stepClass, $stepStatus, $stepNumber, $purpose, $action, $expected
            [void]$stepHtml.AppendLine($stepMarkup)
        }
        if ($stepHtml.Length -eq 0) {
            [void]$stepHtml.AppendLine('<li class="step not-run"><span class="step-status">N/A</span><div>Aucune etape de fiche exploitable pour ce controle technique.</div></li>')
        }

        $failureHtml = ''
        if ($isFailure) {
            $failureText = ConvertTo-HtmlText ([string]$testcase.failure.InnerText)
            $failureHtml = '<details class="failure-detail" open><summary>Sortie de l''echec</summary><pre>{0}</pre></details>' -f $failureText
        }
        $openAttribute = if ($isFailure) { ' open' } else { '' }
        $caseMarkup = '<details class="testcase {0}"{1}><summary><span class="status {0}">{2}</span><span class="case-name">{3}</span><span class="case-meta">{4} - {5}</span></summary><ol class="steps">{6}</ol>{7}</details>' -f $statusClass, $openAttribute, $status, $caseName, $className, $duration, $stepHtml.ToString(), $failureHtml
        [void]$casesHtml.AppendLine($caseMarkup)
    }

    $dirty = $properties['git.working_tree'] -eq 'dirty'
    $dirtyClass = if ($dirty) { 'dirty' } else { 'clean' }
    $dirtyLabel = if ($dirty) { 'DEPOT MODIFIE' } else { 'Depot Git propre' }
    $suiteOutput = ConvertTo-HtmlText ([string]$suite.'system-out')
    $timestamp = ConvertTo-HtmlText $suite.timestamp
    $commit = ConvertTo-HtmlText $properties['git.commit']
    $tags = ConvertTo-HtmlText $properties['git.tags.at.commit']
    $branch = ConvertTo-HtmlText $properties['git.branch']
    $tests = ConvertTo-HtmlText $suite.tests
    $failures = ConvertTo-HtmlText $suite.failures
    $skipped = ConvertTo-HtmlText $suite.skipped
    $duration = ConvertTo-HtmlText ("{0}s" -f $suite.time)
    $dirtyHtml = ''
    if ($dirty) {
        $dirtyHtml = '<aside class="warning"><b>Attention :</b> ce rapport a ete produit avec un arbre Git modifie.<pre>{0}</pre></aside>' -f $suiteOutput
    }

    $html = @"
<!doctype html>
<html lang="fr">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>CAMP-001 - Resultats de test</title>
<style>
:root { color-scheme: dark; --bg:#111827; --card:#1f2937; --muted:#9ca3af; --text:#f9fafb; --green:#22c55e; --red:#ef4444; --amber:#f59e0b; --border:#374151; }
* { box-sizing:border-box; } body { margin:0; background:var(--bg); color:var(--text); font:15px/1.45 system-ui,Segoe UI,sans-serif; }
main { max-width:1120px; margin:0 auto; padding:30px 20px 54px; } h1 { margin:0; font-size:30px; } h2 { margin:30px 0 12px; font-size:20px; }
.subtitle,.case-meta { color:var(--muted); } .summary-grid { display:grid; grid-template-columns:repeat(auto-fit,minmax(145px,1fr)); gap:12px; margin:22px 0; }
.metric,.git { background:var(--card); border:1px solid var(--border); border-radius:10px; padding:14px; } .metric strong { display:block; font-size:28px; } .metric span { color:var(--muted); }
.git { display:grid; gap:7px; } .warning { border-left:5px solid var(--amber); background:#3a2b10; padding:14px; border-radius:6px; margin:18px 0; } .clean { color:var(--green); }
details.testcase { background:var(--card); border:1px solid var(--border); border-left:5px solid var(--green); border-radius:8px; margin:9px 0; } details.testcase.fail { border-left-color:var(--red); } details.testcase.skipped { border-left-color:var(--amber); }
summary { cursor:pointer; padding:13px; display:flex; align-items:center; gap:10px; } .status,.step-status { border-radius:999px; font-size:12px; font-weight:800; padding:3px 8px; color:#08111d; background:var(--green); } .status.fail,.step-status.fail { background:var(--red); } .status.skipped,.step-status.skipped,.step-status.not-run,.step-status.not-run-or-undetermined { background:var(--amber); }
.case-name { font-weight:700; flex:1; } .case-meta { font-size:12px; } .steps { list-style:none; padding:0 15px 10px; margin:0; } .step { display:flex; gap:11px; padding:12px 0; border-top:1px solid var(--border); } .step-status { align-self:start; min-width:74px; text-align:center; } .step p { margin:4px 0 0; color:#d1d5db; } pre { white-space:pre-wrap; overflow-wrap:anywhere; background:#0b1220; padding:12px; border-radius:6px; } .failure-detail { margin:0 15px 15px; } footer { color:var(--muted); margin-top:25px; font-size:12px; }
</style>
</head>
<body><main>
<h1>CAMP-001 - Resultats de test</h1>
<p class="subtitle">Execution locale : $timestamp</p>
<section class="summary-grid"><div class="metric"><strong>$tests</strong><span>controles</span></div><div class="metric"><strong>$failures</strong><span>echecs</span></div><div class="metric"><strong>$skipped</strong><span>ignores</span></div><div class="metric"><strong>$duration</strong><span>duree totale</span></div></section>
<section class="git"><div><b>Commit :</b> $commit</div><div><b>Branche :</b> $branch</div><div><b>Tag :</b> $tags</div><div class="$dirtyClass"><b>$dirtyLabel</b></div></section>
$dirtyHtml
<h2>Detail par test et par etape</h2>
$casesHtml
<footer>Rapport genere localement par Run-CAMP-001.ps1 a partir du compte rendu JUnit.</footer>
</main></body></html>
"@

    [System.IO.File]::WriteAllText($DashboardPath, $html, [System.Text.UTF8Encoding]::new($false))
}

$projectRoot = $PSScriptRoot
$buildDirectory = Join-Path $projectRoot 'build\Desktop_Qt_6_11_2_MinGW_64_bit_Debug'
$campaignDirectory = Join-Path $projectRoot 'tests\campaigns\CAMP-001'
$casesDirectory = Join-Path $campaignDirectory 'cases'
$reportsDirectory = Join-Path $campaignDirectory 'reports'
$defaultCmake = 'C:\Qt\Tools\CMake_64\bin\cmake.exe'
$defaultCtest = 'C:\Qt\Tools\CMake_64\bin\ctest.exe'

$cmake = if (Test-Path -LiteralPath $defaultCmake) { $defaultCmake } else { 'cmake' }
$ctest = if (Test-Path -LiteralPath $defaultCtest) { $defaultCtest } else { 'ctest' }

if (-not (Get-Command $cmake -ErrorAction SilentlyContinue)) {
    throw "CMake est introuvable. Installez-le ou adaptez le chemin dans $($MyInvocation.MyCommand.Name)."
}
if (-not (Get-Command $ctest -ErrorAction SilentlyContinue)) {
    throw "CTest est introuvable. Installez-le avec CMake ou adaptez le chemin dans $($MyInvocation.MyCommand.Name)."
}

Set-Location -LiteralPath $projectRoot
$env:MB_TEST_DB_ENABLED = '1'
$env:MB_TEST_DB_DRIVER = 'QMYSQL'
$env:MB_TEST_DB_HOST = 'localhost'
$env:MB_TEST_DB_PORT = '3306'
$env:MB_TEST_DB_NAME = 'db4qtest'
$env:MB_TEST_DB_USER = 'tester'
$env:MB_TEST_DB_PASSWORD = 'tester'
$env:MB_TEST_ADMIN_PASSWORD = 'CAMP001-fixture-admin'
$env:QT_QPA_PLATFORM = 'offscreen'

Write-Host 'Configuration CMake...' -ForegroundColor Cyan
& $cmake --preset qt-6.11.2-mingw-debug

Write-Host 'Compilation des executables de test...' -ForegroundColor Cyan
& $cmake --build $buildDirectory

Write-Host "Execution de CAMP-001 sur $env:MB_TEST_DB_NAME..." -ForegroundColor Cyan
New-Item -ItemType Directory -Force -Path $reportsDirectory | Out-Null
$startedAt = Get-Date
$commit = Get-GitValue -Arguments @('rev-parse', 'HEAD')
$tags = Get-GitValue -Arguments @('tag', '--points-at', 'HEAD') -Fallback '(aucun)'
$branch = Get-GitValue -Arguments @('branch', '--show-current')
$dirtyChanges = @(& git status --porcelain=v1 2>$null | ForEach-Object { $_.ToString() })
if ($LASTEXITCODE -ne 0) { $dirtyChanges = @('Etat Git indisponible.') }
$reportStem = 'CAMP-001_{0}_{1}' -f $startedAt.ToString('yyyyMMdd-HHmmssfff'), $commit.Substring(0, [Math]::Min(7, $commit.Length))
$nativeReport = Join-Path $reportsDirectory "$reportStem.ctest.xml"
$reportPath = Join-Path $reportsDirectory "$reportStem.junit.xml"
$dashboardPath = Join-Path $reportsDirectory "$reportStem.dashboard.html"
$latestDashboardPath = Join-Path $reportsDirectory 'latest.dashboard.html'

$ctestOutput = @(& $ctest --test-dir $buildDirectory --output-on-failure --output-junit $nativeReport -R $TestRegex 2>&1 |
    ForEach-Object { $_.ToString() })
$ctestExitCode = $LASTEXITCODE
$ctestOutput | ForEach-Object { Write-Host $_ }
$finishedAt = Get-Date

$reportParameters = @{
    NativeReportPath = $nativeReport
    ReportPath = $reportPath
    CasesDirectory = $casesDirectory
    StartedAt = $startedAt
    FinishedAt = $finishedAt
    Commit = $commit
    Tags = $tags
    Branch = $branch
    DirtyChanges = $dirtyChanges
    CTestOutput = ($ctestOutput -join "`n")
    CTestExitCode = $ctestExitCode
}
New-CampaignJUnitReport @reportParameters
Remove-Item -LiteralPath $nativeReport -Force -ErrorAction SilentlyContinue
New-CampaignDashboard -JUnitPath $reportPath -DashboardPath $dashboardPath
Copy-Item -LiteralPath $dashboardPath -Destination $latestDashboardPath -Force

Write-Host "Rapport JUnit : $reportPath" -ForegroundColor Cyan
Write-Host "Dashboard HTML : $latestDashboardPath" -ForegroundColor Cyan

if ($ctestExitCode -ne 0) {
    throw "La campagne CAMP-001 a echoue (code de sortie $ctestExitCode). Consultez le rapport JUnit : $reportPath"
}

Write-Host 'CAMP-001 terminee avec succes.' -ForegroundColor Green

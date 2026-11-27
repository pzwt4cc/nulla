param(
    [string]$Version = "0.3.0",
    [string]$BuildDir = "build-win",
    [string]$OutDir = "dist"
)

$ErrorActionPreference = "Stop"

$RootDir = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$BuildPath = if ([System.IO.Path]::IsPathRooted($BuildDir)) { $BuildDir } else { Join-Path $RootDir $BuildDir }
$StageDir = Join-Path $BuildPath "stage"
$OutPath = if ([System.IO.Path]::IsPathRooted($OutDir)) { $OutDir } else { Join-Path $RootDir $OutDir }

if (Test-Path $StageDir) {
    Remove-Item -Recurse -Force $StageDir
}
New-Item -ItemType Directory -Force -Path $OutPath | Out-Null

$GeneratorArgs = @()
if (Get-Command ninja -ErrorAction SilentlyContinue) {
    $GeneratorArgs = @("-G", "Ninja")
}

& cmake -S $RootDir -B $BuildPath @GeneratorArgs -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& cmake --build $BuildPath --config Release --parallel
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& cmake --install $BuildPath --config Release --prefix $StageDir
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$ExePath = Join-Path $StageDir "nulla.exe"
if (-not (Test-Path $ExePath)) {
    throw "Expected executable not found at $ExePath"
}

$QmlDir = Join-Path $RootDir "qml"
& windeployqt --release --no-translations --qmldir $QmlDir $ExePath
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

if ($env:VCToolsRedistDir) {
    $CrtDirs = Get-ChildItem -Path (Join-Path $env:VCToolsRedistDir "x64") -Directory -Filter "Microsoft.VC*.CRT" -ErrorAction SilentlyContinue
    foreach ($dir in $CrtDirs) {
        Get-ChildItem -Path $dir.FullName -File -Filter "*.dll" | ForEach-Object {
            Copy-Item -Path $_.FullName -Destination $StageDir -Force
        }
    }
}

$RuntimeWxs = Join-Path $BuildPath "QtRuntime.wxs"
$Lines = [System.Collections.Generic.List[string]]::new()
$Lines.Add('<Wix xmlns="http://wixtoolset.org/schemas/v4/wxs">')
$Lines.Add('  <Fragment>')
$Lines.Add('    <ComponentGroup Id="QtRuntimeComponents" Directory="INSTALLFOLDER">')

Get-ChildItem -Path $StageDir -Recurse -File | Sort-Object FullName | ForEach-Object {
    $RelPath = $_.FullName.Substring($StageDir.Length).TrimStart('\', '/')
    if ($RelPath -ieq "nulla.exe") {
        return
    }
    $RelDir = Split-Path -Path $RelPath -Parent
    $EscapedFull = [System.Security.SecurityElement]::Escape($_.FullName)
    if ([string]::IsNullOrEmpty($RelDir)) {
        $Lines.Add('      <Component Guid="*">')
    } else {
        $EscapedDir = [System.Security.SecurityElement]::Escape($RelDir)
        $Lines.Add("      <Component Subdirectory=`"$EscapedDir`" Guid=`"*`">")
    }
    $Lines.Add("        <File Source=`"$EscapedFull`" KeyPath=`"yes`" />")
    $Lines.Add('      </Component>')
}

$Lines.Add('    </ComponentGroup>')
$Lines.Add('  </Fragment>')
$Lines.Add('</Wix>')
[System.IO.File]::WriteAllLines($RuntimeWxs, $Lines, [System.Text.UTF8Encoding]::new($false))

$WxsMain = Join-Path $PSScriptRoot "Nulla.wxs"
$IconPath = Join-Path $RootDir "assets\nulla.ico"
$LicenseRtf = Join-Path $PSScriptRoot "License.rtf"
$MsiOut = Join-Path $OutPath "Nulla-$Version-win64.msi"

& wix build $WxsMain $RuntimeWxs `
    -arch x64 `
    -ext WixToolset.UI.wixext `
    -d "Version=$Version" `
    -d "StageDir=$StageDir" `
    -d "IconPath=$IconPath" `
    -d "LicenseRtf=$LicenseRtf" `
    -o $MsiOut
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "Created MSI: $MsiOut"

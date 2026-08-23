[CmdletBinding()]
param(
    [switch]$SkipBuild
)

$ErrorActionPreference = 'Stop'

$RepositoryRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$BuildDirectory = Join-Path $RepositoryRoot 'x64\Release'
$DistributionRoot = Join-Path $RepositoryRoot 'Dist'
$PackageDirectory = Join-Path $DistributionRoot 'TinyEngine_Demo'

function Find-MSBuild
{
    $Command = Get-Command 'MSBuild.exe' -ErrorAction SilentlyContinue
    if ($Command)
    {
        return $Command.Source
    }

    $CommonPath = 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe'
    if (Test-Path -LiteralPath $CommonPath)
    {
        return $CommonPath
    }

    throw 'MSBuild.exe를 찾을 수 없습니다. Visual Studio Developer PowerShell에서 다시 실행해 주세요.'
}

if (-not $SkipBuild)
{
    $MSBuildPath = Find-MSBuild
    & $MSBuildPath (Join-Path $RepositoryRoot 'TinyEngine.sln') /t:Build /p:Configuration=Release /p:Platform=x64 /m

    if ($LASTEXITCODE -ne 0)
    {
        throw "Release 빌드에 실패했습니다. Exit code: $LASTEXITCODE"
    }
}

$RuntimeFiles = @(
    (Join-Path $BuildDirectory 'TinyEngine.exe'),
    (Join-Path $BuildDirectory 'SDL3.dll')
)

$AssetFiles = @(
    'magicat.png',
    'Maps\dungeon_001.json',
    'Maps\Dungeon_Tileset.png'
)

$ShaderFiles = @(
    'sprite.hlsl',
    'rc_gather.hlsl',
    'rc_merge.hlsl',
    'rc_resolve.hlsl',
    'composite_scene.hlsl'
)

$RequiredFiles = @($RuntimeFiles)
$RequiredFiles += $AssetFiles | ForEach-Object { Join-Path (Join-Path $RepositoryRoot 'Assets') $_ }
$RequiredFiles += $ShaderFiles | ForEach-Object { Join-Path (Join-Path $RepositoryRoot 'TinyEngine\Shaders') $_ }
$RequiredFiles += Join-Path $RepositoryRoot 'Packaging\TinyEngine_Demo_README.txt'

foreach ($RequiredFile in $RequiredFiles)
{
    if (-not (Test-Path -LiteralPath $RequiredFile -PathType Leaf))
    {
        throw "필수 배포 파일이 없습니다: $RequiredFile"
    }
}

$ResolvedPackageDirectory = [System.IO.Path]::GetFullPath($PackageDirectory)
$ExpectedPackagePrefix = [System.IO.Path]::GetFullPath($DistributionRoot) + [System.IO.Path]::DirectorySeparatorChar

if (-not $ResolvedPackageDirectory.StartsWith($ExpectedPackagePrefix, [System.StringComparison]::OrdinalIgnoreCase))
{
    throw "안전하지 않은 배포 경로입니다: $ResolvedPackageDirectory"
}

if (Test-Path -LiteralPath $ResolvedPackageDirectory)
{
    Remove-Item -LiteralPath $ResolvedPackageDirectory -Recurse -Force
}

$AssetDestination = Join-Path $ResolvedPackageDirectory 'Assets'
$MapDestination = Join-Path $AssetDestination 'Maps'
$ShaderDestination = Join-Path $ResolvedPackageDirectory 'Shaders'

New-Item -ItemType Directory -Path $MapDestination -Force | Out-Null
New-Item -ItemType Directory -Path $ShaderDestination -Force | Out-Null

Copy-Item -LiteralPath $RuntimeFiles -Destination $ResolvedPackageDirectory
Copy-Item -LiteralPath (Join-Path $RepositoryRoot 'Assets\magicat.png') -Destination $AssetDestination
Copy-Item -LiteralPath (Join-Path $RepositoryRoot 'Assets\Maps\dungeon_001.json') -Destination $MapDestination
Copy-Item -LiteralPath (Join-Path $RepositoryRoot 'Assets\Maps\Dungeon_Tileset.png') -Destination $MapDestination

foreach ($ShaderFile in $ShaderFiles)
{
    Copy-Item -LiteralPath (Join-Path $RepositoryRoot "TinyEngine\Shaders\$ShaderFile") -Destination $ShaderDestination
}

Copy-Item -LiteralPath (Join-Path $RepositoryRoot 'Packaging\TinyEngine_Demo_README.txt') -Destination (Join-Path $ResolvedPackageDirectory 'README.txt')

Write-Host "TinyEngine 배포 패키지를 생성했습니다: $ResolvedPackageDirectory"

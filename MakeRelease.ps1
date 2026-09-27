# Assembles an AutoHotkey_H v1 release folder in the layout of HotKeyIt's ahkdll-v1-release, from the binaries in bin\.
# Build all 24 release configurations first (Release, ReleaseDll, ReleaseDllMini, Self-contained; each also as MT_ and
# (mbcs), for Win32; the Unicode ones also for x64). Then run:
#   powershell -File ahkdll-updated\MakeRelease.ps1 [-Out <folder>] [-Template <folder>]
# The folders and files that are not built here (lib\, Compiler\ scripts, Compiler\v2\, 7-zip.chm, LICENSE) are copied
# from -Template (default: ..\ahkdll-v1-release-master).
param(
	[string]$Out = (Join-Path (Split-Path $PSScriptRoot) 'ahkdll-v1-release-updated'),
	[string]$Template = (Join-Path (Split-Path $PSScriptRoot) 'ahkdll-v1-release-master')
)
$ErrorActionPreference = 'Stop'
$repo = $PSScriptRoot
$docs = Join-Path (Split-Path $repo) 'AutoHotkey_H-Docs-full'
$redist = 'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Redist\MSVC\14.29.30133'
$platforms = 'Win32a', 'Win32a_MT', 'Win32w', 'Win32w_MT', 'x64w', 'x64w_MT'
$packer = Join-Path $repo 'bin\x64w\AutoHotkey.exe'

foreach ($p in $platforms) {
	foreach ($f in 'AutoHotkey.exe', 'AutoHotkeyDLL.dll', 'AutoHotkeyMini.dll', 'AutoHotkeySC.bin') {
		if (-not (Test-Path "$repo\bin\$p\$f")) { throw "missing bin\$p\$f; build all release configurations first" }
	}
}
if (Test-Path $Out) { Remove-Item $Out -Recurse -Force }
New-Item -ItemType Directory $Out | Out-Null

# Files that are not built here.
Copy-Item "$Template\lib" "$Out\lib" -Recurse
Copy-Item "$Template\Compiler" "$Out\Compiler" -Recurse
Copy-Item "$Template\7-zip.chm", "$Template\LICENSE" $Out
# The merged documentation contains both the AutoHotkey v1.1 and the AutoHotkey_H pages, so it also replaces AutoHotkey.chm.
Copy-Item "$docs\AutoHotkey_H.chm" "$Out\AutoHotkey_H.chm"

foreach ($p in $platforms) {
	$src = "$repo\bin\$p"; $dst = "$Out\$p"
	New-Item -ItemType Directory $dst | Out-Null
	Copy-Item "$src\AutoHotkey.exe", "$src\AutoHotkeyMini.dll", "$src\AutoHotkeySC.bin" $dst
	Copy-Item "$src\AutoHotkeyDLL.dll" "$dst\AutoHotkey.dll"
	$bits = if ($p -like 'x64*') { '64' } else { '32' }
	$mt = if ($p -like '*_MT') { '_MT' } else { '' }
	$sevenzip = "$repo\source\resources\7-zip$bits$mt.dll"
	# AutoHotkey.exe gets the dlls, 7-zip and the resource library; AutoHotkeySC.bin gets the dlls and 7-zip.
	foreach ($t in @(@("$dst\AutoHotkey.exe", "$repo\source\resources\reslib"), @("$dst\AutoHotkeySC.bin", ''))) {
		$argLine = "`"$repo\ReleasePack.ahk`" `"$($t[0])`" `"$dst\AutoHotkey.dll`" `"$dst\AutoHotkeyMini.dll`" `"$sevenzip`" `"$($t[1])`""
		$proc = Start-Process -FilePath $packer -ArgumentList $argLine -Wait -PassThru -NoNewWindow
		if ($proc.ExitCode -ne 0) { throw "packing $($t[0]) failed ($($proc.ExitCode))" }
	}
	if (-not $mt) {
		$arch = if ($bits -eq '64') { 'x64' } else { 'x86' }
		Copy-Item "$redist\$arch\Microsoft.VC142.CRT\vcruntime140.dll" $dst
		if ($bits -eq '64') { Copy-Item "$redist\$arch\Microsoft.VC142.CRT\vcruntime140_1.dll" $dst }
	}
}

# Compiler\Ahk2Exe.exe is AutoHotkey.exe renamed, so it runs Ahk2Exe.ahk. AutoHotkeyU/A.exe run the compiler scripts.
Copy-Item "$Out\Win32w_MT\AutoHotkey.exe" "$Out\Compiler\Ahk2Exe.exe"
Copy-Item "$Out\Win32w_MT\AutoHotkey.exe" "$Out\Compiler\AutoHotkeyU.exe"
Copy-Item "$Out\Win32a_MT\AutoHotkey.exe" "$Out\Compiler\AutoHotkeyA.exe"

$version = (Get-Item "$Out\x64w\AutoHotkey.exe").VersionInfo.ProductVersion
@"
ahkdll-v1-release
=================

ahkdll v1 release, AutoHotkey_H $version (AutoHotkey v1.1.37.02 merged into AutoHotkey_H 1.1.33.10).

Changes since 1.1.33.10-H005: see AutoHotkey_H.chm, "AutoHotkey_H Changes".
Folders ending in _MT are linked with the static CRT; the others need the Visual C++ 2015-2022 runtime
(vcruntime140.dll, and vcruntime140_1.dll for x64w, are included).
"@ | Set-Content "$Out\README.md" -Encoding ascii

# HASH: same format as the original release.
$files = @('7-zip.chm', 'AutoHotkey_H.chm')
foreach ($p in $platforms) { foreach ($f in 'AutoHotkey.dll', 'AutoHotkey.exe', 'AutoHotkeyMini.dll', 'AutoHotkeySC.bin') { $files += "$p\$f" } }
$lines = foreach ($alg in @(@('MD5', 'MD5'), @('SHA', 'SHA1'), @('SHA256', 'SHA256'), @('SHA512', 'SHA512'))) {
	"$($alg[0]):"
	foreach ($f in $files) { "  ${f}: $((Get-FileHash "$Out\$f" -Algorithm $alg[1]).Hash)" }
}
$lines | Set-Content "$Out\HASH" -Encoding ascii
"Release assembled in $Out ($version)"

# Smoke test of a release folder made by MakeRelease.ps1. Each platform folder is copied to %TEMP%\ahkrel-<platform>.
#   powershell -File ahkdll-updated\test\release.ps1 [-Release <folder>]
param([string]$Release = (Join-Path (Split-Path (Split-Path $PSScriptRoot)) 'ahkdll-v1-release-updated'))
$ErrorActionPreference = 'Stop'
$results = New-Object System.Collections.Generic.List[string]
function Run([string]$exe, [string]$argLine, [string]$dir, [int]$timeoutMs = 30000) {
	$psi = New-Object Diagnostics.ProcessStartInfo $exe, $argLine
	$psi.UseShellExecute = $false; $psi.RedirectStandardOutput = $true; $psi.RedirectStandardError = $true
	$psi.WorkingDirectory = $dir
	$p = [Diagnostics.Process]::Start($psi)
	$so = $p.StandardOutput.ReadToEndAsync(); $se = $p.StandardError.ReadToEndAsync()
	if (-not $p.WaitForExit($timeoutMs)) { $p.Kill(); return [pscustomobject]@{ Code = 'TIMEOUT'; Out = ''; Err = '' } }
	[pscustomobject]@{ Code = $p.ExitCode; Out = $so.Result; Err = $se.Result }
}
function Check([string]$name, [bool]$ok, [string]$detail = '') {
	$results.Add(("{0} {1}{2}" -f ($(if ($ok) { 'PASS' } else { 'FAIL' })), $name, $(if ($ok -or !$detail) { '' } else { " -> $detail" })))
}

# Runs in the packed AutoHotkey.exe (reslib, AutoHotkey.dll, AutoHotkeyMini.dll and 7-zip from its resources),
# and compiled into an exe with AutoHotkeySC.bin (dlls and 7-zip from the compiled exe's resources).
$smoke = @'
#NoEnv
#NoTrayIcon
r := A_AhkVersion "|" (A_IsUnicode ? "U" : "A") A_PtrSize * 8 "|" A_IsCompiled
r .= "|" MAKELONG(1, 2) "|" HIWORD(0x20001)
t := AhkThread("#Persistent`nx := 21`nDbl(v) {`nreturn v * 2`n}")
m := AhkMini("#Persistent`nmini := A_IsMini")
Loop 200 {
	if t.ahkReady() && m.ahkReady()
		break
	Sleep 25
}
r .= "|" t.ahkFunction("Dbl", t.ahkgetvar("x")) "|" m.ahkgetvar("mini")
t.ahkTerminate(1000), m.ahkTerminate(1000)
shared := CriticalObject({count: 0})
t2 := AhkThread("obj := CriticalObject(" (&shared) ")`nLoop 100`n obj.count++")
Loop 200 {
	if (shared.count = 100)
		break
	Sleep 25
}
r .= "|" shared.count
sz := SevenZip()
arc := A_ScriptDir "\sz_t.7z"
FileDelete, %arc%
FileDelete, %A_ScriptDir%\sz_in.txt
FileAppend, hello-7zip, %A_ScriptDir%\sz_in.txt
FileRemoveDir, %A_ScriptDir%\sz_out, 1
sz.Add(arc, A_ScriptDir "\sz_in.txt", "-hide")
sz.Extract(arc, A_ScriptDir "\sz_out", "", "-y -hide")
FileRead, s, %A_ScriptDir%\sz_out\sz_in.txt
r .= "|" sz.GetFileCount(arc) "|" s
FileAppend, %r%, *
ExitApp
'@

foreach ($p in 'Win32a', 'Win32a_MT', 'Win32w', 'Win32w_MT', 'x64w', 'x64w_MT') {
	$work = Join-Path $env:TEMP "ahkrel-$p"
	if (Test-Path $work) { Remove-Item $work -Recurse -Force }
	Copy-Item "$Release\$p" $work -Recurse
	[IO.File]::WriteAllText("$work\smoke.ahk", $smoke, (New-Object Text.UTF8Encoding $true))
	$enc = if ($p -like 'Win32a*') { 'A32' } elseif ($p -like 'x64*') { 'U64' } else { 'U32' }
	$want = "1.1.37.02|$enc|{0}|131073|2|42|1|100|1|hello-7zip"
	$r = Run "$work\AutoHotkey.exe" '/ErrorStdOut smoke.ahk' $work
	Check "$p AutoHotkey.exe" ($r.Out -eq ($want -f '')) "code=$($r.Code) out=[$($r.Out)] err=[$($r.Err)]"
	# Compile with the release's Ahk2Exe.ahk and this platform's AutoHotkeySC.bin. (Ahk2Exe.exe runs Ahk2Exe.ahk only when
	# started without parameters, because AutoHotkey.exe takes the first parameter as the script file.)
	$r = Run "$Release\Compiler\AutoHotkeyU.exe" "/ErrorStdOut `"$Release\Compiler\Ahk2Exe.ahk`" /in `"$work\smoke.ahk`" /out `"$work\smoke.exe`" /bin `"$work\AutoHotkeySC.bin`"" $work 60000
	Check "$p Ahk2Exe" ((Test-Path "$work\smoke.exe") -and $r.Code -eq 0) "code=$($r.Code) out=[$($r.Out)] err=[$($r.Err)]"
	if (Test-Path "$work\smoke.exe") {
		$r = Run "$work\smoke.exe" '' $work
		Check "$p compiled exe" ($r.Out -eq ($want -f 1)) "code=$($r.Code) out=[$($r.Out)] err=[$($r.Err)]"
	}
}
$results
$f = ($results | Where-Object { $_ -like 'FAIL*' }).Count
"`n==== release: $($results.Count - $f) passed, $f failed ===="

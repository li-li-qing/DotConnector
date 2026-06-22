param(
    [switch]$AllowStale
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$releaseDir = Join-Path $root 'Win32\Release'
$distDir = Join-Path $root 'dist'
$workDir = Join-Path $distDir 'installer_work'
$payloadDir = Join-Path $workDir 'payload'
$setupExe = Join-Path $distDir 'DotConnectorSetup.exe'
$runtimeZip = Join-Path $workDir 'DotConnector_runtime.zip'
$installerSource = Join-Path $workDir 'DotConnectorSetup.cs'

$exePath = Join-Path $releaseDir 'DotConnector.exe'
$dmPath = Join-Path $releaseDir 'dm.dll'
$assetsPath = Join-Path $releaseDir 'assets'

foreach ($required in @($exePath, $dmPath, $assetsPath)) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Missing runtime file: $required. Build Release|Win32 in Visual Studio first."
    }
}

if (-not $AllowStale) {
    $sourceExtensions = @('.cpp', '.h', '.rc', '.vcxproj', '.filters', '.sln')
    $sourceFiles = Get-ChildItem -LiteralPath $root -File -Recurse |
        Where-Object {
            $sourceExtensions -contains $_.Extension -and
            $_.FullName -notlike "*\Win32\*" -and
            $_.FullName -notlike "*\dist\*" -and
            $_.FullName -notlike "*\scripts\*"
        }
    $newestSource = ($sourceFiles | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1)
    $exe = Get-Item -LiteralPath $exePath
    if ($newestSource -and $exe.LastWriteTimeUtc -lt $newestSource.LastWriteTimeUtc) {
        throw "Release exe is older than source file '$($newestSource.FullName)'. Build Release|Win32 first, or rerun with -AllowStale if you intentionally want the existing exe."
    }
}

if (Test-Path -LiteralPath $workDir) {
    Remove-Item -LiteralPath $workDir -Recurse -Force
}
New-Item -ItemType Directory -Path $payloadDir | Out-Null
New-Item -ItemType Directory -Path $distDir -Force | Out-Null

Copy-Item -LiteralPath $exePath -Destination (Join-Path $payloadDir 'DotConnector.exe') -Force
Copy-Item -LiteralPath $dmPath -Destination (Join-Path $payloadDir 'dm.dll') -Force
Copy-Item -LiteralPath $assetsPath -Destination (Join-Path $payloadDir 'assets') -Recurse -Force

if (Test-Path -LiteralPath $runtimeZip) {
    Remove-Item -LiteralPath $runtimeZip -Force
}
Compress-Archive -Path (Join-Path $payloadDir '*') -DestinationPath $runtimeZip -CompressionLevel Optimal
if (-not (Test-Path -LiteralPath $runtimeZip)) {
    throw "Failed to create runtime payload zip: $runtimeZip"
}

$installerCode = @'
using System;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Reflection;
using System.Windows.Forms;

internal static class DotConnectorSetup
{
    [STAThread]
    private static int Main()
    {
        Application.EnableVisualStyles();
        Application.SetCompatibleTextRenderingDefault(false);

        try
        {
            DialogResult choice = MessageBox.Show(
                "Install DotConnector for the current Windows user and create Desktop/Start Menu shortcuts.",
                "DotConnector Setup",
                MessageBoxButtons.OKCancel,
                MessageBoxIcon.Information);
            if (choice != DialogResult.OK)
            {
                return 1;
            }

            string installRoot = Path.Combine(
                Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
                "Programs",
                "DotConnector");
            string exePath = Path.Combine(installRoot, "DotConnector.exe");

            if (Directory.Exists(installRoot))
            {
                Directory.Delete(installRoot, true);
            }
            Directory.CreateDirectory(installRoot);

            string tempZip = Path.Combine(Path.GetTempPath(), "DotConnector_runtime_" + Guid.NewGuid().ToString("N") + ".zip");
            using (Stream input = Assembly.GetExecutingAssembly().GetManifestResourceStream("DotConnectorRuntime.zip"))
            {
                if (input == null)
                {
                    throw new InvalidOperationException("The installer payload is missing.");
                }
                using (FileStream output = File.Create(tempZip))
                {
                    input.CopyTo(output);
                }
            }

            ZipFile.ExtractToDirectory(tempZip, installRoot);
            File.Delete(tempZip);

            CreateShortcut(Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory), "DotConnector.lnk"), exePath, installRoot);

            string startMenuDir = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.Programs), "DotConnector");
            Directory.CreateDirectory(startMenuDir);
            CreateShortcut(Path.Combine(startMenuDir, "DotConnector.lnk"), exePath, installRoot);

            string uninstallScript = Path.Combine(installRoot, "Uninstall-DotConnector.cmd");
            File.WriteAllText(uninstallScript,
                "@echo off\r\n" +
                "del \"%USERPROFILE%\\Desktop\\DotConnector.lnk\" >nul 2>nul\r\n" +
                "rmdir /S /Q \"%APPDATA%\\Microsoft\\Windows\\Start Menu\\Programs\\DotConnector\" >nul 2>nul\r\n" +
                "cd /d \"%LOCALAPPDATA%\\Programs\"\r\n" +
                "rmdir /S /Q \"DotConnector\" >nul 2>nul\r\n",
                System.Text.Encoding.Default);
            CreateShortcut(Path.Combine(startMenuDir, "Uninstall DotConnector.lnk"), uninstallScript, installRoot);

            MessageBox.Show("DotConnector has been installed and will start now.", "DotConnector Setup", MessageBoxButtons.OK, MessageBoxIcon.Information);
            Process.Start(new ProcessStartInfo { FileName = exePath, WorkingDirectory = installRoot });
            return 0;
        }
        catch (Exception ex)
        {
            MessageBox.Show("Install failed:\r\n" + ex.Message, "DotConnector Setup", MessageBoxButtons.OK, MessageBoxIcon.Error);
            return 2;
        }
    }

    private static void CreateShortcut(string shortcutPath, string targetPath, string workingDirectory)
    {
        Type shellType = Type.GetTypeFromProgID("WScript.Shell");
        object shell = Activator.CreateInstance(shellType);
        object shortcut = shellType.InvokeMember("CreateShortcut", BindingFlags.InvokeMethod, null, shell, new object[] { shortcutPath });
        Type shortcutType = shortcut.GetType();
        shortcutType.InvokeMember("TargetPath", BindingFlags.SetProperty, null, shortcut, new object[] { targetPath });
        shortcutType.InvokeMember("WorkingDirectory", BindingFlags.SetProperty, null, shortcut, new object[] { workingDirectory });
        shortcutType.InvokeMember("IconLocation", BindingFlags.SetProperty, null, shortcut, new object[] { targetPath });
        shortcutType.InvokeMember("Save", BindingFlags.InvokeMethod, null, shortcut, null);
    }
}
'@
Set-Content -LiteralPath $installerSource -Value $installerCode -Encoding UTF8

if (Test-Path -LiteralPath $setupExe) {
    Remove-Item -LiteralPath $setupExe -Force
}

$cscCandidates = @(
    (Join-Path $env:WINDIR 'Microsoft.NET\Framework64\v4.0.30319\csc.exe'),
    (Join-Path $env:WINDIR 'Microsoft.NET\Framework\v4.0.30319\csc.exe')
) | Where-Object { Test-Path -LiteralPath $_ }
if (-not $cscCandidates) {
    throw "Could not find .NET Framework C# compiler."
}

$csc = $cscCandidates[0]
$frameworkDir = Split-Path -Parent $csc
$zipReference = Join-Path $frameworkDir 'System.IO.Compression.FileSystem.dll'
& $csc /nologo /target:winexe /platform:anycpu /out:$setupExe /resource:$runtimeZip,DotConnectorRuntime.zip /reference:System.Windows.Forms.dll /reference:System.dll /reference:$zipReference $installerSource
if ($LASTEXITCODE -ne 0) {
    throw "C# installer build failed with exit code $LASTEXITCODE."
}

Get-Item -LiteralPath $setupExe

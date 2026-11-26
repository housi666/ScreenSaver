; Installer fuer ScreenSaver (Inno Setup 6 oder 7), Aufbau wie RoutedMidi
; Updates: gleiche AppId -> neue Version ersetzt die alte (Installer einfach neu starten, Einstellungen bleiben erhalten).
; AppVersion und AppBuild werden von build.bat automatisch uebergeben.
#define AppName "ScreenSaver"
#define AppTitle "ScreenSaver"
#define ScrName "ScreenSaver.scr"
#ifndef AppVersion
  #define AppVersion "0.0.0"
#endif
#ifndef AppBuild
  #define AppBuild "dev"
#endif

[Setup]
; AppId NIE aendern, sonst erkennt der Installer die alte Version nicht als Update
AppId={{4B75C95B-6373-4259-8571-AE7625878832}
AppName={#AppTitle}
AppVersion={#AppVersion}
AppVerName={#AppTitle} {#AppVersion} (Build {#AppBuild})
VersionInfoVersion={#AppVersion}
AppPublisher=Housi Guntern (Instagram / TikTok @housi_666)
AppContact=Instagram / TikTok @housi_666
AppSupportURL=https://www.instagram.com/housi_666/
AppCopyright=(c) Housi Guntern - Instagram / TikTok @housi_666
VersionInfoCompany=Housi Guntern (@housi_666)
VersionInfoCopyright=(c) Housi Guntern - Instagram / TikTok @housi_666
VersionInfoDescription={#AppTitle} Setup - Housi Guntern (@housi_666)
VersionInfoProductName={#AppTitle}
AppPublisherURL=https://github.com/housi666/{#AppName}
AppUpdatesURL=https://github.com/housi666/{#AppName}/releases
DefaultDirName={autopf}\{#AppName}
UsePreviousAppDir=yes
DisableProgramGroupPage=yes
DisableDirPage=yes
OutputDir=Output
OutputBaseFilename={#AppName}-Setup-v{#AppVersion}-Build{#AppBuild}
UninstallDisplayIcon={sys}\{#ScrName}
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
WizardStyle=modern
Compression=lzma2
SolidCompression=yes

[Languages]
Name: "german"; MessagesFile: "compiler:Languages\German.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "setactive"; Description: "Als Bildschirmschoner von Windows einstellen"; GroupDescription: "Nach der Installation:"

[Files]
; Bildschirmschoner liegen im Windows-Ordner (System32), nur dort listet Windows sie auf
Source: "..\fertig\{#ScrName}"; DestDir: "{sys}"; Flags: ignoreversion

[Registry]
Root: HKCU; Subkey: "Control Panel\Desktop"; ValueType: string; ValueName: "SCRNSAVE.EXE"; ValueData: "{sys}\{#ScrName}"; Tasks: setactive
Root: HKCU; Subkey: "Control Panel\Desktop"; ValueType: string; ValueName: "ScreenSaveActive"; ValueData: "1"; Tasks: setactive

[Run]
Filename: "{sys}\{#ScrName}"; Parameters: "/c"; Description: "Einstellungen jetzt oeffnen"; Flags: postinstall nowait skipifsilent

[InstallDelete]
; alter Name bis v0.3.1
Type: files; Name: "{sys}\BildschirmschonerLED.scr"

[UninstallDelete]
Type: files; Name: "{sys}\{#ScrName}"

; Einstellungen (%APPDATA%\ScreenSaver) bleiben bei Updates und Deinstallation erhalten.

[Code]
// War noch der alte Name (BildschirmschonerLED.scr) als Windows-Bildschirmschoner eingestellt, auf den neuen umstellen
procedure CurStepChanged(CurStep: TSetupStep);
var v: String;
begin
  if CurStep = ssPostInstall then
    if RegQueryStringValue(HKCU, 'Control Panel\Desktop', 'SCRNSAVE.EXE', v) then
      if Pos('BILDSCHIRMSCHONERLED', Uppercase(v)) > 0 then
        RegWriteStringValue(HKCU, 'Control Panel\Desktop', 'SCRNSAVE.EXE', ExpandConstant('{sys}\{#ScrName}'));
end;

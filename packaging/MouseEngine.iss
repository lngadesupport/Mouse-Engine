#define AppName "Mouse Engine"
#define AppVersion "1.0.0-rc.3"
#define AppPublisher "Mouse Engine"
#define AppExeName "MouseEngine.Host.Windows.exe"

[Setup]
AppId={{9F7B0E9A-2C4B-4C68-9A90-8D6C5C3B7C31}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
DefaultDirName={localappdata}\Programs\Mouse Engine
DefaultGroupName=Mouse Engine
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=..\dist
OutputBaseFilename=MouseEngine-1.0.0-rc.3-win64-setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayIcon={app}\{#AppExeName}

[Files]
Source: "..\MouseEngine.Host.Windows.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\Install-MouseEngine.cmd"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\Uninstall-MouseEngine.cmd"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\packaging\RELEASE_MANIFEST.json"; DestDir: "{app}\packaging"; Flags: ignoreversion
Source: "..\packaging\install.ps1"; DestDir: "{app}\packaging"; Flags: ignoreversion
Source: "..\packaging\uninstall.ps1"; DestDir: "{app}\packaging"; Flags: ignoreversion
Source: "..\ui\*"; DestDir: "{app}\ui"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\Mouse Engine"; Filename: "{app}\{#AppExeName}"; WorkingDir: "{app}"
Name: "{autodesktop}\Mouse Engine"; Filename: "{app}\{#AppExeName}"; WorkingDir: "{app}"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional shortcuts:"; Flags: unchecked

[Run]
Filename: "{app}\{#AppExeName}"; Parameters: "--self-test"; WorkingDir: "{app}"; Flags: runhidden waituntilterminated
Filename: "{app}\{#AppExeName}"; WorkingDir: "{app}"; Description: "Launch Mouse Engine"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: filesandordirs; Name: "{app}\packaging"
Type: filesandordirs; Name: "{app}\ui"

; Script Inno Setup pour GitingestUI
; Généré par Antigravity pour une expérience Premium

[Setup]
AppId={{9ED3B20B-306F-4B7C-A58F-3A1B2C4D5E6F}}
AppName=GitingestUI
AppVersion=1.0
AppPublisher=Antigravity
DefaultDirName={pf}\GitingestUI
DefaultGroupName=GitingestUI
AllowNoIcons=yes
; Fix pour l'erreur "PathRedir: Not initialized"
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
; Paramètres de sortie
OutputDir=.
OutputBaseFilename=GitingestUI-Setup
Compression=lzma
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "french"; MessagesFile: "compiler:Languages\French.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "build\Release\GitingestUI.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "build\Release\python_embed\*"; DestDir: "{app}\python_embed"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "imgui.ini"; DestDir: "{app}"; Flags: ignoreversion
; Note: L'icône peut être ajoutée ici si convertie en .ico

[Icons]
Name: "{group}\GitingestUI"; Filename: "{app}\GitingestUI.exe"
Name: "{autodesktop}\GitingestUI"; Filename: "{app}\GitingestUI.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\GitingestUI.exe"; Description: "{cm:LaunchProgram,GitingestUI}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: files; Name: "{app}\config.dat"

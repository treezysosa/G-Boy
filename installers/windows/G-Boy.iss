#ifndef Stage
  #error Stage directory must be supplied
#endif
#ifndef Output
  #error Output directory must be supplied
#endif
[Setup]
AppId={{D05298A2-9C4A-40A8-BDBA-55B6C0C8D06E}
AppName=G Boy
AppVersion=0.6.0
AppVerName=G Boy 0.6.0 Release Candidate 1
AppPublisher=G Boy Audio
AppPublisherURL=https://github.com/treezysosa/G-Boy
DefaultDirName={autopf}\G Boy
DefaultGroupName=G Boy
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=admin
OutputDir={#Output}
OutputBaseFilename=G-Boy-0.6.0-rc.1-Windows-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
LicenseFile={#Stage}\Documentation\LICENSE-BINARY.txt
UninstallDisplayIcon={app}\G Boy.exe
[Files]
Source: "{#Stage}\Standalone\G Boy.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#Stage}\VST3\G Boy.vst3\*"; DestDir: "{commoncf64}\VST3\G Boy.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#Stage}\Factory Content\*"; DestDir: "{commonappdata}\G Boy\Factory Content"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#Stage}\Documentation\*"; DestDir: "{app}\Documentation"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#Stage}\Source.tar.gz"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#Stage}\BUILD-INFO.json"; DestDir: "{app}"; Flags: ignoreversion
[Icons]
Name: "{group}\G Boy"; Filename: "{app}\G Boy.exe"
Name: "{group}\Uninstall G Boy"; Filename: "{uninstallexe}"
[Run]
Filename: "{app}\G Boy.exe"; Description: "Open G Boy"; Flags: nowait postinstall skipifsilent

; Griddy Windows Installer - Inno Setup Script
; Installs VST3 plugin and Standalone application

#define MyAppName "Griddy"
#define MyAppVersion "1.0.12"
#define MyAppPublisher "Generous Corp"
#define MyAppURL "https://github.com/danielraffel/Griddy-MIDI-Effect-Plugin"
#define MyAppExeName "Griddy.exe"

[Setup]
AppId={{8A3F2B1C-4D5E-6F78-9A0B-C1D2E3F4A5B6}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}/issues
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
LicenseFile=..\..\LICENSE
OutputDir=..\..\build\installer
OutputBaseFilename=Griddy_{#MyAppVersion}_Setup
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=admin
UninstallDisplayIcon={app}\{#MyAppExeName}
SetupIconFile=
DisableProgramGroupPage=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Types]
Name: "full"; Description: "Full installation (VST3 + Standalone)"
Name: "vst3only"; Description: "VST3 plugin only"
Name: "standaloneonly"; Description: "Standalone application only"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
Name: "vst3"; Description: "Griddy VST3 Plugin"; Types: full vst3only custom
Name: "standalone"; Description: "Griddy Standalone Application"; Types: full standaloneonly custom

[Files]
; VST3 plugin - install to system VST3 directory
Source: "..\..\build\Griddy_artefacts\Release\VST3\Griddy.vst3\*"; DestDir: "{commoncf}\VST3\Griddy.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs

; Standalone application
Source: "..\..\build\Griddy_artefacts\Release\Standalone\Griddy.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion

; License and readme in app directory
Source: "..\..\LICENSE"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\README.md"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Components: standalone
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Components: standalone; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional shortcuts:"; Components: standalone

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch {#MyAppName}"; Flags: nowait postinstall skipifsilent; Components: standalone

[Messages]
WelcomeLabel2=This will install [name/ver] on your computer.%n%nGriddy is a topographic drum sequencer MIDI effect inspired by Mutable Instruments Grids. It generates evolving drum patterns by interpolating across a 5x5 map of rhythm nodes.%n%nVST3 Plugin: Load in your DAW on one track, route MIDI output to an instrument on another track.%nStandalone App: Run independently with any connected MIDI device.

[Code]
procedure CurPageChanged(CurPageID: Integer);
begin
  if CurPageID = wpFinished then
  begin
    if IsComponentSelected('vst3') then
    begin
      WizardForm.FinishedLabel.Caption :=
        'Griddy has been installed successfully.' + #13#10 + #13#10 +
        'VST3 installed to: ' + ExpandConstant('{commoncf}') + '\VST3\Griddy.vst3' + #13#10 + #13#10 +
        'To use the VST3 in your DAW:' + #13#10 +
        '1. Load Griddy on a MIDI track' + #13#10 +
        '2. Create a second track with a drum instrument' + #13#10 +
        '3. Route the instrument track''s MIDI input from Griddy' + #13#10 +
        '4. Press play and explore the XY pad!';
    end;
  end;
end;

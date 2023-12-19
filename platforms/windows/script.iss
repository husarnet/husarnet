; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

#define MyAppName "Husarnet"
#define HUSARNET_VERSION "2.0.205"
#define MyAppPublisher "Husarnet Sp. z o.o."
#define MyAppURL "https://husarnet.com"
#define MyAppExeName "husarnet.exe"

[Setup]
; NOTE: The value of AppId uniquely identifies this application. Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{843C2149-4374-43E5-8D8E-DFD68C16F5AA}
AppName={#MyAppName}
AppVersion={#HUSARNET_VERSION}
;AppVerName={#MyAppName} {#HUSARNET_VERSION}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DisableDirPage=yes
DefaultDirName={autopf}\{#MyAppName}
DisableProgramGroupPage=yes
; Uncomment the following line to run in non administrative install mode (install for current user only.)
;PrivilegesRequired=lowest
; OutputDir=E:\husarnet\installer
OutputBaseFilename=husarnet-setup
Compression=lzma
SolidCompression=yes
WizardStyle=modern
UsePreviousAppDir=yes
LicenseFile=LICENSE.txt
CloseApplications=force
ChangesEnvironment=yes
ArchitecturesInstallIn64BitMode=x64

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

;[Tasks]
;Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

; credits: https://stackoverflow.com/questions/3304463/how-do-i-modify-the-path-environment-variable-when-running-an-inno-setup-install
[Code]
const EnvironmentKey = 'SYSTEM\CurrentControlSet\Control\Session Manager\Environment';

procedure EnvAddPath(Path: string);
var
    Paths: string;
begin
    { Retrieve current path (use empty string if entry not exists) }
    if not RegQueryStringValue(HKEY_LOCAL_MACHINE, EnvironmentKey, 'Path', Paths)
    then Paths := '';

    { Skip if string already found in path }
    if Pos(';' + Uppercase(Path) + ';', ';' + Uppercase(Paths) + ';') > 0 then exit;

    { App string to the end of the path variable }
    Paths := Paths + ';'+ Path +';'

    { Overwrite (or create if missing) path environment variable }
    if RegWriteStringValue(HKEY_LOCAL_MACHINE, EnvironmentKey, 'Path', Paths)
    then Log(Format('The [%s] added to PATH: [%s]', [Path, Paths]))
    else Log(Format('Error while adding the [%s] to PATH: [%s]', [Path, Paths]));
end;

procedure EnvRemovePath(Path: string);
var
    Paths: string;
    P: Integer;
  begin
    { Skip if registry entry not exists }
    if not RegQueryStringValue(HKEY_LOCAL_MACHINE, EnvironmentKey, 'Path', Paths) then
        exit;

    { Skip if string not found in path }
    P := Pos(';' + Uppercase(Path) + ';', ';' + Uppercase(Paths) + ';');
    if P = 0 then exit;

    { Update path variable }
    Delete(Paths, P - 1, Length(Path) + 1);

    { Overwrite path environment variable }
    if RegWriteStringValue(HKEY_LOCAL_MACHINE, EnvironmentKey, 'Path', Paths)
    then Log(Format('The [%s] removed from PATH: [%s]', [Path, Paths]))
    else Log(Format('Error while removing the [%s] from PATH: [%s]', [Path, Paths]));
end;

[Code]
procedure BeforeReplaceService();
var
  ResultCode: Integer;
begin
  if Exec(ExpandConstant('{app}\bin\nssm.exe'), 'stop husarnet', '', SW_SHOW, ewWaitUntilTerminated, ResultCode) then
  begin
    Log('service stop ok');
  end
  else begin
    Log('service stop not ok');
  end
end;

[Code]
procedure CurStepChanged(CurStep: TSetupStep);
begin
    if CurStep = ssPostInstall
     then EnvAddPath(ExpandConstant('{app}') +'\bin');
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
    if CurUninstallStep = usPostUninstall
    then EnvRemovePath(ExpandConstant('{app}') +'\bin');
end;

[Files]
Source: "husarnet-daemon.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; BeforeInstall: BeforeReplaceService
Source: "husarnet.exe"; DestDir: "{app}\bin"; Flags: ignoreversion;
Source: "addtap.bat"; DestDir: "{app}"; Flags: ignoreversion
Source: "tap-windows.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "nssm.exe"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "installservice.bat"; DestDir: "{app}"; Flags: ignoreversion
Source: "uninstallservice.bat"; DestDir: "{app}"; Flags: ignoreversion

;Source: "*.o"; DestDir: "{app}"; Flags: ignoreversion
;Source: "*.png"; DestDir: "{app}"; Flags: ignoreversion

[Run]
Filename: "{app}\tap-windows.exe"; Parameters: "/S"; StatusMsg: "Installing TAP driver..."

[Run]
Filename: "{app}\installservice.bat"; StatusMsg: "Installing service..."; Flags: runhidden

[UninstallRun]
Filename: "{app}\uninstallservice.bat"; StatusMsg: "Uninstalling service..."; Flags: runhidden

;[Icons]
;Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
;Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon
;Name: "{commonstartup}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Parameters: "--autostart"

;[Run]
;Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

program SDDumpDesktopACL;

{$APPTYPE CONSOLE}

{$R 'SDDumpDesktopACL.res' 'SDDumpDesktopACL.rc'}

uses
  SysUtils,
  SDInfoProcesses in 'SDInfoProcesses.pas',
  SDCommon in 'SDCommon.pas',
  SDInfoSecurity in 'SDInfoSecurity.pas';

begin
  try
    { TODO -oUser -cConsole Main : Insert code here }
    Log('Enumerating sessions');
    EnumerateSessions;
    if SDRetrieveWindowStationsAndDesktops then
    begin
      Log('Retrieved window stations and desktops');
      Log('Dumping window station and desktop ACLs');
      SDDumpWindowStationAndDesktopSecurityInformation;
      Log('Dumping of window station and desktop ACLs completed.');
    end;
  except
    on E: Exception do
      Writeln(E.ClassName, ': ', E.Message);
  end;

end.

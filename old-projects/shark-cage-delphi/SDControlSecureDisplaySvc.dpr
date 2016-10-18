program SDControlSecureDisplaySvc;

uses
  Forms,
  fmControlSecureDisplaySvcUnit in 'fmControlSecureDisplaySvcUnit.pas' {fmControlSecureDisplaySvc},
  SDCommon in 'SDCommon.pas',
  SDProtocol in 'SDProtocol.pas',
  SDSecureDisplaySvcIntf in 'SDSecureDisplaySvcIntf.pas';

//{$R *.res}
{$R 'requireAdministratorManifest.res' 'requireAdministratorManifest.rc'}

begin
  Application.Initialize;
  Application.MainFormOnTaskbar := True;
  Application.CreateForm(TfmControlSecureDisplaySvc, fmControlSecureDisplaySvc);
  Application.Run;
end.

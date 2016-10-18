program WinStaTest;

uses
  Forms,
  fmMainUnit in 'fmMainUnit.pas' {fmMain} ,
  SDCommon in 'SDCommon.pas',
  SDInfoSecurity in 'SDInfoSecurity.pas';

{$R *.res}

begin
  Application.Initialize;
  Application.CreateForm(TfmMain, fmMain);
  Application.Run;

end.

program SDPrepareSwitch;

uses
  Forms,
  Graphics,
  dmScreenshot in 'dmScreenshot.pas' {Screenshot: TDataModule},
  SDCommon in 'SDCommon.pas';

{$R *.res}

var
  BackgroundBitmap: Graphics.TBitmap;

begin
  Application.Initialize;
  Application.MainFormOnTaskbar := True;
  Application.CreateForm(TScreenshot, Screenshot);
  Application.Run;
  BackgroundBitmap := Screenshot.GetDimmed;
  BackgroundBitmap.SaveToFile(SDAppInfoBackgroundBitmapFileName);
end.

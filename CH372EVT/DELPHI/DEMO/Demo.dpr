program Demo;

uses
  Forms,
  Main in 'Main.pas' {FrmMain},
  CH375DLL in 'CH375DLL.pas',
  EnterDate in 'EnterDate.pas' {EnterDlg},
  Unit1 in 'Unit1.pas';

{$R *.res}
begin
  Application.Initialize;
  Application.Title := 'Demo';
  Application.CreateForm(TFrmMain, FrmMain);
  Application.CreateForm(TEnterDlg, EnterDlg);
  Application.Run;
end.

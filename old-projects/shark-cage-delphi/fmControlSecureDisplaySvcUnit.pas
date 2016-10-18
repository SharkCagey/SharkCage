unit fmControlSecureDisplaySvcUnit;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, IdHTTPWebBrokerBridge, InvokeRegistry, Rio, SOAPHTTPClient;

type
  TfmControlSecureDisplaySvc = class(TForm)
    btnQueryServiceStatus: TButton;
    lblServiceStatus: TLabel;
    btnLaunchApplication: TButton;
    cmbApplication: TComboBox;
    btnReturnFromApplication: TButton;
    btnStopService: TButton;
    btnStartservice: TButton;
    ConsumeWebService: THTTPRIO;
    procedure btnQueryServiceStatusClick(Sender: TObject);
    procedure btnStartserviceClick(Sender: TObject);
    procedure btnStopServiceClick(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure btnLaunchApplicationClick(Sender: TObject);
    procedure btnReturnFromApplicationClick(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
  end;

var
  fmControlSecureDisplaySvc: TfmControlSecureDisplaySvc;

implementation

{$R *.dfm}

uses
  WinSvc, SDCommon, SDProtocol, SDSecureDisplaySvcIntf;

type
  _SERVICE_STATUS_PROCESS = record
    dwServiceType: DWORD;
    dwCurrentState: DWORD;
    dwControlsAccepted: DWORD;
    dwWin32ExitCode: DWORD;
    dwServiceSpecificExitCode: DWORD;
    dwCheckPoint: DWORD;
    dwWaitHint: DWORD;
    dwProcessId: DWORD;
    dwServiceFlags: DWORD;
  end;

procedure TfmControlSecureDisplaySvc.btnStartserviceClick(Sender: TObject);
var
  hSCM: SC_HANDLE;
  hService: SC_HANDLE;
  lpServiceArgVectors: PChar;
begin
  hSCM := OpenSCManager(nil, nil, SC_MANAGER_CONNECT);
  if (hSCM > 0) then
  begin
    hService := OpenService(hSCM, PChar(SecureDisplayServiceName), SERVICE_START);
    if (hService > 0) then
    begin
      lpServiceArgVectors := nil;
      if StartService(hService, 0, lpServiceArgVectors) then
      begin
        lblServiceStatus.Caption := 'StartService() succeeded.';
      end
      else
      begin
        lblServiceStatus.Caption := Format('StartService(): %s', [SysErrorMessage(GetLastError)]);
      end;
      CloseServiceHandle(hService)
    end
    else
    begin
      lblServiceStatus.Caption := Format('OpenService(): %s', [SysErrorMessage(GetLastError)]);
    end;
    CloseServiceHandle(hSCM)
  end
  else
  begin
    lblServiceStatus.Caption := Format('OpenSCManager(): %s', [SysErrorMessage(GetLastError)]);
  end;
end;

procedure TfmControlSecureDisplaySvc.btnLaunchApplicationClick(Sender: TObject);
var
  cResponse: Cardinal;
begin
  ConsumeWebService.URL := Format('http://localhost:%d%s', [SDServiceRequestPort, SDServiceRequestURL]);
  try
    cResponse :=
      (ConsumeWebService as ISecureDisplaySvc).launchApplication(cmbApplication.ItemIndex);
    if (cResponse = SDP_SOAP_RESPONSE_LAUNCHED_APP) then
    begin
      lblServiceStatus.Caption := Format('Request sent to launch app %d.', [cmbApplication.ItemIndex]);
    end;
  except
    lblServiceStatus.Caption := Format('Request to launch app %d failed.', [cmbApplication.ItemIndex]);
  end;
end;

procedure TfmControlSecureDisplaySvc.btnQueryServiceStatusClick(Sender: TObject);
var
  hSCM: SC_HANDLE;
  hService: SC_HANDLE;
  SvcStatus: _SERVICE_STATUS_PROCESS;
  cbBufSize: Cardinal;
  pcbBytesNeeded: Cardinal;
begin
  hSCM := OpenSCManager(nil, nil, SC_MANAGER_CONNECT);
  if (hSCM > 0) then
  begin
    hService := OpenService(hSCM, PChar(SecureDisplayServiceName), SERVICE_QUERY_STATUS);
    if (hService > 0) then
    begin
      cbBufSize := SizeOf(SvcStatus);
      pcbBytesNeeded := 0;
      if QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, @SvcStatus, cbBufSize, pcbBytesNeeded) then
      begin
        case SvcStatus.dwCurrentState of
        SERVICE_CONTINUE_PENDING: lblServiceStatus.Caption := 'The service is about to continue.';
        SERVICE_PAUSE_PENDING: lblServiceStatus.Caption := 'The service is pausing.';
        SERVICE_PAUSED: lblServiceStatus.Caption := 'The service is paused.';
        SERVICE_RUNNING: lblServiceStatus.Caption := 'The service is running.';
        SERVICE_START_PENDING: lblServiceStatus.Caption := 'The service is starting.';
        SERVICE_STOP_PENDING: lblServiceStatus.Caption := 'The service is stopping.';
        SERVICE_STOPPED: lblServiceStatus.Caption := 'The service has stopped.';
        else lblServiceStatus.Caption := 'The service is in an unknown state.';
        end;
      end
      else
      begin
        lblServiceStatus.Caption := Format('QueryServiceStatusEx(): %s', [SysErrorMessage(GetLastError)]);
      end;
      CloseServiceHandle(hService);
    end
    else
    begin
      lblServiceStatus.Caption := Format('OpenService(): %s', [SysErrorMessage(GetLastError)]);
    end;
    CloseServiceHandle(hSCM);
  end
  else
  begin
    lblServiceStatus.Caption := Format('OpenSCManager(): %s', [SysErrorMessage(GetLastError)]);
  end;
end;

procedure TfmControlSecureDisplaySvc.btnReturnFromApplicationClick(Sender: TObject);
var
  cResponse: Cardinal;
begin
  ConsumeWebService.URL := Format('http://localhost:%d%s', [SDServiceRequestPort, SDServiceRequestURL]);
  try
    cResponse :=
      (ConsumeWebService as ISecureDisplaySvc).returnFromApplication(cmbApplication.ItemIndex);
    if (cResponse = SDP_SOAP_RESPONSE_RETURNED_FROM_APP) then
    begin
      lblServiceStatus.Caption := Format('Request sent to return from app %d.', [cmbApplication.ItemIndex]);
    end;
  except
    lblServiceStatus.Caption := Format('Request to return from app %d failed.', [cmbApplication.ItemIndex]);
  end;
end;

procedure TfmControlSecureDisplaySvc.btnStopServiceClick(Sender: TObject);
var
  hSCM: SC_HANDLE;
  hService: SC_HANDLE;
  SvcStatus: TServiceStatus;
begin
  hSCM := OpenSCManager(nil, nil, SC_MANAGER_CONNECT);
  if (hSCM > 0) then
  begin
    hService := OpenService(hSCM, PChar(SecureDisplayServiceName), SERVICE_START);
    if (hService > 0) then
    begin
      if ControlService(hService, SERVICE_CONTROL_STOP, SvcStatus) then
      begin
        lblServiceStatus.Caption := 'ControlService() succeeded.';
      end
      else
      begin
        lblServiceStatus.Caption := Format('ControlService(): %s', [SysErrorMessage(GetLastError)]);
      end;
      CloseServiceHandle(hService)
    end
    else
    begin
      lblServiceStatus.Caption := Format('OpenService(): %s', [SysErrorMessage(GetLastError)]);
    end;
    CloseServiceHandle(hSCM)
  end
  else
  begin
    lblServiceStatus.Caption := Format('OpenSCManager(): %s', [SysErrorMessage(GetLastError)]);
  end;
end;

procedure TfmControlSecureDisplaySvc.FormCreate(Sender: TObject);
var
  cApplicationKey: Cardinal;
begin
  lblServiceStatus.Caption := '';

  cmbApplication.Items.Clear;
  cApplicationKey := 0;
  while (SDAppInfoApplicationName(cApplicationKey) <> '') do
  begin
    cmbApplication.Items.Add(SDAppInfoApplicationName(cApplicationKey));
    cApplicationKey := cApplicationKey + 1;
  end;
  if (cmbApplication.Items.Count > 0) then
  begin
    cmbApplication.ItemIndex := 0;
  end;
end;

end.

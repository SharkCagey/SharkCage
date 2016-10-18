unit fmMainUnit;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, ComCtrls, IdTCPConnection, IdTCPClient, IdFTP,
  IdUserAccounts, IdIOHandler, IdIOHandlerSocket, IdSSLOpenSSL,
  IdServerIOHandler, IdFTPServer, IdBaseComponent, IdComponent,
  IdTCPServer, IdMappedPortTCP, IdMappedFTP, IdSSL,
  IdExplicitTLSClientServerBase, IdCustomTCPServer, IdCmdTCPServer,
  IdIOHandlerStack, IdContext, ExtCtrls, JwaWindows;

type
  TfmMain = class(TForm)
    StatusBar1: TStatusBar;
    meMessages: TMemo;
    panelButtons: TPanel;
    meTODO: TMemo;
    btnDLL: TButton;
    btnUploadFileToServer: TButton;
    btnCreateSwitchSwitchClose: TButton;
    btnCreateNewDesktop: TButton;
    btnEnumerateDesktops: TButton;
    btnEnumerateWindowStations: TButton;
    cmbWindowStations: TComboBox;
    cmbDesktops: TComboBox;
    btnEnumerateSessions: TButton;
    btnWaitThenEnumerateThreads: TButton;
    procedure btnEnumerateWindowStationsClick(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure btnEnumerateDesktopsClick(Sender: TObject);
    procedure btnCreateNewDesktopClick(Sender: TObject);
    procedure btnCreateSwitchSwitchCloseClick(Sender: TObject);
    procedure FTPClientAfterClientLogin(Sender: TObject);
    procedure btnDLLClick(Sender: TObject);
    procedure btnEnumerateSessionsClick(Sender: TObject);
    procedure btnWaitThenEnumerateThreadsClick(Sender: TObject);
  private
    { Private declarations }
    FWindowStations: TStringList;
    FDesktops: TStringList;
    FPictureTempFileName: String;
    function LookupAccountBySID(ASID: JwaWindows.PSID): String;
    procedure DisplayAccessMask(const AMask: Cardinal; const AList: TStrings);
    procedure DisplayACL(const AACL: Windows.PACL; const AList: TStrings);
  public
    { Public declarations }
    Procedure ShowDesktopSecurityInformation(Const nWindowStationIndex: Integer;
      Const sDesktopName: String);
    property WindowStations: TStringList read FWindowStations;
    property Desktops: TStringList read FDesktops;
  end;

var
  fmMain: TfmMain;

Function EnumWindowStationsCallBackProc(Const lpszWindowStation: PChar; Const lParam: Integer)
  : Boolean; stdcall;
Function EnumDesktopsCallBackProc(Const lpszDesktop: PChar; Const lParam: Integer)
  : Boolean; stdcall;

implementation

{$R *.dfm}

uses
  JwsclSecureObjects, JwsclSid,
  JwsclACL, SDInfoProcesses, SDCommon, SDInfoSecurity;

const
  ACCESS_ALLOWED_ACE_TYPE = 0;
  ACCESS_DENIED_ACE_TYPE = 1;
  SYSTEM_AUDIT_ACE_TYPE = 2;

type
  TACEHeader = packed record
    ACEFlags: Byte;
    ACESize: Word;
  end;

  TAccessAllowedACE = packed record
    Header: TACEHeader;
    Mask: ACCESS_MASK;
    SIDStart: DWORD;
  end;

  TAccessDeniedACE = packed record
    Header: TACEHeader;
    Mask: ACCESS_MASK;
    SIDStart: DWORD;
  end;

  TSystemAuditACE = packed record
    Header: TACEHeader;
    Mask: ACCESS_MASK;
    SIDStart: DWORD;
  end;

  TACE = packed record
    case ACEType: Byte of
      ACCESS_ALLOWED_ACE_TYPE:
        (AccessAllowedACE: TAccessAllowedACE);
      ACCESS_DENIED_ACE_TYPE:
        (AccessDeniedACE: TAccessDeniedACE);
      SYSTEM_AUDIT_ACE_TYPE:
        (SystemAuditACE: TSystemAuditACE);
  end;

  PACE = ^TACE;

procedure TfmMain.btnEnumerateWindowStationsClick(Sender: TObject);
begin
  FWindowStations.Clear;
  EnumWindowStations(@EnumWindowStationsCallBackProc, 0);
  cmbWindowStations.Items.Assign(FWindowStations);
  cmbWindowStations.Text := cmbWindowStations.Items[0];
end;

procedure TfmMain.btnWaitThenEnumerateThreadsClick(Sender: TObject);
var
  Processes: TProcesses;
begin
  Sleep(2000);
  meMessages.Lines.Add('2s');
  Sleep(4000);
  meMessages.Lines.Add('6s');
  Sleep(4000);
  meMessages.Lines.Add('10s');
  SDEnumerateProcesses(Processes, lvDetailed);
  SDProcessAllPIDS(Processes, @SDDumpProcessToken, 0);
  SDProcessAllPIDS(Processes, @SDDumpProcessThreads, 0);
end;

procedure TfmMain.DisplayAccessMask(const AMask: Cardinal; const AList: TStrings);
type
  TAccessRight = record
    Value: Cardinal;
    Name: string;
  end;
const
  AccessRight: array [0 .. 21] of TAccessRight = ((Value: 0; Name: '(Start)'), (Value: $00010000;
    Name: '_DELETE'), (Value: $0001; Name: 'DESKTOP_READOBJECTS'), (Value: $0002;
    Name: 'DESKTOP_CREATEWINDOW'), (Value: $0004; Name: 'DESKTOP_CREATEMENU'), (Value: $0008;
    Name: 'DESKTOP_HOOKCONTROL'), (Value: $0010; Name: 'DESKTOP_JOURNALRECORD'), (Value: $0020;
    Name: 'DESKTOP_JOURNALPLAYBACK'), (Value: $0040; Name: 'DESKTOP_ENUMERATE'), (Value: $0080;
    Name: 'DESKTOP_WRITEOBJECTS'), (Value: $0100; Name: 'DESKTOP_SWITCHDESKTOP'), (Value: $0000FFFF;
    Name: 'SPECIFIC_RIGHTS_ALL'), (Value: $00020000; Name: 'READ_CONTROL'), (Value: $00040000;
    Name: 'WRITE_DAC'), (Value: $00080000; Name: 'WRITE_OWNER'), (Value: $001F0000;
    Name: 'STANDARD_RIGHTS_ALL'), (Value: $01000000; Name: 'ACCESS_SYSTEM_SECURITY'),
    (Value: $02000000; Name: 'MAXIMUM_ALLOWED'), (Value: $10000000; Name: 'GENERIC_ALL'),
    (Value: $20000000; Name: 'GENERIC_EXECUTE'), (Value: $40000000; Name: 'GENERIC_WRITE'),
    (Value: $80000000; Name: 'GENERIC_READ'));
var
  nAccessRightIndex: Integer;
begin
  // Windows.GENERIC_READ

  for nAccessRightIndex := Low(AccessRight) to High(AccessRight) do
  begin
    if ((AMask and AccessRight[nAccessRightIndex].Value) = AccessRight[nAccessRightIndex]
      .Value) then
    begin
      AList.Add(Format('%s - %s', [IntToHex(AccessRight[nAccessRightIndex].Value, 8),
        AccessRight[nAccessRightIndex].Name]));
    end;
  end;
end;

procedure TfmMain.DisplayACL(const AACL: Windows.PACL; const AList: TStrings);
var
  nACEIndex: Integer;
  DesktopDACL: TJwDAccessControlList;
begin
  DesktopDACL := TJwDAccessControlList.Create(JwaWindows.PACL(AACL));
  for nACEIndex := 0 to DesktopDACL.Count - 1 do
  begin
    AList.Add(Format('ACE %d', [nACEIndex]));
    AList.Add(Format('Account name: %s (SID: %s)',
      [LookupAccountBySID(DesktopDACL[nACEIndex].SID.SID), DesktopDACL[nACEIndex].SID.StringSID]));
    AList.Add(Format('Access mask: %s', [IntToHex(DesktopDACL[nACEIndex].AccessMask, 8)]));
    DisplayAccessMask(DesktopDACL[nACEIndex].AccessMask, AList);
  end;
end;

function EnumWindowStationsCallBackProc(const lpszWindowStation: PChar;
  const lParam: Integer): Boolean;
begin
  fmMain.WindowStations.Add(StrPas(lpszWindowStation));
  fmMain.meMessages.Lines.Add(Format('Window station %d: %s', [fmMain.WindowStations.Count - 1,
    StrPas(lpszWindowStation)]));
  Result := true;
end;

function EnumDesktopsCallBackProc(const lpszDesktop: PChar; const lParam: Integer): Boolean;
begin
  fmMain.Desktops.Add(StrPas(lpszDesktop));
  fmMain.meMessages.Lines.Add(Format('Desktop for %d: %s', [lParam, StrPas(lpszDesktop)]));
  fmMain.ShowDesktopSecurityInformation(lParam, StrPas(lpszDesktop));
  Result := true;
end;

procedure TfmMain.FormCreate(Sender: TObject);
begin
  FWindowStations := TStringList.Create;
  FDesktops := TStringList.Create;
end;

procedure TfmMain.FTPClientAfterClientLogin(Sender: TObject);
begin
  meMessages.Lines.Add('FTPClient.AfterClientLogin');
end;

function TfmMain.LookupAccountBySID(ASID: JwaWindows.PSID): String;
var
  Name: string;
  Domain: string;
  cchName: Cardinal;
  cchDomain: Cardinal;
  peUse: Cardinal;
begin
  cchName := 0;
  cchDomain := 0;
  LookupAccountSid(nil, ASID, nil, cchName, nil, cchDomain, peUse);
  SetLength(Name, cchName);
  SetLength(Domain, cchDomain);
  LookupAccountSid(nil, ASID, PChar(Name), cchName, PChar(Domain), cchDomain, peUse);
  Result := PChar(Domain) + '\' + PChar(Name);
end;

procedure TfmMain.btnEnumerateDesktopsClick(Sender: TObject);
Var
  nIndex: Integer;
begin
  FDesktops.Clear;
  For nIndex := 0 to FWindowStations.Count - 1 do
    If EnumDesktops(OpenWindowStation(PChar(FWindowStations[nIndex]), false, WINSTA_ENUMERATE or
      WINSTA_ENUMDESKTOPS), @EnumDesktopsCallBackProc, nIndex) Then
      meMessages.Lines.Add(Format('EnumDesktops() failed for %d: %s',
        [nIndex, SysErrorMessage(GetLastError)]))
    Else
      meMessages.Lines.Add('EnumDesktops() succeeded');
  cmbDesktops.Items.Assign(FDesktops);
  if cmbDesktops.Items.Count > 0 then
    cmbDesktops.Text := cmbDesktops.Items[0];
end;

procedure TfmMain.btnEnumerateSessionsClick(Sender: TObject);
var
  SessionInfo: PWTS_SESSION_INFO;
  cCount: Cardinal;
  nIndex: Integer;
begin
  cCount := 0;
  if WTSEnumerateSessions(0, 0, 1, SessionInfo, cCount) then
  begin
    meMessages.Lines.Add('WTSGetActiveConsoleSessionId(): ' +
      IntToStr(WTSGetActiveConsoleSessionId()));
    meMessages.Lines.Add('WTSEnumerateSessions() succeeded.');
    meMessages.Lines.Add('cCount: ' + IntToStr(cCount));
    for nIndex := 0 to cCount - 1 do
    begin
      meMessages.Lines.Add
        (Format('SessionId: %d  pWinStationName ("session name"): "%s"  State: %d',
        [Cardinal(SessionInfo^.SessionId), string(SessionInfo^.pWinStationName),
        Cardinal(SessionInfo^.State)]));
      Inc(SessionInfo);
    end;
  end
  else
  begin
    meMessages.Lines.Add('WTSEnumerateSessions() failed.');
  end;
end;

procedure TfmMain.btnCreateNewDesktopClick(Sender: TObject);
Var
  hDesktop: HDESK;
  // SecurityAttributes: TSECURITYATTRIBUTES;
begin
  hDesktop := CreateDesktop(PChar(Format('NewDesktop%d', [random(65536)])), NIL, NIL, 0,
    DESKTOP_CREATEMENU or DESKTOP_CREATEWINDOW or DESKTOP_ENUMERATE or DESKTOP_HOOKCONTROL or
    DESKTOP_JOURNALPLAYBACK or DESKTOP_JOURNALRECORD or DESKTOP_READOBJECTS or
    DESKTOP_SWITCHDESKTOP or DESKTOP_WRITEOBJECTS, NIL);
  If hDesktop = 0 Then
  Begin
    meMessages.Lines.Add(Format('CreateDesktop() failed: %s', [SysErrorMessage(GetLastError)]));
  End
  Else
  Begin
    meMessages.Lines.Add(Format('Handle of new desktop: %d', [hDesktop]));
  End;
end;

procedure TfmMain.ShowDesktopSecurityInformation(const nWindowStationIndex: Integer;
  const sDesktopName: String);
Var
  hDesktop: HDESK;
  SIRequested: SECURITY_INFORMATION;
  pSecDescriptor: Windows.PSecurityDescriptor;
  nLengthNeeded: Cardinal;
  nBytesReserved: Integer;
  OwnerName: PChar;
  cbName: Cardinal;
  ReferencedDomainName: PChar;
  cbReferencedDomainName: Cardinal;
  peUse: Cardinal;
  pOwner: Windows.PSID;
  lpbOwnerDefaulted: LongBool;
  lpbDACLPresent: LongBool;
  pDACL: Windows.PACL;
  lpbDACLDefaulted: LongBool;
begin
  If nWindowStationIndex = 0 Then
  Begin
    hDesktop := OpenDesktop(PChar(sDesktopName), 0, false, DESKTOP_CREATEMENU or
      DESKTOP_CREATEWINDOW or DESKTOP_ENUMERATE or DESKTOP_HOOKCONTROL or DESKTOP_JOURNALPLAYBACK or
      DESKTOP_JOURNALRECORD or DESKTOP_READOBJECTS or DESKTOP_SWITCHDESKTOP or
      DESKTOP_WRITEOBJECTS or READ_CONTROL);
    If hDesktop = 0 Then
    Begin
      meMessages.Lines.Add(Format('OpenDesktop() failed: %s', [SysErrorMessage(GetLastError)]));
    End
    Else
    Begin
      meMessages.Lines.Add(Format('OpenDesktop(%s), handle: %d', [sDesktopName, hDesktop]));
      SIRequested := OWNER_SECURITY_INFORMATION or GROUP_SECURITY_INFORMATION or
        DACL_SECURITY_INFORMATION; // or SACL_SECURITY_INFORMATION;
      pSecDescriptor := NIL;
      Windows.GetUserObjectSecurity(hDesktop, SIRequested, pSecDescriptor, 0, nLengthNeeded);
      // meMessages.Lines.Add(Format('GetUserObjectSecurity() completed. LastError: %s',
      // [SysErrorMessage(GetLastError)]));
      nBytesReserved := nLengthNeeded;
      GetMem(pSecDescriptor, nLengthNeeded);
      SIRequested := OWNER_SECURITY_INFORMATION or GROUP_SECURITY_INFORMATION or
        DACL_SECURITY_INFORMATION; // or
      // SACL_SECURITY_INFORMATION;
      If Windows.GetUserObjectSecurity(hDesktop, SIRequested, pSecDescriptor, nBytesReserved,
        nLengthNeeded) Then
      Begin
        meMessages.Lines.Add(Format('GetUserObjectSecurity() succeeded', []));

        Windows.GetSecurityDescriptorOwner(pSecDescriptor, pOwner, lpbOwnerDefaulted);

        cbName := 2048 + 1;
        GetMem(OwnerName, cbName);
        cbReferencedDomainName := 2048 + 1;
        GetMem(ReferencedDomainName, cbReferencedDomainName);
        if LookupAccountSid(nil, pOwner, OwnerName, cbName, ReferencedDomainName,
          cbReferencedDomainName, peUse) then
        begin
          meMessages.Lines.Add(Format('LookupAccountSid() succeeded', []));
          meMessages.Lines.Add(Format('Owner name: %s', [StrPas(OwnerName)]));
          meMessages.Lines.Add(Format('Domain: %s', [StrPas(ReferencedDomainName)]));
        end
        else
        begin
          meMessages.Lines.Add(Format('LookupAccountSid() failed. LastError: %s',
            [SysErrorMessage(GetLastError)]));
        end;

        Windows.GetSecurityDescriptorDacl(pSecDescriptor, lpbDACLPresent, pDACL, lpbDACLDefaulted);

        if lpbDACLPresent then
        begin
          meMessages.Lines.Add(Format('GetSecurityDescriptorDacl() succeeded', []));
          meMessages.Lines.Add(Format('ACECount: %d', [pDACL^.AceCount]));
          DisplayACL(pDACL, meMessages.Lines);
        end
        else
        begin
          meMessages.Lines.Add(Format('GetSecurityDescriptorDacl() failed. LastError: %s',
            [SysErrorMessage(GetLastError)]));
        end;
      End
      Else
      Begin
        meMessages.Lines.Add(Format('GetUserObjectSecurity() failed. ' + 'nBytesReserved: %d, ' +
          'nLengthNeeded: %d, ' + 'LastError: %s', [nBytesReserved, nLengthNeeded,
          SysErrorMessage(GetLastError)]));
      End;

    End;
    CloseDesktop(hDesktop);
  End;
end;

procedure TfmMain.btnCreateSwitchSwitchCloseClick(Sender: TObject);
Var
  hOldDesktop: HDESK;
  sNewDesktop: string;
  hNewDesktop: HDESK;
  lpProcessAttributes: PSecurityAttributes;
  lpThreadAttributes: PSecurityAttributes;
  lpStartupInfo: TStartupInfo;
  lpProcessInformation: TProcessInformation;
begin
  hOldDesktop := GetThreadDesktop(GetCurrentThreadId);
  meMessages.Lines.Add(Format('hOldDesktop: %d', [hOldDesktop]));
  if hOldDesktop <> 0 then
  begin
    sNewDesktop := Format('NewDesktop%d', [random(65536)]);
    hNewDesktop := CreateDesktop(PChar(sNewDesktop), nil, nil, DF_ALLOWOTHERACCOUNTHOOK,
      DESKTOP_CREATEMENU or DESKTOP_CREATEWINDOW or DESKTOP_ENUMERATE or DESKTOP_HOOKCONTROL or
      DESKTOP_JOURNALPLAYBACK or DESKTOP_JOURNALRECORD or DESKTOP_READOBJECTS or
      DESKTOP_SWITCHDESKTOP or DESKTOP_WRITEOBJECTS, nil);
    if hNewDesktop = 0 then
    begin
      meMessages.Lines.Add(Format('CreateDesktop() failed: %s', [SysErrorMessage(GetLastError)]));
    end
    else
    begin
      meMessages.Lines.Add(Format('Handle of new desktop: %d', [hNewDesktop]));
      if SwitchDesktop(hNewDesktop) then
      begin
        meMessages.Lines.Add('Switched to new desktop');
        FillChar(lpStartupInfo, SizeOf(TStartupInfo), 0);
        FillChar(lpProcessInformation, SizeOf(TProcessInformation), 0);
        lpStartupInfo.cb := SizeOf(TStartupInfo);
        lpStartupInfo.lpDesktop := PChar(sNewDesktop);
        CreateProcess(PChar('ShowPicture.exe'), PChar('ShowPicture.exe 3'), nil, nil, false, 0, nil,
          nil, lpStartupInfo, lpProcessInformation);
        // CreateProcessAsUser(0, // hToken
        // PChar('ShowPicture.exe'), nil, // lpCommandLine
        // 0, // lpProcessAttributes
        // 0, // lpThreadAttributes
        // false, // bInheritHandles
        // 0, // dwCreationFlags
        // nil, // lpEnvironment
        // nil, // lpCurrentDurectory
        // lpStartupInfo, lpProcessInformation);
      end;
      Sleep(15000);
      if SwitchDesktop(hOldDesktop) then
      begin
        meMessages.Lines.Add('Switched back to old desktop');
      end;
    end;
  end
  else
  begin
    meMessages.Lines.Add('Will not switch');
  end;
end;

type
  Tfunc_GetSecureViewerDC = function(): HDC; stdcall;

type
  Tfunc_UpdateSecureViewer = procedure(); stdcall;

type
  Tfunc_ReleaseSecureViewerDC = procedure();

procedure TfmMain.btnDLLClick(Sender: TObject);
var
  hSecureViewerDLL: HMODULE;
  sSecureViewerDLLFileName: String;
  f_GetSecureViewerDC: Tfunc_GetSecureViewerDC;
  f_UpdateSecureViewer: Tfunc_UpdateSecureViewer;
  f_ReleaseSecureViewerDC: Tfunc_ReleaseSecureViewerDC;
  AImage: TImage;
  sFile: Array of String;
  nIndex: Integer;
begin
  sSecureViewerDLLFileName := 'SecureViewer.dll';
  hSecureViewerDLL := LoadLibrary(PChar(sSecureViewerDLLFileName));
  if hSecureViewerDLL <> NULL then
  begin
    try
      meMessages.Lines.Add(Format('LoadLibrary(%s)', [sSecureViewerDLLFileName]));
      f_GetSecureViewerDC := GetProcAddress(hSecureViewerDLL, 'GetSecureViewerDC');
      f_UpdateSecureViewer := GetProcAddress(hSecureViewerDLL, 'UpdateSecureViewer');
      f_ReleaseSecureViewerDC := GetProcAddress(hSecureViewerDLL, 'ReleaseSecureViewerDC');
      if Assigned(f_GetSecureViewerDC) and Assigned(f_UpdateSecureViewer) and
        Assigned(f_ReleaseSecureViewerDC) then
      begin
        meMessages.Lines.Add('GetProcAddress: ok');
        SetLength(sFile, 2);
        sFile[0] := 'C:\Firma\WinStaTest\media\WinStaTest.bmp';
        sFile[1] := 'C:\Firma\WinStaTest\media\WinStaTest2.bmp';
        // AImage.Canvas.HandleAllocated:=true;
        for nIndex := Low(sFile) to High(sFile) do
        begin
          AImage := TImage.Create(Self);
          AImage.AutoSize := true;
          AImage.Picture.Bitmap.LoadFromFile(sFile[nIndex]);
          meMessages.Lines.Add(Format('AImage.Picture.Bitmap.LoadFromFile(%s);', [sFile[nIndex]]));

          AImage.Picture.Bitmap.Canvas.TextOut(50, 50, Format('File: %s', [sFile[nIndex]]));
          TextOut(AImage.Picture.Bitmap.Canvas.Handle, 50, 150, '2. Text', 7);
          BitBlt(f_GetSecureViewerDC, 0, 0, AImage.Width, AImage.Height, AImage.Canvas.Handle, 0,
            0, SRCCOPY);
          f_UpdateSecureViewer;
          Sleep(4500);
          f_ReleaseSecureViewerDC;
          Application.ProcessMessages;
          Sleep(500);
        end;
      end
      else
      begin
        meMessages.Lines.Add('GetProcAddress: failed');
      end;
    finally
      FreeLibrary(hSecureViewerDLL);
    end;
  end
  else
  begin
    meMessages.Lines.Add(Format('LoadLibrary failed: %s', [SysErrorMessage(GetLastError)]));
  end;
end;

end.

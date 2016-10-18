unit SDInfoSecurity;

interface

uses
  SDInfoProcesses,
  Windows,
  JwaWindows;

// Move to SDInfoProcesses or SDModifiedTokens once dependence on Jwa* is removed
function SDDumpProcessToken(var AProcessData: TProcessData): Boolean;
function SDDumpACL(const AACL: Windows.PACL): Boolean;
function SDAddGroupToTokenGroups(const Groups: JwaWindows.PTokenGroups; const AGroupSid: PSID)
  : JwaWindows.PTokenGroups;

function SDDumpSecurityInfo(const ASecurityDescriptor: Windows.PSecurityDescriptor): Boolean;

implementation

uses
  SDCommon,
  SDModifiedTokens,
  SysUtils,
  JwsclSid,
  JwsclToken,
  JwsclTypes,
  JwsclACL;

function AceTypeToText(const AAceType: TJwAceType): string;
begin
  case AAceType of
    actAudit:
      Result := 'actAudit';
    actAuditCallback:
      Result := 'actAuditCallback';
    actAuditObject:
      Result := 'actAuditObject';
    actAuditCallbackObject:
      Result := 'actAuditCallbackObject';

    actMandatory:
      Result := 'actMandatory';

    actAllow:
      Result := 'actAllow';
    actAllowCallback:
      Result := 'actAllowCallback';
    actAllowObject:
      Result := 'actAllowObject';
    actAllowCallbackObject:
      Result := 'actAllowCallbackObject';

    actDeny:
      Result := 'actDeny';
    actDenyCallback:
      Result := 'actDenyCallback';
    actDenyObject:
      Result := 'actDenyObject';
    actDenyCallbackObject:
      Result := 'actDenyCallbackObject';

    actUnknown:
      Result := 'actUnknown';
  else
    Result := '';
  end;
end;

function SDDumpProcessToken(var AProcessData: TProcessData): Boolean;
var
  hProcess: THandle;
  hToken: THandle;
  ProcessToken: TJwSecurityToken;
  nGroupIndex: Integer;
begin
  Log(Format('SDDumpProcessToken(PID: %d %s)', [AProcessData.PID, AProcessData.FileName]));
  Result := false;
  hProcess := OpenProcess(PROCESS_QUERY_INFORMATION OR PROCESS_VM_READ, false, AProcessData.PID);
  if (hProcess <> 0) then
  begin
    if OpenProcessToken(hProcess, TOKEN_ALL_ACCESS, hToken) then
    begin
      Log(Format('OpenProcessToken() succeeded', []));
      try
        ProcessToken := TJwSecurityToken.CreateDuplicateExistingToken(hToken, TOKEN_ALL_ACCESS);
        // Log(Format('ProcessToken.GetUserName: %s', [ProcessToken.GetTokenUserName]));
        Log(Format('GetTokenUser: %s', [GetTokenUser(hToken)]));

        for nGroupIndex := 0 to ProcessToken.GetTokenGroups.Count - 1 do
        begin
          try
            Log(Format('  Group %.2d: %s', [nGroupIndex,
              GetSidUserName(ProcessToken.GetTokenGroups.Items[nGroupIndex].SID)]));
          except
            on E: Exception do
            begin
              Log(E.Message);
            end;
          end;
        end;
        // Log(Format('GetTokenGroups: %s', [GetTokenGroups(hToken)]));

        AProcessData.SessionID := GetTokenSessionId(hToken);
        Log(Format('GetTokenSessionId: %d', [AProcessData.SessionID]));

        // ProcessToken := TJwSecurityToken.CreateNewToken();
        Log(Format('', []));

      except
        Log(Format('  LastError: %s', [SysErrorMessage(GetLastError)]));
      end;
      Result := true;
    end
    else
    begin
      Log(Format('OpenProcessToken failed, hProcess: %d, LastError: %s',
        [hProcess, SysErrorMessage(GetLastError)]));
    end;
  end
  else
  begin
    Log(Format('OpenProcess() failed, LastError: %s', [SysErrorMessage(GetLastError)]));
  end;
  CloseHandle(hProcess);
end;

function SDDumpACL(const AACL: Windows.PACL): Boolean;
var
  nACEIndex: Integer;
  DesktopDACL: TJwDAccessControlList;
begin
  DesktopDACL := TJwDAccessControlList.Create(JwaWindows.PACL(AACL));
  for nACEIndex := 0 to DesktopDACL.Count - 1 do
  begin
    Log(Format('ACE %d', [nACEIndex]));
    Log(Format('Account name: %s (SID: %s)', [SDLookupAccountBySID(DesktopDACL[nACEIndex].SID.SID),
      DesktopDACL[nACEIndex].SID.StringSID]));
    Log(Format('ACE type: %s', [AceTypeToText(DesktopDACL[nACEIndex].AceType)]));
    // Log(Format('ACE: %s', [DesktopDACL[nACEIndex].GetText]));
    Log(Format('Access mask: %s', [IntToHex(DesktopDACL[nACEIndex].AccessMask, 8)]));
    SDDumpAccessMask(DesktopDACL[nACEIndex].AccessMask);
  end;
  Result := true;
end;

function SDAddGroupToTokenGroups(const Groups: JwaWindows.PTokenGroups; const AGroupSid: PSID)
  : JwaWindows.PTokenGroups;
var
  nGroupIndex: Integer;
begin
  Log(Format('SDAddGroupToToken(Groups: %p)', [Groups]));

  GetMem(Result, SizeOf(Groups^.GroupCount) + (Groups^.GroupCount + 1) * SizeOf(TSIDAndAttributes));
  Result.GroupCount := Groups^.GroupCount + 1;
  for nGroupIndex := 0 to (Groups^.GroupCount - 1) do
  begin
    Result.Groups[nGroupIndex].SID := Groups^.Groups[nGroupIndex].SID;
    Result.Groups[nGroupIndex].Attributes := Groups^.Groups[nGroupIndex].Attributes;
  end;
  Result.Groups[Result.GroupCount - 1].SID := AGroupSid;
  Result.Groups[Result.GroupCount - 1].Attributes := SE_GROUP_ENABLED OR
    SE_GROUP_ENABLED_BY_DEFAULT OR SE_GROUP_MANDATORY;
end;

function SDDumpSecurityInfo(const ASecurityDescriptor: Windows.PSecurityDescriptor): Boolean;
var
  pOwner: Windows.PSID;
  lpbOwnerDefaulted: LongBool;
  cbName: Cardinal;
  OwnerName: PChar;
  cbReferencedDomainName: Cardinal;
  ReferencedDomainName: PChar;
  peUse: Cardinal;
  lpbDACLPresent: LongBool;
  pDACL: Windows.PACL;
  lpbDACLDefaulted: LongBool;
begin
  Windows.GetSecurityDescriptorOwner(ASecurityDescriptor, pOwner, lpbOwnerDefaulted);

  cbName := 2048 + 1;
  GetMem(OwnerName, cbName);
  cbReferencedDomainName := 2048 + 1;
  GetMem(ReferencedDomainName, cbReferencedDomainName);
  if LookupAccountSid(nil, pOwner, OwnerName, cbName, ReferencedDomainName, cbReferencedDomainName,
    peUse) then
  begin
    Log(Format('LookupAccountSid() succeeded', []));
    Log(Format('Owner name: %s', [StrPas(OwnerName)]));
    Log(Format('Domain: %s', [StrPas(ReferencedDomainName)]));
  end
  else
  begin
    Log(Format('LookupAccountSid() failed. LastError: %s', [SysErrorMessage(GetLastError)]));
  end;

  Windows.GetSecurityDescriptorDacl(ASecurityDescriptor, lpbDACLPresent, pDACL, lpbDACLDefaulted);

  if lpbDACLPresent then
  begin
    Log(Format('GetSecurityDescriptorDacl() succeeded', []));
    Log(Format('ACECount: %d', [pDACL^.AceCount]));
    SDDumpACL(pDACL);
  end
  else
  begin
    Log(Format('GetSecurityDescriptorDacl() failed. LastError: %s',
      [SysErrorMessage(GetLastError)]));
  end;
  Result := true;
end;

end.

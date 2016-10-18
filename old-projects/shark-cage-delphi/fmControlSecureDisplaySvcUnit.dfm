object fmControlSecureDisplaySvc: TfmControlSecureDisplaySvc
  Left = 0
  Top = 0
  Caption = 'fmControlSecureDisplaySvc'
  ClientHeight = 242
  ClientWidth = 267
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  Position = poScreenCenter
  OnCreate = FormCreate
  PixelsPerInch = 96
  TextHeight = 13
  object lblServiceStatus: TLabel
    Left = 8
    Top = 70
    Width = 76
    Height = 13
    Caption = 'lblServiceStatus'
  end
  object btnQueryServiceStatus: TButton
    Left = 8
    Top = 39
    Width = 249
    Height = 25
    Caption = 'Query secure display service status'
    TabOrder = 1
    OnClick = btnQueryServiceStatusClick
  end
  object btnLaunchApplication: TButton
    Left = 8
    Top = 89
    Width = 249
    Height = 25
    Caption = 'Launch application'
    TabOrder = 2
    OnClick = btnLaunchApplicationClick
  end
  object cmbApplication: TComboBox
    Left = 8
    Top = 120
    Width = 249
    Height = 21
    Style = csDropDownList
    TabOrder = 3
  end
  object btnReturnFromApplication: TButton
    Left = 8
    Top = 147
    Width = 249
    Height = 25
    Caption = 'Return from application'
    TabOrder = 4
    OnClick = btnReturnFromApplicationClick
  end
  object btnStopService: TButton
    Left = 8
    Top = 207
    Width = 249
    Height = 25
    Caption = 'Stop secure display service'
    TabOrder = 5
    OnClick = btnStopServiceClick
  end
  object btnStartservice: TButton
    Left = 8
    Top = 8
    Width = 249
    Height = 25
    Caption = 'Start secure display service'
    TabOrder = 0
    OnClick = btnStartserviceClick
  end
  object ConsumeWebService: THTTPRIO
    HTTPWebNode.UseUTF8InHeader = True
    HTTPWebNode.InvokeOptions = [soIgnoreInvalidCerts, soAutoCheckAccessPointViaUDDI]
    HTTPWebNode.WebNodeOptions = []
    Converter.Options = [soSendMultiRefObj, soTryAllSchema, soRootRefNodesToBody, soCacheMimeResponse, soUTF8EncodeXML]
    Left = 216
    Top = 160
  end
end

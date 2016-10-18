object fmAppInfo: TfmAppInfo
  Left = 0
  Top = 0
  BorderStyle = bsNone
  Caption = 'fmAppInfo'
  ClientHeight = 347
  ClientWidth = 682
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  Visible = True
  OnCreate = FormCreate
  OnDestroy = FormDestroy
  PixelsPerInch = 96
  TextHeight = 13
  object panelBackground: TPanel
    Left = 192
    Top = 24
    Width = 457
    Height = 305
    BevelOuter = bvNone
    Caption = 'panelBackground'
    ParentBackground = False
    TabOrder = 0
    object imgBackground: TImage
      Left = 16
      Top = 16
      Width = 105
      Height = 105
      AutoSize = True
    end
    object panelAppInfo: TPanel
      Left = 184
      Top = 16
      Width = 257
      Height = 129
      BevelOuter = bvNone
      Caption = 'panelAppInfo'
      ParentBackground = False
      TabOrder = 0
      object imgApplicationLogo: TImage
        Left = 127
        Top = 8
        Width = 105
        Height = 105
        AutoSize = True
      end
      object lblApplicationName: TLabel
        Left = 7
        Top = 39
        Width = 89
        Height = 13
        Caption = 'lblApplicationName'
      end
      object btnHideApp: TButton
        Left = 16
        Top = 58
        Width = 75
        Height = 25
        Caption = 'btnHideApp'
        TabOrder = 0
        OnClick = btnHideAppClick
      end
    end
  end
  object CloseTimer: TTimer
    Enabled = False
    Interval = 12000
    OnTimer = CloseTimerTimer
    Left = 72
    Top = 96
  end
  object RefreshTimer: TTimer
    Enabled = False
    Interval = 20
    OnTimer = RefreshTimerTimer
    Left = 72
    Top = 48
  end
  object ConsumeWebService: THTTPRIO
    HTTPWebNode.UseUTF8InHeader = True
    HTTPWebNode.InvokeOptions = [soIgnoreInvalidCerts, soAutoCheckAccessPointViaUDDI]
    HTTPWebNode.WebNodeOptions = []
    Converter.Options = [soSendMultiRefObj, soTryAllSchema, soRootRefNodesToBody, soCacheMimeResponse, soUTF8EncodeXML]
    Left = 72
    Top = 144
  end
end

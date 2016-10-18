object fmMain: TfmMain
  Left = 268
  Top = 107
  Caption = 'fmMain'
  ClientHeight = 702
  ClientWidth = 645
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  OnCreate = FormCreate
  PixelsPerInch = 96
  TextHeight = 13
  object StatusBar1: TStatusBar
    Left = 0
    Top = 683
    Width = 645
    Height = 19
    Panels = <>
  end
  object meMessages: TMemo
    Left = 0
    Top = 457
    Width = 645
    Height = 226
    Align = alClient
    Lines.Strings = (
      'meMessages')
    ScrollBars = ssVertical
    TabOrder = 1
  end
  object panelButtons: TPanel
    Left = 0
    Top = 0
    Width = 645
    Height = 457
    Align = alTop
    BevelOuter = bvNone
    TabOrder = 2
    object meTODO: TMemo
      Left = 336
      Top = 8
      Width = 289
      Height = 281
      Lines.Strings = (
        'meTODO'
        '- Enumerate window stations'
        '- Enumerate desktops in window stations'
        '- Create new desktop in interactive window station'
        '- Create new desktop in interactive window station using a '
        'non-NULL security descriptor'
        '- Run program under different account so that only this '
        'account has access to the new desktop'
        '- Switch to new desktop, show form, switch back to '
        'common desktop'
        '- Switch to new desktop from a service, i.e., create new '
        'desktop on startup, then create new process in desktop on '
        'demand and switch to it'
        '- Transfer content of e.g. a bitmap to the service to have it '
        'displayed'
        '- Offer access to the bitmap via a DC, e.g., from a DLL '
        'loaded by other processes'
        '- Try to access the desktop from other processes not '
        'running under the special account'
        '- Adhere to standard like SicCT to simulate class 3 terminal?')
      TabOrder = 0
    end
    object btnDLL: TButton
      Left = 16
      Top = 264
      Width = 297
      Height = 25
      Caption = 'btnDLL'
      TabOrder = 1
      OnClick = btnDLLClick
    end
    object btnUploadFileToServer: TButton
      Left = 16
      Top = 216
      Width = 297
      Height = 25
      Caption = 'btnUploadFileToServer'
      TabOrder = 2
    end
    object btnCreateSwitchSwitchClose: TButton
      Left = 16
      Top = 176
      Width = 297
      Height = 25
      Caption = 'btnCreateSwitchSwitchClose'
      TabOrder = 3
      OnClick = btnCreateSwitchSwitchCloseClick
    end
    object btnCreateNewDesktop: TButton
      Left = 16
      Top = 145
      Width = 297
      Height = 25
      Caption = 'btnCreateNewDesktop'
      TabOrder = 4
      OnClick = btnCreateNewDesktopClick
    end
    object btnEnumerateDesktops: TButton
      Left = 16
      Top = 80
      Width = 297
      Height = 25
      Caption = 'btnEnumerateDesktops'
      TabOrder = 5
      OnClick = btnEnumerateDesktopsClick
    end
    object btnEnumerateWindowStations: TButton
      Left = 16
      Top = 16
      Width = 297
      Height = 25
      Caption = 'btnEnumerateWindowStations'
      TabOrder = 6
      OnClick = btnEnumerateWindowStationsClick
    end
    object cmbWindowStations: TComboBox
      Left = 16
      Top = 47
      Width = 297
      Height = 21
      TabOrder = 7
      Text = 'cmbWindowStations'
    end
    object cmbDesktops: TComboBox
      Left = 16
      Top = 111
      Width = 297
      Height = 21
      TabOrder = 8
      Text = 'cmbDesktops'
    end
    object btnEnumerateSessions: TButton
      Left = 16
      Top = 295
      Width = 297
      Height = 25
      Caption = 'btnEnumerateSessions'
      TabOrder = 9
      OnClick = btnEnumerateSessionsClick
    end
    object btnWaitThenEnumerateThreads: TButton
      Left = 16
      Top = 326
      Width = 297
      Height = 25
      Caption = 'btnWaitThenEnumerateThreads'
      TabOrder = 10
      OnClick = btnWaitThenEnumerateThreadsClick
    end
  end
end

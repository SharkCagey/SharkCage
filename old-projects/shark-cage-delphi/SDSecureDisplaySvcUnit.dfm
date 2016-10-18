object SDSecureDisplayService: TSDSecureDisplayService
  OldCreateOrder = False
  DisplayName = 'Secure Display Service'
  AfterInstall = ServiceAfterInstall
  OnExecute = ServiceExecute
  Height = 150
  Width = 215
  object timSafetyTimer: TTimer
    Enabled = False
    OnTimer = timSafetyTimerTimer
    Left = 24
    Top = 16
  end
end

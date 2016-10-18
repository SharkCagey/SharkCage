object SecureDisplaySvcWeb: TSecureDisplaySvcWeb
  OldCreateOrder = False
  Actions = <
    item
      Default = True
      Name = 'DefaultHandler'
      PathInfo = '/'
    end
    item
      Name = 'AppStartexclusive'
      PathInfo = '/soap'
    end>
  Height = 230
  Width = 415
  object HTTPSoapDispatcher: THTTPSoapDispatcher
    Dispatcher = HTTPSoapPascalInvoker
    WebDispatch.PathInfo = 'soap*'
    Left = 60
    Top = 11
  end
  object HTTPSoapPascalInvoker: THTTPSoapPascalInvoker
    Converter.Options = [soSendMultiRefObj, soTryAllSchema, soUTF8InHeader]
    Left = 60
    Top = 67
  end
end

object Form1: TForm1
  Left = 0
  Top = 0
  BorderIcons = [biSystemMenu, biMinimize]
  BorderStyle = bsSingle
  Caption = 'Form1'
  ClientHeight = 338
  ClientWidth = 591
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  Position = poDesktopCenter
  OnActivate = FormActivate
  OnDestroy = FormDestroy
  PixelsPerInch = 96
  TextHeight = 13
  object PageControl1: TPageControl
    Left = 0
    Top = 0
    Width = 591
    Height = 338
    ActivePage = TabSheet1
    Align = alClient
    TabOrder = 0
    TabPosition = tpBottom
    ExplicitHeight = 312
    object TabSheet1: TTabSheet
      Caption = ' Firmware Update '
      ExplicitHeight = 286
      object BitBtn1: TBitBtn
        Left = 478
        Top = 30
        Width = 75
        Height = 25
        Caption = 'Detect'
        TabOrder = 0
        OnClick = BitBtn1Click
      end
      object BitBtn2: TBitBtn
        Left = 478
        Top = 252
        Width = 75
        Height = 25
        Caption = 'Close'
        TabOrder = 1
        OnClick = BitBtn2Click
      end
      object BitBtn3: TBitBtn
        Left = 478
        Top = 125
        Width = 75
        Height = 25
        Caption = 'Update DSP'
        TabOrder = 2
        OnClick = BitBtn3Click
      end
      object BitBtn4: TBitBtn
        Left = 478
        Top = 61
        Width = 75
        Height = 25
        Caption = '...'
        TabOrder = 3
        OnClick = BitBtn4Click
      end
      object Edit1: TEdit
        Left = 3
        Top = 259
        Width = 425
        Height = 19
        Color = clCream
        Ctl3D = False
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clNavy
        Font.Height = -11
        Font.Name = 'Tahoma'
        Font.Style = []
        ParentCtl3D = False
        ParentFont = False
        ReadOnly = True
        TabOrder = 4
      end
      object BitBtn5: TBitBtn
        Left = 478
        Top = 156
        Width = 75
        Height = 25
        Caption = 'Update UI'
        TabOrder = 5
        OnClick = BitBtn5Click
      end
      object BitBtn6: TBitBtn
        Left = 478
        Top = 221
        Width = 75
        Height = 25
        Caption = 'Abort'
        TabOrder = 6
        OnClick = BitBtn6Click
      end
      object pg1: TProgressBar
        Left = 3
        Top = 284
        Width = 425
        Height = 19
        TabOrder = 7
      end
    end
  end
  object Memo1: TMemo
    Left = 8
    Top = 8
    Width = 441
    Height = 249
    Color = clCream
    Ctl3D = False
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clNavy
    Font.Height = -11
    Font.Name = 'Tahoma'
    Font.Style = []
    ParentCtl3D = False
    ParentFont = False
    ReadOnly = True
    ScrollBars = ssVertical
    TabOrder = 1
    OnChange = Memo1Change
  end
  object OpenDialog1: TOpenDialog
    Left = 392
    Top = 24
  end
end

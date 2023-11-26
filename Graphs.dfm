object GraphImag: TGraphImag
  Left = 100
  Top = 415
  Caption = 'Images'
  ClientHeight = 379
  ClientWidth = 697
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  PopupMenu = MImagMenu
  Position = poDesigned
  Visible = True
  OnActivate = FormActivate
  OnCreate = FormCreate
  OnResize = FormResize
  PixelsPerInch = 96
  TextHeight = 13
  object LED1Img: TImage
    Left = 428
    Top = 45
    Width = 226
    Height = 156
    Visible = False
    OnMouseDown = LEDImgMouseDown
    OnMouseMove = LEDImgMouseMove
    OnMouseUp = LEDImgMouseUp
  end
  object LED2Img: TImage
    Left = 462
    Top = 160
    Width = 144
    Height = 105
    Enabled = False
    Visible = False
    OnMouseDown = LEDImgMouseDown
    OnMouseMove = LEDImgMouseMove
    OnMouseUp = LEDImgMouseUp
  end
  object LED3Img: TImage
    Left = 501
    Top = 224
    Width = 144
    Height = 105
    Enabled = False
    Visible = False
    OnMouseDown = LEDImgMouseDown
    OnMouseMove = LEDImgMouseMove
    OnMouseUp = LEDImgMouseUp
  end
  object LED4Img: TImage
    Left = 547
    Top = 289
    Width = 144
    Height = 105
    Enabled = False
    Visible = False
    OnMouseDown = LEDImgMouseDown
    OnMouseMove = LEDImgMouseMove
    OnMouseUp = LEDImgMouseUp
  end
  object LEDord1LblG: TLabel
    Left = 72
    Top = 2
    Width = 70
    Height = 13
    Caption = 'LEDord1LblG'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'Tahoma'
    Font.Style = [fsBold]
    ParentFont = False
    Visible = False
  end
  object LEDord2LblG: TLabel
    Left = 208
    Top = 2
    Width = 46
    Height = 13
    Caption = 'LEDord2'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'Tahoma'
    Font.Style = [fsBold]
    ParentFont = False
    Visible = False
  end
  object LEDord3LblG: TLabel
    Left = 360
    Top = 2
    Width = 46
    Height = 13
    Caption = 'LEDord3'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'Tahoma'
    Font.Style = [fsBold]
    ParentFont = False
    Visible = False
  end
  object LEDord4LblG: TLabel
    Left = 488
    Top = 2
    Width = 46
    Height = 13
    Caption = 'LEDord4'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'Tahoma'
    Font.Style = [fsBold]
    ParentFont = False
    Visible = False
  end
  object BrightPorog: TScrollBar
    Left = 1
    Top = 21
    Width = 16
    Height = 124
    Kind = sbVertical
    Min = 1
    PageSize = 1
    Position = 1
    TabOrder = 0
    OnChange = BrightPorogChange
  end
  object LED1Panel: TPanel
    Left = 42
    Top = 28
    Width = 231
    Height = 197
    Caption = 'LED1'
    TabOrder = 1
    Visible = False
    OnMouseDown = LEDImgMouseDown
    OnMouseMove = LEDImgMouseMove
    OnMouseUp = LEDImgMouseUp
  end
  object LED2Panel: TPanel
    Left = 98
    Top = 152
    Width = 120
    Height = 90
    Caption = 'LED2'
    TabOrder = 2
    Visible = False
    OnMouseDown = LEDImgMouseDown
    OnMouseMove = LEDImgMouseMove
    OnMouseUp = LEDImgMouseUp
  end
  object LED3Panel: TPanel
    Left = 134
    Top = 205
    Width = 120
    Height = 90
    Caption = 'LED3'
    TabOrder = 3
    Visible = False
    OnMouseDown = LEDImgMouseDown
    OnMouseMove = LEDImgMouseMove
    OnMouseUp = LEDImgMouseUp
  end
  object LED4Panel: TPanel
    Left = 153
    Top = 261
    Width = 120
    Height = 90
    Caption = 'LED4'
    TabOrder = 4
    Visible = False
    OnMouseDown = LEDImgMouseDown
    OnMouseMove = LEDImgMouseMove
    OnMouseUp = LEDImgMouseUp
  end
  object MImagMenu: TPopupMenu
    Left = 8
    Top = 256
    object SaveClk: TMenuItem
      Caption = 'Save image'
      OnClick = SaveClkClick
    end
    object DeleteROI: TMenuItem
      Caption = 'Delete ROI'
      OnClick = DeleteROIClick
    end
    object DeleteAllROI: TMenuItem
      Tag = 1
      Caption = 'Delete all ROI'
      OnClick = DeleteAllROIClick
    end
  end
end

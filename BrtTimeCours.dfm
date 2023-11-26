object RoiGraph: TRoiGraph
  Left = 800
  Top = 20
  Caption = 'Brightness'
  ClientHeight = 434
  ClientWidth = 643
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  Menu = ImagesMenu
  OldCreateOrder = False
  Position = poDesigned
  Visible = True
  DesignSize = (
    643
    434)
  PixelsPerInch = 96
  TextHeight = 13
  object Label1: TLabel
    Left = 21
    Top = 412
    Width = 29
    Height = 14
    Anchors = [akLeft, akBottom]
    Caption = 'Tag: '
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -12
    Font.Name = 'Tahoma'
    Font.Style = []
    ParentFont = False
  end
  object LedRoi: TChart
    Left = -1
    Top = 0
    Width = 645
    Height = 401
    Title.Text.Strings = (
      'Brightness, percent')
    BottomAxis.Title.Caption = 'Time, s'
    View3D = False
    OnAfterDraw = LedRoiAfterDraw
    TabOrder = 0
    Anchors = [akLeft, akTop, akRight, akBottom]
    PrintMargins = (
      15
      19
      15
      19)
    ColorPaletteIndex = 13
  end
  object TagText: TEdit
    Left = 56
    Top = 407
    Width = 561
    Height = 22
    Anchors = [akLeft, akRight, akBottom]
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -12
    Font.Name = 'Tahoma'
    Font.Style = []
    ParentFont = False
    TabOrder = 1
    OnKeyPress = TagTextKeyPress
  end
  object ImagesMenu: TMainMenu
    Left = 128
    object XScale: TMenuItem
      Caption = 'XScale'
      object TimeScl: TMenuItem
        Caption = 'time'
        Checked = True
        RadioItem = True
        OnClick = TmFrmRwRtClick
      end
      object FrameScl: TMenuItem
        Caption = 'frames'
        RadioItem = True
        OnClick = TmFrmRwRtClick
      end
    end
    object Data1: TMenuItem
      Caption = 'Data'
      object RawData: TMenuItem
        Caption = 'raw'
        Checked = True
        RadioItem = True
        OnClick = TmFrmRwRtClick
      end
      object Ratios: TMenuItem
        Caption = 'ratio'
        RadioItem = True
        OnClick = TmFrmRwRtClick
      end
    end
  end
end

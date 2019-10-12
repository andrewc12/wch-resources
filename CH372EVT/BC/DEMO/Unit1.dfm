object Form1: TForm1
  Left = 314
  Top = 329
  Width = 292
  Height = 170
  Caption = 'Form1'
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  OnDestroy = btnCloseClick
  PixelsPerInch = 96
  TextHeight = 13
  object Label1: TLabel
    Left = 22
    Top = 104
    Width = 35
    Height = 13
    AutoSize = False
    Caption = '¼üºÅ'
  end
  object Label2: TLabel
    Left = 12
    Top = 40
    Width = 85
    Height = 13
    AutoSize = False
    Caption = '×î×ó±ßÊýÂë¹Ü'
  end
  object Label3: TLabel
    Left = 22
    Top = 64
    Width = 35
    Height = 13
    Caption = 'ÃüÁî'
  end
  object btnCmd: TBitBtn
    Left = 184
    Top = 64
    Width = 75
    Height = 25
    Caption = '·¢ËÍ'
    TabOrder = 0
    OnClick = btnCmdClick
  end
  object btnSend: TBitBtn
    Left = 184
    Top = 32
    Width = 75
    Height = 25
    Caption = '·¢ËÍ'
    TabOrder = 1
    OnClick = btnSendClick
  end
  object btnClose: TBitBtn
    Left = 184
    Top = 96
    Width = 75
    Height = 25
    Caption = 'ÍË³ö'
    TabOrder = 2
    OnClick = btnCloseClick
  end
  object edtN1: TEdit
    Left = 103
    Top = 32
    Width = 65
    Height = 21
    TabOrder = 3
    Text = 'edtN1'
    OnKeyPress = edtN1KeyPress
  end
  object edtCmd: TEdit
    Left = 103
    Top = 64
    Width = 65
    Height = 21
    TabOrder = 4
    Text = 'edtCmd'
    OnKeyPress = edtCmdKeyPress
  end
  object btnInt: TButton
    Left = 8
    Top = 8
    Width = 41
    Height = 17
    Caption = 'btnInt'
    TabOrder = 5
    Visible = False
    OnClick = btnIntClick
  end
  object btnRe: TButton
    Left = 56
    Top = 8
    Width = 49
    Height = 17
    Caption = 'btnRe'
    TabOrder = 6
    Visible = False
    OnClick = btnReClick
  end
  object edtInt: TEdit
    Left = 103
    Top = 96
    Width = 65
    Height = 21
    TabOrder = 7
    Text = 'edtInt'
    OnKeyPress = edtCmdKeyPress
  end
end

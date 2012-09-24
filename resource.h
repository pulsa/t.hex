#pragma once

#define IDM_CursorLeft                  100
#define IDM_CursorRight                 101
#define IDM_CursorUp                    102
#define IDM_CursorDown                  103
#define IDM_CursorStartOfLine           104
#define IDM_CursorEndOfLine             105
#define IDM_CursorStartOfFile           106
#define IDM_CursorEndOfFile             107
#define IDM_GotoDlg                     108
#define IDM_GotoAgain                   109
#define IDM_Find                        110
#define IDM_Replace                     111
#define IDM_Save                        112
#define IDM_SaveAs                      113
#define IDM_OpenFile                    114
#define IDM_OpenProcess                 115
#define IDM_FindNext                    116
#define IDM_FindPrevious                117
#define IDM_NextSector                  118
#define IDM_PreviousSector              119
#define IDM_SelectAll                   120
#define IDM_Copy                        121
#define IDM_CopyAsDlg                   122
#define IDM_Paste                       123
#define IDM_Cut                         124
#define IDM_Delete                      125
#define IDM_CopyDlg                     126
#define IDM_PasteDlg                    127
#define IDM_SelectDlg                   128
#define IDM_SelectModeNormal            129
#define IDM_SelectModeBlock             130
#define IDM_SelectModeToggle            131
#define IDM_NextView                    132
#define IDM_PrevView                    133
#define IDM_ViewFontSizeUp              134
#define IDM_ViewFontSizeDown            135
#define IDM_RegionNext                  136
#define IDM_RegionPrev                  137
#define IDM_FontDlg                     139
#define IDM_ViewLineUp                  140
#define IDM_ViewLineDown                141
#define IDM_ViewPageUp                  142
#define IDM_ViewPageDown                143
#define IDM_FullScreen                  144
#define IDM_Undo                        145
#define IDM_Redo                        146
#define IDM_ToggleInsert                147
#define IDM_ViewFileMap                 148
#define IDM_InsertRange                 149
#define IDM_Print                       150
#define IDM_SaveAll                     151
#define IDM_FileProperties              152
#define IDM_UndoAll                     153
#define IDM_RedoAll                     154
#define IDM_ViewDocList                 155
#define IDM_OpsRepeatLast               156
#define IDM_Settings                    157
#define IDM_CycleClipboard              158
#define IDM_Histogram                   159
#define IDM_CollectStrings              160
#define IDM_ViewStatusBar               161
#define IDM_ViewToolBar                 162
#define IDM_ToggleEndianMode            163
#define IDM_ViewNumberView              164
#define IDM_ViewStringView              165
#define IDM_ViewStructureView           166
#define IDM_ViewDisasmView              167
#define IDM_ViewRelativeAddresses       168
#define IDM_ViewAbsoluteAddresses       169
#define IDM_CopyAddress                 170
#define IDM_CopyCurrentAddress          171
#define IDM_ViewAdjustColumns           172
#define IDM_OffsetLeft                  173
#define IDM_OffsetRight                 174
#define IDM_FileNew                     175
#define IDM_ViewRuler                   176
#define IDM_OpenSpecial                 177
#define IDM_WriteSpecial                178
#define IDM_WindowCloseTab              181
//#define IDM_ViewUnicode                 182
#define IDM_ViewASCII                   183
#define IDM_ViewAuto                    184
#define IDM_WordWrap                    185 // StringView pop-up menu
#define IDM_OpenDrive                   186
#define IDM_ReadPalette                 187
#define IDM_ToggleReadOnly              188

#define IDM_ViewStickyAddr              201
#define IDM_ViewUTF8                    202
#define IDM_ViewUTF16                   203
#define IDM_ViewUTF32                   204
#define IDM_CopyCodePoints              205
#define IDM_FindDiffRec                 206
#define IDM_FindDiffRecBack             207
#define IDM_CopySector                  208
#define IDM_About                       209
#define IDM_OpenTestFile                210
#define IDM_OpenProcessFile             211
#define IDM_NextBinChar                 212
#define IDM_PrevBinChar                 213
#define IDM_OpsCustom1                  214
#define IDM_ViewExportView              215
#define IDM_ZipRecover                  216
#define IDM_Compressability             217
#define IDM_SwapOrder2                  218
#define IDM_SwapOrder4                  219
#define IDM_UnZlib                      220

#define IDM_ViewPrevRegion              240
#define IDM_ViewNextRegion              241

//#define IDM_FocusHexWnd                 250

#define IDM_Max                         299

// These items are on a popup menu for the status bar.  Order must match THBASE enum.
#define IDM_BASE_DEC                    300
#define IDM_BASE_HEX                    301
#define IDM_BASE_BOTH                   302
#define IDM_BASE_OCT                    303
#define IDM_BASE_ALL                    304

//#define IDI_THEX                        1000
#define IDI_SIZE1                       1001

#define IDC_SCROLL1                     1000
#define IDC_U                           1000
#define IDC_UR                          1001
#define IDC_R                           1002
#define IDC_DR                          1003
#define IDC_D                           1004
#define IDC_DL                          1005
#define IDC_L                           1006
#define IDC_UL                          1007
#define IDC_UD                          1008
#define IDC_LR                          1009
#define IDC_UDLR                        1010

#define IDD_GOTO                        1000
#define IDD_FIND                        1001
#define IDD_OPTIONS                     1002
#define IDD_COLORS                      1003

// DocList dialog items
#define IDC_DOC_LIST                    100

// Do not edit past here.  rc.exe needs a blank line at EOF.

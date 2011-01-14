#pragma once

#ifndef _STRUCT_H_
#define _STRUCT_H_

class VarInfo
{
public:
    void Init(wxString name = wxEmptyString)
    {
        this->name = name;
        bSigned = true;
        type = 0; // int
        size = 0;
        count = 0;
        base = 0;
        pointerType = PT_NONE;
        IsArray = false;
        pointerBase = 0;
        offset = 0;
    }

    wxString name;
    bool bSigned;
    int type; // 0 = int, 1 = float
    int size; // 1 - 8
    bool IsArray;
    int count; // for arrays
    int base; // 0 = default
    int pointerType; // is this member a pointer to somewhere in this file?
    enum {PT_NONE, PT_RELATIVE, PT_ABSOLUTE};
    THSIZE pointerBase;
    int offset; // offset of this member in structure

    wxString Format(const void *pStart);
    wxString FormatItem(const void *pStart);
    THSIZE GetSize() { return (count || IsArray) ? (THSIZE)size * count : size; }
    //! need something better for char[0] types
    wxString FormatType();
    THSIZE GetPointer(HexWnd *hw, THSIZE base);
};

class StructInfo
{
public:
    wxString name;
    std::vector<VarInfo> vars;

    THSIZE m_tmpOffset;

    bool HasVar(wxString name);
    void Init(wxString name = wxEmptyString);
    void Add(VarInfo &var);
    THSIZE GetSize();
};

class yyFlexLexer;
#define YYPARSE_PARAM dummy, yyFlexLexer *flex, std::vector<StructInfo> *pStructVec
extern int yyparse(void *YYPARSE_PARAM);

#endif //_STRUCT_H_

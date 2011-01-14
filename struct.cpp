#include "precomp.h"
#include "thex.h"
#include "struct.h"
#include "bigint.h"
#include "hexwnd.h"
#include "utils.h"

#define new New

//#include <boost/spirit/core.hpp>
//#include <boost/spirit/actor/push_back_actor.hpp>
//#include <boost/spirit/symbols/symbols.hpp>
//using namespace boost::spirit;
//using namespace std;

//struct set_int
//{
//    set_int(unsigned &u) : m_uint(u) {}
//    void operator()(ScannerT& data) { this->m_uint
//    unsigned &m_uint;
//};

///////////////////////////////////////////////////////////////////////////////
//
//  T. Hex struct grammar
//
///////////////////////////////////////////////////////////////////////////////
//struct thStructGrammar : public grammar<thStructGrammar>
//{
//    template <typename ScannerT>
//    struct definition
//    {
//        definition(thStructGrammar const& self)
//        {
//            //first
//            //    =   +ch_p('M')  [add_1000(self.r)]
//            //    ||  hundreds_p  [add_roman(self.r)]
//            //    ||  tens_p      [add_roman(self.r)]
//            //    ||  ones_p      [add_roman(self.r)];
//
//            //  Note the use of the || operator. The expression
//            //  a || b reads match a or b and in sequence. Try
//            //  defining the roman numerals grammar in YACC or
//            //  PCCTS. Spirit rules! :-)
//
//            sign_r  = eps_p[sign_signed(self.r)]
//                    | "signed"[sign_signed(self.r)]
//                    | "unsigned"[sign_unsigned(self.r)];
//            int_type_r = str_p("char")[type_char(self.r)];
//        }
//
//        rule<ScannerT> sign_r, int_type_r, float_type_r, offset_option_r, base_option_r, optoins_r, declaration_r;
//        rule<ScannerT> const&
//        start() const { return first; }
//    };
//
//    thStructGrammar(unsigned& r_) : r(r_) {}
//    unsigned& r;
//};
//
//bool parse_structures(char const* str, vector<double>& v)
//{
//    symbols<VarInfo> sym;
//
//    sym = "this";
//
//    //VarInfo *var = sym.find("this");
//    VarInfo *var = find(sym, "this");
//
//    rule<> ident_p = ('_' | alpha_p) >> *('_' | alnum_p);
//
//    rule<> datatype_p = (str_p("float") | "double") | (
//        (str_p("signed") | "unsigned" | eps_p) >> (str_p("char") | "short" | "int" | "long" | "__int64")
//        );
//
//    rule<> declaration_p = datatype_p >> ident_p >> ';';
//    return parse(str,
//
//        //  Begin grammar
//        (
//            real_p[push_back_a(v)] >> *(',' >> real_p[push_back_a(v)])
//        )
//        ,
//        //  End grammar
//
//        space_p).full;
//}

wxString VarInfo::Format(const void *pStart)
{
    if (IsArray && count >= 0)
    {
        if (type == 0 && size == 1 && bSigned)  // string = signed character array
        {
            if (count == 0)
            {
                while (my_isprint[((uint8*)pStart)[count]] && count < 100)  //! artificial limit from StructureView::ProcessUpdates
                    count++;
            }
            return _T('"') + wxString((const char*)pStart, wxConvLibc, count) + _T('"');
        }

        wxString str = _T("[");
        for (int i = 0; i < count; i++)
        {
            if (i > 0)
                str += _T(" ");
            str += FormatItem(pStart);
            pStart = (const void*)((uint8*)pStart + size);
        }
        str += _T("]");
        return str;
    }

    return FormatItem(pStart);
}

wxString VarInfo::FormatItem(const void *pStart)
{
    if (count == -1)
        count = this->count;

    if (type == 0) // int
    {
        uint64 n = 0;
        memcpy(&n, pStart, size);
        //wxString str = FormatHex(n);
        int tmpBase = (base ? base : 10); // pick a default base somehow
        wxString str = FormatNumber(n, tmpBase);
        if (size == 1 && bSigned)
        {
            str += _T(" ") + Escape(wxString((wxChar)n));
            //char c = n;
            //if ((unsigned char)c >= ' ')
            //    str += wxString::Format(_T(" '%c'"), c);
            //else if (c == 0x0D)
            //    str += _T("\\r");
            //else if (c == 0x0A)
            //    str += _T("\\n");
            //else if (c == 0x09)
            //    str += _T("\\t");
        }
        return str;
    }
    else //if (type == 1) // float
    {
        if (size == 4)
            return FormatDouble(*(float*)pStart);
        return FormatDouble(*(double*)pStart);
    }
}

bool StructInfo::HasVar(wxString name)
{
    std::vector<VarInfo>::const_iterator iter;
    for (iter = vars.begin(); iter != vars.end(); iter++)
    {
        if ((*iter).name == name)
            return true;
    }
    return false;
}

void StructInfo::Init(wxString name /*= wxEmptyString*/)
{
    this->name = name;
    this->vars.clear();
    m_tmpOffset = 0;
}

void StructInfo::Add(VarInfo &var)
{
    var.offset = m_tmpOffset;
    PRINTF(_T("Adding variable %s\n"), var.name.c_str());
    vars.push_back(var);
    m_tmpOffset += var.GetSize();
}

THSIZE StructInfo::GetSize()
{
    THSIZE size = 0;
    for (size_t i = 0; i < vars.size(); i++)
        size = wxMax(size, vars[i].offset + vars[i].GetSize());
    return size;
}

wxString VarInfo::FormatType()
{
    wxString str;
    if (type == 0) // int
    {
        if (!bSigned)
            str = _T("unsigned ");
        if (size == 1) str += _T("char");
        else if (size == 2) str += _T("short");
        else if (size == 4) str += _T("int");
        else if (size == 8) str += _T("__int64");
    }
    else //if (type == 1) // float
    {
        if (size == 4) str = _T("float");
        else if (size == 8) str = _T("double");
    }

    if (IsArray)
        str += wxString::Format(_T("[%d]"), count);
    return str;
}

THSIZE VarInfo::GetPointer(HexWnd *hw, THSIZE base)
{
    if (pointerType == PT_NONE)
        return base + offset;
    THSIZE target;
    if (!hw->doc->ReadInt(base + offset, size, &target))
        return 0;
    Selection sel = hw->GetSelection();
    //bigint bigtarget;
    if (pointerType == PT_RELATIVE)
        //target += sel.GetFirst() + pointerBase; //! may overflow
        target = Add(Add(target, sel.GetFirst()), pointerBase);
    return target;
}

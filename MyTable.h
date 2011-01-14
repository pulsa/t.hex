#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifndef max
#define max(a, b) (((a) < (b)) ? (b) : (a))
#endif

/*
Table formatting class by Adam Bailey, 2008-08-06.

Use:
    MyTable tab;
    tab.Init(3);
    tab.Align(2, tab::RIGHT);
    tab.SetHeader("Dish", "Cook", "Score");
    tab.AddRow("Pad Thai", "Vanessa", "9.5");
    tab.AddRow("Ramen Surprise", "Peter", "7.0");
    tab.AddRow("Vegetables", "Jeff", "fail");
    tab.Print(stdout);
*/

#pragma warning(disable: 4200)

typedef struct _MyRow {
    _MyRow *prev, *next;
    char **ptrs;
    char data[0];
} MyRow;

class MyTable
{
public:
    MyTable(int cols = 0)
    {
        if (cols)
            Init(cols);
    }

    virtual ~MyTable()
    {
        delete [] m_colWidth;
        delete [] m_fmt;
        free(m_header);
        for (MyRow *row = m_firstRow; row != NULL; row = row->next)
            free(row->prev);
        free(m_lastRow);
        delete [] m_align;
    }

    enum {LEFT=0, RIGHT=1};
    void Align(int col, int align) { m_align[col] = align; }

    void Init(int cols, int align = LEFT)
    {
        m_firstRow = m_lastRow = m_header = NULL;
        m_nCols = cols;
        m_colWidth = new int[cols];
        m_align = new int[cols];
        for (int i = 0; i < cols; i++)
            Align(i, align);
        m_fmt = NULL;
    }

    void AddRow(const char *col0, ...)
    {
        va_list args;
        va_start(args, col0);
        MyRow *row = NewRowV(col0, args);
        if (m_firstRow == NULL)
            m_firstRow = row;
        else {
            m_lastRow->next = row;
            row->prev = m_lastRow;
        }
        m_lastRow = row;
    }

    void SetHeader(const char *col0, ...)
    {
        delete m_header;
        va_list args;
        va_start(args, col0);
        m_header = NewRowV(col0, args);
    }

    void Print(FILE *f, bool backward = 0)
    {
        delete [] m_fmt;
        m_fmt = new char[20 * m_nCols];
        int len = 0, i;
        for (i = 0; i < m_nCols-1; i++)
        {
            len += sprintf(m_fmt + len, "%%%s%ds  ",
                (m_align[i] == LEFT) ? "-" : "",
                m_colWidth[i]);
        }
        if (m_align[i] == LEFT)
            sprintf(m_fmt + len, "%%s\n");
        else
            sprintf(m_fmt + len, "%%%ds\n", m_colWidth[i]);

        PrintRow(f, m_header);
        PrintSeparator(f);
        if (backward) {
            for (MyRow *row = m_lastRow; row != NULL; row = row->prev)
                PrintRow(f, row);
        }
        else {
            for (MyRow *row = m_firstRow; row != NULL; row = row->next)
                PrintRow(f, row);
        }
        PrintSeparator(f);
        PrintRow(f, m_header);
    }

    void PrintSeparator(FILE *f)
    {
        const char *h = "------------------------------";
        for (int i = 0; i < m_nCols; i++)
            fprintf(f, ((m_colWidth[i] < 30) ? "%.*s%s" : "%-*s%s"),
               m_colWidth[i], h, ((i+1 < m_nCols) ? "  " : "\n"));
    }

protected:
    MyRow *m_firstRow, *m_lastRow, *m_header;
    int m_nCols, *m_colWidth, *m_align;
    char *m_fmt;

    MyRow *NewRowV(const char *col0, va_list args)
    {
        va_list args_save = args;
        size_t size = 0;
        const char *s = col0;
        for (int i = 0; i < m_nCols; i++)
        {
            size += strlen(s) + 1;
            s = (const char*)va_arg(args, char*);
            if (!s) s = "";
        }
        size = (size + 3) & ~3;  // align on 4-byte boundary.
        size_t tsize = size + sizeof(MyRow) + m_nCols * sizeof(char*);
        MyRow *row = (MyRow*)malloc(tsize);
        memset(row, 0, tsize);
        row->ptrs = (char**)(row->data + size);
        args = args_save;
        size = 0;
        s = col0;
        for (int i = 0; i < m_nCols; i++)
        {
            size_t len = strlen(s);
            m_colWidth[i] = max(m_colWidth[i], (int)len);
            row->ptrs[i] = row->data + size;
            memcpy(row->ptrs[i], s, len+1);
            size += len+1;
            s = (const char*)va_arg(args, char*);
            if (!s) s = "";
        }
        return row;
    }

    void PrintRow(FILE *f, MyRow *row)
    {
        vfprintf(f, m_fmt, (va_list)row->ptrs);
    }
};

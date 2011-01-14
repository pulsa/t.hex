#pragma once
#ifndef _REFCOUNT_H_
#define _REFCOUNT_H_

class thRefCount
{
public:
    thRefCount() {
        m_nRefs = 1;
    }

    inline int AddRef() {
        m_nRefs++; return m_nRefs;
    }

    inline int Release() {
        wxASSERT(m_nRefs > 0);
        int tmp = --m_nRefs; if (tmp == 0) delete this; return tmp;
    }

protected:
    int m_nRefs;
};

#endif // _REFCOUNT_H_

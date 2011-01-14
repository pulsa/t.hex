//*****************************************************************************
//*****************************************************************************
// HexDocCache
//*****************************************************************************
//*****************************************************************************

class HexDocCache : public thRefCount
{
public:
	HexDocCache(HexDoc *doc) : m_doc(doc)
	{
		m_start = m_size = m_end = m_bufferSize = 0;
		m_pBuffer = NULL;
		m_pData = NULL;
	}

	bool Cache(THSIZE nIndex, THSIZE nSize);

    inline bool HasByte(uint64 nIndex) const
	{
		return (m_start <= nIndex && m_end > nIndex);
	}

    inline bool HasRange(uint64 nIndex, uint64 nSize) const
    {
        return (m_start <= nIndex && m_end >= nIndex + nSize);
    }

	void Invalidate()
	{
		m_start = m_size = m_end = 0;
	}

	void Clear()
	{
		delete [] m_pBuffer;
		m_start = m_size = m_end = m_bufferSize = 0;
		m_pBuffer = NULL;
		m_pData = NULL;
	}

	HexDoc *GetDoc() const { return m_doc; }
	THSIZE GetStart() const { return m_start; }

	//uint8 operator[](size_t nIndex) const { return m_pData[nIndex]; }

protected:
	HexDoc *m_doc;
    THSIZE m_start, m_size, m_bufferSize;
	THSIZE m_end; // always m_start + m_size
    uint8* m_pBuffer; // scratch
    const uint8* m_pData;   // points to m_pCacheBuffer OR some other memory
};

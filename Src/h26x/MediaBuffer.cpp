//
//

#include "MediaBuffer.h"
#include <string.h>

namespace Zilu {
namespace StreamResolver {

#define JmixDeletePointer(p) \
    if (NULL != p) { \
        delete p; \
        p = NULL; \
    } \
    (void)0

#define JmixDeleteArray(p) \
    if (NULL != p) { \
        delete [] (p); \
        (p) = NULL; \
    } \
    (void)0

void CMediaBuffer::PushData(const uint8_t *pData, const int nDataSize)
{
    lock_guard<mutex> g(m_mutex);
    if (NULL == pData || nDataSize <= 0)
        return;

    if (LeftSpaceSize() < nDataSize)
        ReSizeBuffSize(m_nBufferMaxSize + nDataSize);

    memcpy(m_pData + m_nCurrentDataPos, pData, nDataSize);
    m_nCurrentDataPos += nDataSize;
}

void CMediaBuffer::GetData(uint8_t *pData, int nDataSize, int nStartIndex)
{
    {
        lock_guard<mutex> g(m_mutex);
        if (NULL == pData || nDataSize <= 0 || nStartIndex < 0)
            return;

        if (m_nCurrentDataPos < nStartIndex + nDataSize)
            return;

        memcpy(pData, m_pData + nStartIndex, nDataSize);
    }

    MoveDataToBuffHead(nStartIndex + nDataSize);
}

int CMediaBuffer::LeftSpaceSize()
{
    return m_nBufferMaxSize - m_nCurrentDataPos;
}

uint8_t *CMediaBuffer::Data() const
{
    return m_pData;
}

int CMediaBuffer::Size()
{
    return m_nCurrentDataPos;
}

CMediaBuffer::CMediaBuffer()
{
    enum
    {
        BUFFER_DEFAULT_SIZE = 10 * 1024
    };

    m_pData = new uint8_t[BUFFER_DEFAULT_SIZE];
    m_nBufferMaxSize = BUFFER_DEFAULT_SIZE;
    m_nCurrentDataPos = 0;
}

CMediaBuffer::~CMediaBuffer()
{
    JmixDeleteArray(m_pData);
    m_nBufferMaxSize = 0;
    m_nCurrentDataPos = 0;
}

void CMediaBuffer::ReSizeBuffSize(int nExpectSize)
{
    if (nExpectSize <= m_nBufferMaxSize)
        return;

    uint8_t *pNewData = new uint8_t[nExpectSize];
    memset(pNewData, 0, nExpectSize);
    memcpy(pNewData, m_pData, m_nCurrentDataPos);

    JmixDeleteArray(m_pData);

    m_pData = pNewData;
    m_nBufferMaxSize = nExpectSize;
}

void CMediaBuffer::MoveDataToBuffHead(int nMoveSize)
{
    lock_guard<mutex> g(m_mutex);
    if (m_nCurrentDataPos == nMoveSize) {
        m_nCurrentDataPos = 0;
        return;
    }

    memmove(m_pData, m_pData + nMoveSize, m_nCurrentDataPos - nMoveSize);
    m_nCurrentDataPos -= nMoveSize;
}

}
}

//
//

#pragma once

#include <cstdint>
#include <mutex>
using namespace std;

namespace Zilu {
namespace StreamResolver {

class CMediaBuffer
{
public:

    void PushData(const uint8_t *pData, const int nDataSize);

    void GetData(uint8_t *pData, int nDataSize, int nStartIndex = 0);

    uint8_t *Data() const;

    int Size();

    void MoveDataToBuffHead(int nMoveSize);

public:
    CMediaBuffer();

    ~CMediaBuffer();

private:
    int LeftSpaceSize();

    void ReSizeBuffSize(int nExpectSize);

private:
    mutex       m_mutex;
    uint8_t *   m_pData;
    int         m_nBufferMaxSize;
    int         m_nCurrentDataPos;
};

}
}

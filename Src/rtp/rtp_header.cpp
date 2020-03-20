/*
**	********************************************************************************
**
**	File		: RtpHeader.cpp
**	Description	: 
**	Modify		: 2019/12/25		zhangqiang		Create the file
**	********************************************************************************
*/
#include "rtp_header.hpp"

namespace Zilu {
namespace StreamResolver {

int CRtpHeader::Decode(uint8_t *praw, uint32_t dsize)
{
    const uint8_t * p = praw;
    if (dsize < 12)
        return -1;

    V = uint8_t(p[0] & 0xc0) >> 6;
    if (2 != V)
        return -2;

    P = uint8_t(p[0] & 0x20) >> 5;
    X = uint8_t(p[0] & 0x10) >> 4;
    CC = uint8_t(p[0] & 0x0f);
    M = uint8_t(p[1] & 0x80) >> 7;
    PT = uint8_t(p[1] & 0x7f);
    seq = (uint16_t(p[2]) << 8) + uint16_t(p[3]);
    ts = (uint32_t(p[4]) << 24) + (uint32_t(p[5]) << 16) + (uint32_t(p[6]) << 8) + uint32_t(p[7]);
    SSRC = (uint32_t(p[8]) << 24) + (uint32_t(p[9]) << 16) + (uint32_t(p[10]) << 8) + uint32_t(p[11]);

    m_headsize = 12;
    if (CC > 0)
    {
        if (dsize < uint32_t (12 + uint32_t (CC ) * 4))
            return -3;

        int offset = 12;
        for (int i = 0; i < (int) CC; ++i)
        {
            CSRC[i] = (uint32_t(p[i + offset]) << 24) + (uint32_t(p[i + offset + 1]) << 16) +
                      (uint32_t(p[i + offset + 2]) << 8) + uint32_t(p[i + offset + 3]);
            offset += 4;
        }

        m_headsize += CC * 4;
    }

    if (1 == X)
    {
        if (dsize < (m_headsize + 4))
            return -4;

        extension_profile = (uint16_t(p[m_headsize]) << 8) + uint16_t(p[m_headsize + 1]);
        extension_len = (uint16_t(p[m_headsize + 2]) << 8) + uint16_t(p[m_headsize + 3]);
        m_headsize += 4;

        if (extension_len > 0)
        {
            if (dsize < (m_headsize + extension_len))
                return -5;

//            header_extension_data = std::shared_ptr<uint8_t>(new uint8_t[header_extension_length], std::default_delete<uint8_t[]>());
//            memcpy(header_extension_data.get(), &p[m_headsize], header_extension_length);
            m_headsize += extension_len;
        }
    }

    if (1 == P)
    {
        //存在填充标志位，最后一个字节指明可以忽略多少个填充比特
        padding_len = p[dsize - 1];
    }
    return 0;
}

}
}

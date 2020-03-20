/*
**	********************************************************************************
**
**	File		: Rtp.cpp
**	Description	: 
**	Modify		: 2019/12/22		zhangqiang		Create the file
**	********************************************************************************
*/
#include <list>
#include "rtp_demux.hpp"
#include <iostream>

#define TIMESTAMP_KIND 3

namespace Zilu {
namespace StreamResolver {

CRtpDemux::CRtpDemux()
{
    m_bLostPframe = false;
    m_nPreSequence = -1;
#ifdef DEBUG_
    m_nLastSequence = 0;
#endif
}

CRtpDemux::~CRtpDemux()
{
}

int CRtpDemux::DecodeRawData(uint8_t *pRaw, uint32_t nSize, RtpPayload &payload)
{
    int ret = 0;
    if ((ret = m_rtpHeader.Decode(pRaw, nSize)) != 0) {
        return ret;
    }

#ifdef DEBUG_
    if (m_rtpHeader.seq - m_nLastSequence != 1) {
        std::cout << "Rtp sequence lost, SSRC: " << m_rtpHeader.SSRC << ", current: " << m_rtpHeader.seq << ", Pre sequence: " << m_nLastSequence;
    }
    m_nLastSequence = m_rtpHeader.seq;
#endif
    if (RTP_PAYLOAD_TYPE_PS == m_rtpHeader.PT)
    {
        int padding = 0;
        nSize -= m_rtpHeader.m_headsize;
        pRaw += m_rtpHeader.m_headsize;
        if (1 == m_rtpHeader.P) {
            padding = pRaw[nSize - 1];
        }

        nSize -= padding;
    }
    else
    {
        //TODO: 处理其他类型payload
        std::cout << "Don't be ps mode! PT: "<< m_rtpHeader.PT;
        return -1;
    }

    payload.seq = m_rtpHeader.seq;
    payload.ts = m_rtpHeader.ts;
    payload.ptype = m_rtpHeader.PT;
    payload.ssrc = m_rtpHeader.SSRC;
    payload.p_len = nSize;
    payload.payload = std::shared_ptr<uint8_t>(new uint8_t[nSize], std::default_delete<uint8_t[]>());
    memcpy(payload.payload.get(), pRaw, nSize);

    return 0;
}

int CRtpDemux::AppendPayload(RtpPayload &payload)
{
    //是PS包的I帧
    m_lRtpPayload.push_back(payload);

    if (m_vTimestamp.size() == TIMESTAMP_KIND)
        return 0;

    auto index = 0;
    for (; index < m_vTimestamp.size(); ++index) {
        if (m_vTimestamp[index] == payload.ts)
            break;
    }
    //需要记录时间戳,时间戳不在列表中
    if (index == m_vTimestamp.size())
        m_vTimestamp.push_back(payload.ts);
    return 0;
}

int CRtpDemux::PreprocessRtpPayload()
{
    m_bLostPframe = false;
    m_lRtpPayload.sort(RtpPayload::CompareSequence);

    int32_t nPreSequence = m_lRtpPayload.back().seq;
    //判断两个比较单元时间戳是否连续
    if (m_nPreSequence != -1)
    {
        RtpPayload pl = m_lRtpPayload.front();

        int32_t dvalue = abs(m_nPreSequence - pl.seq);
        if (dvalue < 1000 && dvalue > 1)
        {
            m_bLostPframe = true;
            std::cout << "Two diff unit lost package, Cur sequence: " << m_lRtpPayload.front().seq << ", pre timestamp: " << m_nPreSequence;
            clean_by_timestamp(pl.ts);
        }
    }
    m_nPreSequence = nPreSequence;

    std::vector<uint32_t > loseTs;

    //同一时间戳情况
    uint16_t lastSeq = m_lRtpPayload.front().seq;
    auto itr = m_lRtpPayload.begin();
    itr++; //从第二个开始处理
    for (; itr != m_lRtpPayload.end(); ++itr)
    {
        int32_t dvalue = abs((int32_t)itr->seq - (int32_t)lastSeq);
        if ( dvalue > 1 && dvalue < 1000 ) //适应sequence溢出的情况
        {
            RtpPayload prePayload = *(--itr);
            itr++;
            if (itr->ts == prePayload.ts)
            {
                m_bLostPframe = true;
                loseTs.push_back(itr->ts);
                std::cout << "this: " << this << ", One timestamp: Cur sequence: " << itr->seq
                          << ", pre sequence: " << prePayload.seq << ", timestamp: " << itr->ts;
                break;
            }
        }
        lastSeq = itr->seq;
    }

    //丢弃此时间戳后面所有数据
    if (loseTs.size() == 1)
        for (auto & ts : m_vTimestamp)
            if (loseTs[0] < ts)
                loseTs.push_back(ts);

    //丢掉数据
    for (auto &ts : loseTs) {
        clean_by_timestamp(ts);
    }
    loseTs.clear();

    //不同时间戳 序列不连续，有两种情况
    // 1. 后面的时间戳的包是首包，则丢上一个时间戳
    // 2. 后面的时间戳的包不是首包，则两个时间戳都需要丢掉
    lastSeq = m_lRtpPayload.front().seq;
    itr = m_lRtpPayload.begin();
    itr++;
    for (; itr != m_lRtpPayload.end(); ++itr)
    {
        int32_t dvalue = abs((int32_t)itr->seq - (int32_t)lastSeq);
        if ( dvalue > 1 && dvalue < 1000 ) //适配sequence溢出的情况
        {
            RtpPayload prePayload = *(--itr);
            itr++;
            if (itr->ts != prePayload.ts)
            {
                m_bLostPframe = true;
                loseTs.push_back(prePayload.ts);
                loseTs.push_back(itr->ts);
                std::cout << "this: " << this << ", Second timestamp: Cur sequence: " << itr->seq << ", timestamp: " << itr->ts <<
                          ", pre sequence: " << prePayload.seq << ", timestamp: " << prePayload.ts;
                break;
            }
        }
        lastSeq = itr->seq;
    }
    //丢弃此时间戳后面所有数据
    //3个时间戳，若剩余的时间戳比其中任何都小，就需丢弃
    if (loseTs.size() == 2) {
        for (auto & ts : m_vTimestamp) {
            if (ts > loseTs[1]) {
                loseTs.push_back(ts);
            }
        }
    }

    //丢掉数据
    for (auto &ts : loseTs) {
        clean_by_timestamp(ts);
    }
    return 0;
}

int CRtpDemux::FetchFrame(RtpPayload &payload)
{
    if (m_lRtpPayload.size() == 0) {
        m_vTimestamp.clear();
        return -1;
    }

    payload = m_lRtpPayload.front();
    m_lRtpPayload.pop_front();

    return 0;
}

bool CRtpDemux::IsAppendData(RtpPayload &payload)
{
    if (m_vTimestamp.size() < TIMESTAMP_KIND)
        return true;

    /// size = TIEMESTAMP_KIND
    for (auto & ts : m_vTimestamp)
    {
        if (ts == payload.ts) {
            return true;
        }
    }
    return false;
}

int CRtpDemux::clean_by_timestamp(uint32_t timestamp)
{
    for (auto itr = m_lRtpPayload.begin(); itr != m_lRtpPayload.end();)
    {
        if (itr->ts == timestamp)
        {
            itr = m_lRtpPayload.erase(itr);
        }
        else
            ++itr;
    }
    return 0;
}

bool CRtpDemux::IsLostPframe()
{
    return m_bLostPframe;
}


}
}

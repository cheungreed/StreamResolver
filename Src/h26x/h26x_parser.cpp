/*
**	********************************************************************************
**
**	File		: h26x_Parser.cpp
**	Description	: 
**	Modify		: 2020/1/15		zhangqiang		Create the file
**	********************************************************************************
*/
#include <string.h>
#include <vector>
#include <memory>
#include <iostream>
#include "h26x_parser.hpp"
#include "H264ParseSPS.h"
#include "H265ParseSPS.hpp"

namespace Zilu {
namespace StreamResolver {

CH26xParser::CH26xParser()
{
}

CH26xParser::~CH26xParser()
{
}
#if 0
int CH26xParser::PushFrameData(const uint8_t *p_data, uint32_t i_size)
{
    if (p_data == nullptr)
        return -1;

    m_videoFrame.PushData(p_data, i_size);
    return 0;
}

int CH26xParser::Decode(std::vector<FrameData> &frame, int codecType)
{
    while (1)
    {
        int framesize = -1;
        GetOneNalu(framesize, nullptr, 0);
        if (framesize <= 0)
            break;

        int frametype = -1;
        if (codecType == VIDEO_CODEC_TYPE_H264)
            frametype = PeekFrameTypeH264(m_videoFrame.Data(), m_videoFrame.Size());
        else if (codecType == VIDEO_CODEC_TYPE_H265)
            frametype = PeekFrameTypeH265(m_videoFrame.Data(), m_videoFrame.Size());
        else {
            m_videoFrame.MoveDataToBuffHead(framesize);
            continue;
        }

        switch (frametype)
        {
            case H265_STD_TYPE_VPS:
            {
                pushAndMoveDataPos(m_cacheVPS, framesize);
                continue;
            }
            case H265_STD_TYPE_SPS:
            case H264_STD_TYPE_SPS:
            {
                pushAndMoveDataPos(m_cacheSPS, framesize);
                if (m_cacheSPS.Data() + 4 == nullptr || m_cacheSPS.Size() - 4 <= 0)
                    continue;
                SpsInfo sps;
                ParseSpsInfo(frametype, sps, nullptr, 0);
                m_frameinfo.heigth = sps.height;
                m_frameinfo.width = sps.width;
                m_frameinfo.fps = sps.fps;
            }
            case H265_STD_TYPE_PPS:
            case H264_STD_TYPE_PPS:
            {
                pushAndMoveDataPos(m_cachePPS, framesize);
                continue;
            }
            case H265_STD_TYPE_SEI:
            case H264_STD_TYPE_SEI:
            {
                pushAndMoveDataPos(m_cacheSEI, framesize);
                continue;
            }
            case H265_STD_TYPE_I:
            case H264_STD_TYPE_I:
            {
                framesize += m_cacheSPS.Size() > 0 ? m_cacheSPS.Size() : 0;
                framesize += m_cachePPS.Size() > 0 ? m_cachePPS.Size() : 0;
                framesize += m_cacheSEI.Size() > 0 ? m_cacheSEI.Size() : 0;
                framesize += m_cacheVPS.Size() > 0 ? m_cacheVPS.Size() : 0;

                int offset = 0;
                FrameData lframe;
                lframe.frame_type = MFT_H264_H265_I;
                lframe.frame_size = framesize;
                lframe.frame_data = std::shared_ptr<uint8_t>(new uint8_t[framesize], std::default_delete<uint8_t[]>());

                if (m_cacheVPS.Size() > 0) {
                    memcpy(lframe.frame_data.get() + offset, m_cacheVPS.Data(), m_cacheVPS.Size());
                    offset += m_cacheVPS.Size();
                }
                if (m_cacheSPS.Size() > 0) {
                    memcpy(lframe.frame_data.get() + offset, m_cacheSPS.Data(), m_cacheSPS.Size());
                    offset += m_cacheSPS.Size();
                }
                if (m_cachePPS.Size() > 0) {
                    memcpy(lframe.frame_data.get() + offset, m_cachePPS.Data(), m_cachePPS.Size());
                    offset += m_cachePPS.Size();
                }
                if (m_cacheSEI.Size() > 0) {
                    memcpy(lframe.frame_data.get() + offset, m_cacheSEI.Data(), m_cacheSEI.Size());
                    offset += m_cacheSEI.Size();
                }

                m_videoFrame.GetData(lframe.frame_data.get() + offset, framesize - offset);

                frame.push_back(lframe);

                std::cout << "zqiang: I frameType: " << frametype;
            }
            default:
            {
                FrameData lframe;
                lframe.frame_type = MFT_H264_H265_P;
                lframe.frame_size = framesize;
                lframe.frame_data = std::shared_ptr<uint8_t >(new uint8_t[framesize], std::default_delete<uint8_t >());
                m_videoFrame.GetData(lframe.frame_data.get(), framesize);
                frame.push_back(lframe);
            }
        }
    }

    return 0;
}

int CH26xParser::GetFrameInfo(VideoFrameInfo &finfo)
{
    finfo = m_frameinfo;
    return 0;
}

void CH26xParser::Clear()
{
    m_cachePPS.MoveDataToBuffHead(m_cachePPS.Size());
    m_cacheVPS.MoveDataToBuffHead(m_cacheVPS.Size());
    m_cacheSEI.MoveDataToBuffHead(m_cacheSEI.Size());
    m_cacheSPS.MoveDataToBuffHead(m_cacheSPS.Size());
}
#endif
int CH26xParser::GetOneNalu(int &endIndex, const uint8_t *p_data, uint32_t i_size)
{
    if (i_size < 10)
        return -1;
    uint8_t startCode1[] = {0x00, 0x00, 0x00, 0x01};
    uint8_t startCode2[] = {0x00, 0x00, 0x01};

    for (int i = 4; i < i_size - 4; ++i)
    {
        if (0 == memcmp(startCode1, p_data + i, sizeof(startCode1)) ||
            0 == memcmp(startCode2, p_data + i, sizeof(startCode2)))
        {
            endIndex = i;
            break;
        }
    }
    return 0;
}

int CH26xParser::PeekFrameTypeH264(const uint8_t *pFrame, uint32_t nFrameSize)
{
    if (nFrameSize < 4)
        return 0;

    uint8_t naluType = 0;

    uint8_t startCode1[] = {0x00, 0x00, 0x00, 0x01};
    uint8_t startCode2[] = {0x00, 0x00, 0x01};
    if (0 == memcmp(startCode1, pFrame, sizeof(startCode1)))
        naluType = pFrame[sizeof(startCode1)];
    else if (0 == memcmp(startCode2, pFrame, sizeof(startCode2)))
        naluType = pFrame[sizeof(startCode2)];

    int tmpNaluType = -1;
    tmpNaluType = naluType & 0x1f;

    return tmpNaluType;
}

int CH26xParser::PeekFrameTypeH265(const uint8_t *pFrame, uint32_t nFrameSize)
{
    if (nFrameSize < 4)
        return 0;

    uint8_t naluType = 0;

    uint8_t startCode1[] = {0x00, 0x00, 0x00, 0x01};
    uint8_t startCode2[] = {0x00, 0x00, 0x01};
    if (0 == memcmp(startCode1, pFrame, sizeof(startCode1)))
        naluType = pFrame[sizeof(startCode1)];
    else if (0 == memcmp(startCode2, pFrame, sizeof(startCode2)))
        naluType = pFrame[sizeof(startCode2)];

    int tmpNaluType = -1;
    tmpNaluType = (naluType & 0x7E) >> 1;

    return tmpNaluType;
}

int CH26xParser::PeekVideoCodecType(const uint8_t *p_data, uint32_t i_size)
{
    int frametype = 0;
    frametype = PeekFrameTypeH264(p_data, i_size);
    if (H264_STD_TYPE_I == frametype ||
        H264_STD_TYPE_SPS == frametype ||
        H264_STD_TYPE_PPS == frametype ||
        H264_STD_TYPE_SEI == frametype)
    {
        return 1;//VIDEO_CODEC_TYPE_H264;
    }

    frametype = PeekFrameTypeH265(p_data, i_size);
    if (H265_STD_TYPE_VPS == frametype ||
        H265_STD_TYPE_SPS == frametype ||
        H265_STD_TYPE_PPS == frametype ||
        H265_STD_TYPE_SEI == frametype ||
        H265_STD_TYPE_I == frametype)
    {
        return 2;//VIDEO_CODEC_TYPE_H265;
    }
    return -1;
}

void CH26xParser::ParseSpsInfo(int frameType, SpsInfo &sps, const uint8_t *p_data, uint32_t i_size)
{
    if (H264_STD_TYPE_SPS == frameType) {
        //Ω‚Œˆ ”∆µøÌ∏ﬂ
        H264SPSInfo h264;
        memset(&h264, 0x0, sizeof(H264SPSInfo));
        h264_parse_sps(p_data + 4, i_size - 4, &h264);

        std::cout << "H264 Sps Info, width: " << h264.width << ", height: " << h264.height << ", fps: " << h264.fps;
        sps.width = h264.width;
        sps.height = h264.height;
    }
    else if (H265_STD_TYPE_SPS == frameType) {
        H265SPSInfo h265;
        H265ParseSPS(p_data + 4, i_size - 4, h265);

        std::cout << "H265 Sps Info, width: " << h265.width << ", height: " << h265.height;
        sps.width = h265.width;
        sps.height = h265.height;
    }
}

}
}

/*
**	********************************************************************************
**
**	File		: ps2es.cpp
**	Description	:
**	Modify		: 2019/10/16		zhangqiang		Create the file
**	********************************************************************************
*/
#include "ps_demux.hpp"
#include "timestamps.hpp"
#include "pes.hpp"

namespace Zilu {
namespace StreamResolver {

int CPsDemux::Decode(uint8_t *ps,
                     int ps_size,
                     PsPackHeader &ps_pack_header,
                     PsSystemHeader &ps_system_header,
                     PsSystemMap & ps_sysmap,
                     std::vector<EsInfo> &es_array)
{
    int ret = 0;

    int offset = 0;

    while (offset < ps_size)
    {
        if (ps_stream_id_peek(&ps[offset], PS_STREAM_ID_PACK_HEADER))
        {
            if ((ret = decode_pack_header(ps, ps_size, offset, ps_pack_header)) != 0) {
                std::cout << "decode ps pack header failed, ret: " << ret;
                return ret;
            }
        }
        else if(ps_stream_id_peek(&ps[offset], PS_STREAM_ID_SYSTEM_HEADER))
        {
            if ((ret = decode_system_header(ps, ps_size, offset, ps_system_header)) != 0)
            {
                std::cout << "decode ps system header failed, ret: "<<ret;
                return ret;
            }
        }
        else if (ps_stream_id_peek(&ps[offset], PS_STREAM_ID_MAP))
        {
            if ((ret = decode_system_map(ps, ps_size, offset, ps_sysmap)) != 0) {
                std::cout << "decode ps pack header failed, ret: "<<ret;
                return ret;
            }
        } else {
            // pes
            EsInfo es;
            if ((ret = decode_pes(ps, ps_size, offset, &es)) != 0) {
                std::cout << "decode pes failed, ret: " << ret;
                return ret;
            }

            if (es.data_size > 0) {
                es_array.push_back(es);
            }
        }
    }

    return ret;
}

int
CPsDemux::decode_pack_header(uint8_t *ps, int ps_size, int &offset, PsPackHeader &pack_header)
{
    if (ps_size - offset < PS_STREAM_HEADER_SIZE_AT_LEAST)
        return -1;

    const uint8_t * p = &ps[offset];

    uint8_t stuffing_size = p[13] & 0x07;
    pack_header.head_size = PS_STREAM_HEADER_SIZE_AT_LEAST + stuffing_size;

    if (ps_size - offset < pack_header.head_size)
        return -2;

    if(pack_header.head_size >= 14 && (p[4] >> 6) == 0x01 )
    {
        pack_header.timestramp = FROM_SCALE(ExtractPackHeaderTimestamp(&p[4] ) );
        pack_header.mux_rate = ( p[10] << 14 ) | ( p[11] << 6 ) | ( p[12] >> 2);
    }
    else if(pack_header.head_size >= 12 && (p[4] >> 4) == 0x02 ) /* MPEG-1 Pack SCR, same bits as PES/PTS */
    {
        int64_t i_scr;
        if(!ExtractPESTimestamp( &p[4], 0x02, &i_scr ))
            return -3;
        pack_header.timestramp = FROM_SCALE(i_scr );
        pack_header.mux_rate = ( (p[9] & 0x7f ) << 15 ) | ( p[10] << 7 ) | ( p[11] >> 1);
    }
    else {
        return -4;
    }

    offset += pack_header.head_size;

    return 0;
}

int CPsDemux::decode_system_header(uint8_t *ps, int ps_size, int &offset, PsSystemHeader &sysheader)
{
    if (ps_size - offset < 18)
        return -1;

    const uint8_t* p = &ps[offset];

    sysheader.head_size = ps_pkt_size(p, 14);

    offset += sysheader.head_size;
    return 0;
}

int CPsDemux::decode_system_map(uint8_t *ps, int ps_size, int &offset, PsSystemMap& es)
{
    int ret = 0;

    if (ps_size - offset < 9) {
        return -1;
    }

    if (offset > ps_size) {
        return -2;
    }

    const uint8_t * p = &ps[offset];

    uint16_t ps_map_size = ps_pkt_size(p, 14);

    uint8_t current_next_indicator = (p[6] >> 7) & 0x01;
    uint8_t single_extension_stream_flag = (p[6] >> 6) & 0x01;

    uint16_t ps_info_size = (p[8] << 8) | p[9]; // 0

    // TODO: parse descriptor
    int i = 10 + ps_info_size; // 10
    // program element stream
    uint16_t es_map_size = ps_map_size - ps_info_size - 10; //20

    int j = i + 2; //12
    es.es_cnt = 0;
    while (j < i + 2 + es_map_size && es.es_cnt < sizeof(es.pes) / sizeof(es.pes[0]))
    {
        es.pes[es.es_cnt].es_type = p[j];
        es.pes[es.es_cnt].es_id = p[j + 1];
        uint16_t es_info_size = (p[j + 2] << 8) | p[j + 3];

        ++es.es_cnt;
        j += 4 + es_info_size; //36
    }

    offset += ps_map_size;

    return ret;
}

int CPsDemux::decode_pes(uint8_t *ps, int ps_size, int &offset, EsInfo *es)
{
    if (ps_size - offset < 14)
        return -1;

    if (offset > ps_size)
        return -2;

    uint8_t * p = &ps[offset];

    uint16_t pes_pkt_size = ps_pkt_size(p, 14);

    uint8_t stream_id = ps_pkt_id(p, pes_pkt_size);

    ///修正skip值
    int i_skip = 0;
    ps_correct_skip(stream_id, i_skip);

    //初始化
    block_t p_pes;
    p_pes.p_buffer = p;
    p_pes.i_buffer = pes_pkt_size;

    int ret = ps_pkt_parse_pes(&p_pes, i_skip);
    if (0 != ret) {
        return ret;
    }

    es->es_streamid = stream_id;
    es->data_size = p_pes.i_buffer;
    es->p_data = p_pes.p_buffer;

    offset += pes_pkt_size;

    return 0;
}

bool CPsDemux::ps_stream_id_peek(const uint8_t *bits, int expected)
{
    if (NULL == bits) {
        return false;
    }

    if (0x00 == bits[0] && 0x00 == bits[1] && 0x01 == bits[2] && expected == bits[3])
        return true;

    return false;
}

int CPsDemux::ps_pkt_size(const uint8_t *p, int i_peek)
{
    if( unlikely(i_peek < 4) )
        return -1;

    switch( p[3] )
    {
        case PS_STREAM_ID_END_STREAM:
            return 4;

        case PS_STREAM_ID_PACK_HEADER:
            if( i_peek > 4 )
            {
                if( i_peek >= 14 && (p[4] >> 6) == 0x01 )
                    return 14 + (p[13]&0x07);
                else if( i_peek >= 12 && (p[4] >> 4) == 0x02 )
                    return 12;
            }
            break;

        case PS_STREAM_ID_SYSTEM_HEADER:
        case PS_STREAM_ID_MAP:
        case PS_STREAM_ID_DIRECTORY:
        default:
            if( i_peek >= 6 )
                return 6 + ((p[4]<<8) | p[5] );
    }
    return -1;
}

int CPsDemux::get_ps_unit_size(int &startIndex, int &psUnitSize, const uint8_t *pdata, uint32_t datasize)
{
    if (datasize < PS_STREAM_HEADER_SIZE_AT_LEAST)
        return 0;
    startIndex = -1;
    int nEndIndex = -1;

    for (int i = 0; i < datasize; ++i)
    {
        if (CPsDemux::ps_stream_id_peek(&pdata[i], PS_STREAM_ID_PACK_HEADER))
        {
            startIndex = i;
            break;
        }
    }

    if (startIndex < 0)
        return 0;
    uint8_t startCode[] = {0x00, 0x00, 0x01, 0xba};
    for (int i = startIndex + sizeof(startCode); i < datasize; ++i)
    {
        if (CPsDemux::ps_stream_id_peek(&pdata[i], PS_STREAM_ID_PACK_HEADER))
        {
            nEndIndex = i;
            break;
        }
    }

    if (nEndIndex < 0)
        return 0;
    psUnitSize = nEndIndex - startIndex;

    return 0;
}

}
}

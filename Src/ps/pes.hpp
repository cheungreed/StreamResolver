/*
**	********************************************************************************
**
**	File		: pes.hpp
**	Description	: 
**	Modify		: 2020/1/9		zhangqiang		Create the file
**	********************************************************************************
*/
#pragma once
#include "timestamps.hpp"
#include "ps_common.hpp"
#include <iostream>

namespace Zilu {
namespace StreamResolver {

static inline int64_t GetPESTimestamp( const uint8_t *p_data )
{
    return  ((int64_t)(p_data[ 0]&0x0e ) << 29)|
            (int64_t)(p_data[1] << 22)|
            ((int64_t)(p_data[2]&0xfe) << 14)|
            (int64_t)(p_data[3] << 7)|
            (int64_t)(p_data[4] >> 1);
}

static inline bool ExtractPESTimestamp( const uint8_t *p_data, uint8_t i_flags, int64_t *ret )
{
    /* !warn broken muxers set incorrect flags. see #17773 and #19140 */
    /* check marker bits, and i_flags = b 0010, 0011 or 0001 */
    if((p_data[0] & 0xC1) != 0x01 ||
       (p_data[2] & 0x01) != 0x01 ||
       (p_data[4] & 0x01) != 0x01 ||
       (p_data[0] & 0x30) == 0 || /* at least needs one bit */
       (p_data[0] >> 5) > i_flags ) /* needs flags 1x => 1x or flags 01 => 01 */
        return false;


    *ret =  GetPESTimestamp( p_data );
    return true;
}

/* PS SCR timestamp as defined in H222 2.5.3.2 */
static inline int64_t ExtractPackHeaderTimestamp( const uint8_t *p_data )
{
    return ((int64_t)(p_data[ 0]&0x38 ) << 27)|
           ((int64_t)(p_data[0]&0x03 ) << 28)|
           (int64_t)(p_data[1] << 20)|
           ((int64_t)(p_data[2]&0xf8 ) << 12)|
           ((int64_t)(p_data[2]&0x03 ) << 13)|
           (int64_t)(p_data[3] << 5) |
           (int64_t)(p_data[4] >> 3);
}

#if 1
inline
static int ParsePESHeader( const uint8_t *p_header, size_t i_header,
                           unsigned *pi_skip, int64_t *pi_dts, int64_t *pi_pts,
                           uint8_t *pi_stream_id, bool *pb_pes_scambling )
{
    unsigned i_skip;

    if ( i_header < 9 )
        return -1;

    *pi_stream_id = p_header[3];

    switch( p_header[3] )
    {
        case 0xBC:  /* Program stream map */
        case 0xBE:  /* Padding */
        case 0xBF:  /* Private stream 2 */
        case 0xF0:  /* ECM */
        case 0xF1:  /* EMM */
        case 0xFF:  /* Program stream directory */
        case 0xF2:  /* DSMCC stream */
        case 0xF8:  /* ITU-T H.222.1 type E stream */
            i_skip = 6;
            if( pb_pes_scambling )
                *pb_pes_scambling = false;
            break;
        default:
            if( ( p_header[6]&0xC0 ) == 0x80 )
            {
                /* mpeg2 PES */
                i_skip = p_header[8] + 9;

                if( pb_pes_scambling )
                    *pb_pes_scambling = p_header[6]&0x30;

                if( p_header[7]&0x80 )    /* has pts */
                {
                    if( i_header >= 9 + 5 )
                        (void) ExtractPESTimestamp( &p_header[9], p_header[7] >> 6, pi_pts );

                    if( ( p_header[7]&0x40 ) &&    /* has dts */
                        i_header >= 14 + 5 )
                        (void) ExtractPESTimestamp( &p_header[14], 0x01, pi_dts );
                }
            }
            else
            {
                /* FIXME?: WTH do we have undocumented MPEG1 packet stuff here ?
                   This code path should not be valid, but seems some ppl did
                   put MPEG1 packets into PS or TS.
                   Non spec reference for packet format on http://andrewduncan.net/mpeg/mpeg-1.html */
                i_skip = 6;

                if( pb_pes_scambling )
                    *pb_pes_scambling = false;

                while( i_skip < 23 && p_header[i_skip] == 0xff )
                {
                    i_skip++;
                    if( i_header < i_skip + 1 )
                        return -2;
                }
                if( i_skip == 23 )
                {
                    return -3;
                }
                /* Skip STD buffer size */
                if( ( p_header[i_skip] & 0xC0 ) == 0x40 )
                {
                    i_skip += 2;
                }

                if( i_header < i_skip + 1 )
                    return -4;

                if(  p_header[i_skip]&0x20 )
                {
                    if( i_header >= i_skip + 5 )
                        (void) ExtractPESTimestamp( &p_header[i_skip], p_header[i_skip] >> 4, pi_pts );

                    if( ( p_header[i_skip]&0x10 ) &&     /* has dts */
                        i_header >= i_skip + 10 )
                    {
                        (void) ExtractPESTimestamp( &p_header[i_skip+5], 0x01, pi_dts );
                        i_skip += 10;
                    }
                    else
                    {
                        i_skip += 5;
                    }
                }
                else
                {
                    if( (p_header[i_skip] & 0xFF) != 0x0F ) /* No pts/dts, lowest bits set to 0x0F */
                        return -5;
                    i_skip += 1;
                }
            }
            break;
    }

    *pi_skip = i_skip;
    return 0;
}
#endif

/* return the id of a PES (should be valid) */
static inline int ps_pkt_id( const uint8_t *p_pkt, size_t i_pkt )
{
    if(unlikely(i_pkt < 4))
        return 0;
    if( p_pkt[3] == 0xbd )
    {
        uint8_t i_sub_id = 0;
        if( i_pkt >= 9 &&
            i_pkt > 9 + (size_t)p_pkt[8] )
        {
            const unsigned i_start = 9 + p_pkt[8];
            i_sub_id = p_pkt[i_start];

            if( (i_sub_id & 0xfe) == 0xa0 &&
                i_pkt >= i_start + 7 &&
                ( p_pkt[i_start + 5] >=  0xc0 ||
                  p_pkt[i_start + 6] != 0x80 ) )
            {
                /* AOB LPCM/MLP extension
                * XXX for MLP I think that the !=0x80 test is not good and
                * will fail for some valid files */
                return 0xa000 | (i_sub_id & 0x01);
            }
        }

        /* VOB extension */
        return 0xbd00 | i_sub_id;
    }
    else if( i_pkt >= 9 &&
             p_pkt[3] == 0xfd &&
             (p_pkt[6]&0xC0) == 0x80 &&   /* mpeg2 */
             (p_pkt[7]&0x01) == 0x01 )    /* extension_flag */
    {
        /* ISO 13818 amendment 2 and SMPTE RP 227 */
        const uint8_t i_flags = p_pkt[7];
        unsigned int i_skip = 9;

        /* Find PES extension */
        if( (i_flags & 0x80 ) )
        {
            i_skip += 5;        /* pts */
            if( (i_flags & 0x40) )
                i_skip += 5;    /* dts */
        }
        if( (i_flags & 0x20 ) )
            i_skip += 6;
        if( (i_flags & 0x10 ) )
            i_skip += 3;
        if( (i_flags & 0x08 ) )
            i_skip += 1;
        if( (i_flags & 0x04 ) )
            i_skip += 1;
        if( (i_flags & 0x02 ) )
            i_skip += 2;

        if( i_skip < i_pkt && (p_pkt[i_skip]&0x01) )
        {
            const uint8_t i_flags2 = p_pkt[i_skip];

            /* Find PES extension 2 */
            i_skip += 1;
            if( i_flags2 & 0x80 )
                i_skip += 16;
            if( (i_flags2 & 0x40) && i_skip < i_pkt )
                i_skip += 1 + p_pkt[i_skip];
            if( i_flags2 & 0x20 )
                i_skip += 2;
            if( i_flags2 & 0x10 )
                i_skip += 2;

            if( i_skip + 1 < i_pkt )
            {
                const int i_extension_field_length = p_pkt[i_skip]&0x7f;
                if( i_extension_field_length >=1 )
                {
                    int i_stream_id_extension_flag = (p_pkt[i_skip+1] >> 7)&0x1;
                    if( i_stream_id_extension_flag == 0 )
                        return 0xfd00 | (p_pkt[i_skip+1]&0x7f);
                }
            }
        }
    }
    return p_pkt[3];
}

/* From id fill i_skip and es_format_t */
int ps_correct_skip(int i_id, int& i_skip)
{
    i_skip = 0;

    if( ( i_id&0xff00 ) == 0xbd00 ) /* 0xBD00 -> 0xBDFF, Private Stream 1 */
    {
        if( ( i_id&0xf8 ) == 0x88 || /* 0x88 -> 0x8f - Can be DTS-HD primary audio in evob */
            ( i_id&0xf8 ) == 0x98 )  /* 0x98 -> 0x9f - Can be DTS-HD secondary audio in evob */
        {
            i_skip = 4;
        }
        else if( ( i_id&0xf8 ) == 0x80 || /* 0x80 -> 0x87 */
                 ( i_id&0xf0 ) == 0xc0 )  /* 0xc0 -> 0xcf AC-3, Can also be DD+/E-AC3 in evob */
        {
            i_skip = 4;
        }
        else if( ( i_id&0xe0 ) == 0x20 ) /* 0x20 -> 0x3f */
        {
            i_skip = 1;
        }
        else if( ( i_id&0xf0 ) == 0xa0 ) /* 0xa0 -> 0xaf */
        {
            i_skip = 1;
        }
        else if( ( i_id&0xf0 ) == 0xb0 ) /* 0xb0 -> 0xbf */
        {
            i_skip = 5;
        }
    }
    else if( (i_id&0xff00) == 0xa000 ) /* 0xA000 -> 0xA0FF */
    {
        uint8_t i_sub_id = i_id & 0x07;
        if( i_sub_id == 0 )
        {
            i_skip = 1;
        }
        else if( i_sub_id == 1 )
        {
            i_skip = -1; /* It's a hack for variable skip value */
        }
    }
    return 0;
}

struct block_t
{
    uint8_t    *p_buffer; /**< Payload start */
    int         i_buffer; /**< Payload length */

    uint32_t    i_flags;

    int64_t     i_pts;
    int64_t     i_dts;
};

/* Parse a PES (and skip i_skip_extra in the payload) */
static inline int ps_pkt_parse_pes( block_t *p_pes, int i_skip_extra )
{
    unsigned int i_skip  = 0;
    int64_t i_pts = -1;
    int64_t i_dts = -1;
    uint8_t i_stream_id = 0;
    bool b_pes_scrambling = false;

    int ret = ParsePESHeader( p_pes->p_buffer, p_pes->i_buffer,
                              &i_skip, &i_dts, &i_pts, &i_stream_id, &b_pes_scrambling );
    if( ret != 0 ) {
        std::cout << "Parse pes header failed, ret: " << ret;
        return -3;
    }

    if( b_pes_scrambling )
        p_pes->i_flags |= BLOCK_FLAG_SCRAMBLED;

    if( i_skip_extra >= 0 )
        i_skip += i_skip_extra;
    else if( p_pes->i_buffer > i_skip + 3 &&
             ( ps_pkt_id( p_pes->p_buffer, p_pes->i_buffer ) == 0xa001 ||
               ps_pkt_id( p_pes->p_buffer, p_pes->i_buffer ) == 0xbda1 ) )
        i_skip += 4 + p_pes->p_buffer[i_skip+3];

    if( p_pes->i_buffer <= i_skip )
    {
        return -4;
    }

    p_pes->p_buffer += i_skip;
    p_pes->i_buffer -= i_skip;

    /* ISO/IEC 13818-1 2.7.5: if no pts and no dts, then dts == pts */
    if( i_pts >= 0 && i_dts < 0 )
        i_dts = i_pts;

    if( i_dts >= 0 )
        p_pes->i_dts = FROM_SCALE( i_dts );
    if( i_pts >= 0 )
        p_pes->i_pts = FROM_SCALE( i_pts );

    return 0;
}

}
}

/*
**	********************************************************************************
**
**	File		: H265ParseSPS.cpp
**	Description	: 
**	Modify		: 2019/11/15		zhangqiang		Create the file
**	********************************************************************************
*/
#include "H265ParseSPS.hpp"

class NALBitstream
{
public:
    NALBitstream() : m_data(nullptr), m_len(0), m_idx(0), m_bits(0), m_byte(0), m_zeros(0)
    {

    };
    NALBitstream(void * data, int len)
    {
        Init(data, len);
    };
    void Init(void * data, int len)
    {
        m_data = (uint8_t*)data;
        m_len = len;
        m_idx = 0;
        m_bits = 0;
        m_byte = 0;
        m_zeros = 0;
    };
    uint8_t GetBYTE()
    {
        if ( m_idx >= m_len )
            return 0;
        uint8_t b = m_data[m_idx++];
        if ( b == 0 )
        {
            m_zeros++;
            if ( (m_idx < m_len) && (m_zeros == 2) && (m_data[m_idx] == 0x03) )
            {
                m_idx++;
                m_zeros=0;
            }
        }
        else  m_zeros = 0;

        return b;
    };

    uint32_t GetBit()
    {
        if (m_bits == 0)
        {
            m_byte = GetBYTE();
            m_bits = 8;
        }
        m_bits--;
        return (m_byte >> m_bits) & 0x1;
    };

    uint32_t GetWord(int bits)
    {
        uint32_t u = 0;
        while ( bits > 0 )
        {
            u <<= 1;
            u |= GetBit();
            bits--;
        }
        return u;
    };

    uint32_t GetUE()
    {
        int zeros = 0;
        while (m_idx < m_len && GetBit() == 0 ) zeros++;
        return GetWord(zeros) + ((1 << zeros) - 1);
    };

    uint32_t GetSE()
    {
        uint32_t UE = GetUE();
        bool positive = UE & 1;
        uint32_t SE = (UE + 1) >> 1;
        if ( !positive )
        {
            SE = -SE;
        }
        return SE;
    };
private:
    uint8_t* m_data;
    int m_len;
    int m_idx;
    int m_bits;
    uint8_t m_byte;
    int m_zeros;
};

bool H265ParseSPS(const uint8_t *data, int size, H265SPSInfo& params)
{
    if (size < 20)
    {
        return false;
    }
    NALBitstream bs(const_cast<uint8_t *>(data), size);
    // seq_parameter_set_rbsp()
    bs.GetWord(4);// sps_video_parameter_set_id
    int sps_max_sub_layers_minus1 = bs.GetWord(3);
    if (sps_max_sub_layers_minus1 > 6)
    {
        return false;
    }
    bs.GetWord(1);
    {
        bs.GetWord(2);
        bs.GetWord(1);
        params.profile = bs.GetWord(5);
        bs.GetWord(32);//
        bs.GetWord(1);//
        bs.GetWord(1);//
        bs.GetWord(1);//
        bs.GetWord(1);//
        bs.GetWord(44);//
        params.level   = bs.GetWord(8);// general_level_idc
        uint8_t sub_layer_profile_present_flag[6] = {0};
        uint8_t sub_layer_level_present_flag[6]   = {0};
        for (int i = 0; i < sps_max_sub_layers_minus1; i++) {
            sub_layer_profile_present_flag[i]= bs.GetWord(1);
            sub_layer_level_present_flag[i]= bs.GetWord(1);
        }
        if (sps_max_sub_layers_minus1 > 0)
        {
            for (int i = sps_max_sub_layers_minus1; i < 8; i++) {
                uint8_t reserved_zero_2bits = bs.GetWord(2);
            }
        }
        for (int i = 0; i < sps_max_sub_layers_minus1; i++)
        {
            if (sub_layer_profile_present_flag[i]) {
                bs.GetWord(2);
                bs.GetWord(1);
                bs.GetWord(5);
                bs.GetWord(32);
                bs.GetWord(1);
                bs.GetWord(1);
                bs.GetWord(1);
                bs.GetWord(1);
                bs.GetWord(44);
            }
            if (sub_layer_level_present_flag[i]) {
                bs.GetWord(8);// sub_layer_level_idc[i]
            }
        }
    }
    uint32_t sps_seq_parameter_set_id= bs.GetUE();
    if (sps_seq_parameter_set_id > 15) {
        return false;
    }
    uint32_t chroma_format_idc = bs.GetUE();
    if (sps_seq_parameter_set_id > 3) {
        return false;
    }
    if (chroma_format_idc == 3) {
        bs.GetWord(1);//
    }
    params.width  = bs.GetUE(); // pic_width_in_luma_samples
    params.height  = bs.GetUE(); // pic_height_in_luma_samples
    if (bs.GetWord(1)) {
        bs.GetUE();
        bs.GetUE();
        bs.GetUE();
        bs.GetUE();
    }
    uint32_t bit_depth_luma_minus8= bs.GetUE();
    uint32_t bit_depth_chroma_minus8= bs.GetUE();
    if (bit_depth_luma_minus8 != bit_depth_chroma_minus8) {
        return false;
    }
    //...
    return true;
}
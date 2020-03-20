/*
**	********************************************************************************
**
**	File		: H265ParseSPS.hpp
**	Description	: 
**	Modify		: 2019/11/15		zhangqiang		Create the file
**	********************************************************************************
*/
#pragma once

#include <cstdint>
#include <cstring>

struct H265SPSInfo
{
    uint32_t width,height;
    uint32_t profile, level;
    uint32_t nal_length_size;
    H265SPSInfo() {
        width = 0;
        height = 0;
        profile = 0;
        level = 0;
        nal_length_size = 0;
    }
};

bool  H265ParseSPS(const uint8_t *data, int size, H265SPSInfo& params);

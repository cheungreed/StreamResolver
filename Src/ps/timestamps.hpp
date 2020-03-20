/*
**	********************************************************************************
**
**	File		: timestamps.hpp
**	Description	: 
**	Modify		: 2020/1/9		zhangqiang		Create the file
**	********************************************************************************
*/
#pragma once

namespace Zilu {
namespace StreamResolver {

#define FROM_SCALE_NZ(x) ((int64_t)((x) * 100 / 9))
#define TO_SCALE_NZ(x)   ((x) * 9 / 100)

#define FROM_SCALE(x) (INT64_C(1) + FROM_SCALE_NZ(x))
#define TO_SCALE(x)   TO_SCALE_NZ((x) - INT64_C(1))

static inline int64_t TimeStampWrapAround( int64_t i_first_pcr, int64_t i_time )
{
    int64_t i_adjust = 0;
    if( i_first_pcr > 0x0FFFFFFFF && i_time < 0x0FFFFFFFF )
        i_adjust = 0x1FFFFFFFF;

    return i_time + i_adjust;
}

}
}


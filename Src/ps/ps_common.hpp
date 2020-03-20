/*
**	********************************************************************************
**
**	File		: ps_common.hpp
**	Description	: 
**	Modify		: 2020/1/10		zhangqiang		Create the file
**	********************************************************************************
*/
#pragma once

namespace Zilu {
namespace StreamResolver {

# define unlikely(p)   __builtin_expect(!!(p), 0)

/** This block is scrambled */
#define BLOCK_FLAG_SCRAMBLED     0x0100

}
}

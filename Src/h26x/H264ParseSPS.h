//
//  H264ParseSPS.h
//
//  Created by lzj<lizhijian_21@163.com> on 2018/7/6.
//  Copyright © 2018年 LZJ. All rights reserved.

#ifndef H264ParseSPS_h
#define H264ParseSPS_h
#ifdef __cplusplus
extern "C"
{
#endif
#include <stdio.h>
    
struct H264SPSInfo
{
    unsigned int profile_idc;
    unsigned int level_idc;
    
    unsigned int width;
    unsigned int height;
    unsigned int fps;       //SPS中可能不包含FPS信息
    H264SPSInfo() {
        width = 0;
        height = 0;
        fps = 25;
    }
};

    
/**
 解析SPS数据信息

 @param data SPS数据内容，需要Nal类型为0x7数据的开始(比如：67 42 00 28 ab 40 22 01 e3 cb cd c0 80 80 a9 02)
 @param dataSize SPS数据的长度
 @param info SPS解析之后的信息数据结构体
 @return success:1，fail:0
 
 */
int h264_parse_sps(const unsigned char *data, unsigned int dataSize, H264SPSInfo *info);

#ifdef __cplusplus
}
#endif
#endif /* H264ParseSPS_h */

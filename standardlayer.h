// Copyright 2023-2023 The jdh99 Authors. All rights reserved.
// 标准头部层处理
// Authors: jdh99 <jdh821@163.com>

#ifndef STANDARDLAYER_H
#define STANDARDLAYER_H

#include "tztype.h"
#include "utz.h"

// StandardLayerRxFunc 接收回调函数
typedef void (*StandardLayerRxFunc)(uint8_t* data, int dataLen, UtzStandardHeader* standardHeader, int pipe, 
    uint32_t ip, uint16_t port);

// StandardLayerLoad 模块载入.如果不调用本函数,模块会申请默认内存id
bool StandardLayerLoad(void);

// StandardLayerRegister 注册接收观察者
bool StandardLayerRegister(StandardLayerRxFunc callback);

// StandardLayerSend 基于标准头部发送.标准头部的长度可以不用定义,由本函数计算
bool StandardLayerSend(uint8_t* data, int dataLen, UtzStandardHeader* standardHeader, int pipe, uint32_t ip, 
    uint16_t port);

// StandardLayerSendCcp 基于标准头部发送CCP帧
// 标准头部的长度可以不用定义,由本函数计算.同时本函数会自动为正文增加CCP头部
bool StandardLayerSendCcp(uint8_t* data, int dataLen, UtzStandardHeader* standardHeader, int pipe, uint32_t ip, 
    uint16_t port);

#endif

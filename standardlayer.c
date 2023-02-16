// Copyright 2023-2023 The jdh99 Authors. All rights reserved.
// 标准头部层处理
// Authors: jdh99 <jdh821@163.com>

#include "standardlayer.h"

#include "vsocket.h"
#include "lagan.h"
#include "tzlist.h"
#include "tzmalloc.h"

#define TAG "standardlayer"
#define MALLOC_TOTAL 4096

#pragma pack(1)

typedef struct {
    StandardLayerRxFunc callback;
} tItem;

#pragma pack()

static intptr_t gList = 0;
static int gMid = -1;

static void dealVsocketRx(VSocketRxParam* rxParam);
static bool isObserverExist(StandardLayerRxFunc callback);
static TZListNode* createNode(void);

// StandardLayerLoad 模块载入.如果不调用本函数,模块会申请默认内存id
bool StandardLayerLoad(void) {
    gMid = TZMallocRegister(0, TAG, MALLOC_TOTAL);
    if (gMid == -1) {
        LE(TAG, "load failed!malloc failed");
        return false;
    }
    gList = TZListCreateList(gMid);
    if (gList == 0) {
        LE(TAG, "load failed!create list failed");
        return false;
    }

    if (VSocketRegisterObserver(dealVsocketRx) == false) {
        LE(TAG, "load failed!vsocket register observer failed");
        return false;
    }

    return true;
}

static void dealVsocketRx(VSocketRxParam* rxParam) {
    UtzStandardHeader standardHeader;
    int offset = UtzBytesToStandardHeader(rxParam->Bytes, rxParam->Size, &standardHeader);
    if (offset == 0) {
        return;
    }

    TZListNode* node = TZListGetHeader(gList);
    tItem* item = NULL;

    for (;;) {
        if (node == NULL) {
            break;
        }

        item = (tItem*)node->Data;
        if (item->callback) {
            item->callback(rxParam->Bytes + offset, rxParam->Size - offset, &standardHeader, rxParam->Pipe, 
                rxParam->IP, rxParam->Port);
        }

        node = node->Next;
    }
}

// StandardLayerRegister 注册接收观察者
bool StandardLayerRegister(StandardLayerRxFunc callback) {
    if (gMid < 0 || callback == NULL) {
        LE(TAG, "register observer failed!mid is wrong or callback is null");
        return false;
    }

    if (isObserverExist(callback)) {
        return true;
    }

    TZListNode* node = createNode();
    if (node == NULL) {
        LE(TAG, "register observer failed!create node is failed");
        return false;
    }
    tItem* item = (tItem*)node->Data;
    item->callback = callback;
    TZListAppend(gList, node);
    return true;
}

static bool isObserverExist(StandardLayerRxFunc callback) {
    TZListNode* node = TZListGetHeader(gList);
    tItem* item = NULL;
    for (;;) {
        if (node == NULL) {
            break;
        }
        item = (tItem*)node->Data;
        if (item->callback == callback) {
            return true;
        }
        node = node->Next;
    }
    return false;
}

static TZListNode* createNode(void) {
    TZListNode* node = TZListCreateNode(gList);
    if (node == NULL) {
        return NULL;
    }
    node->Data = TZMalloc(gMid, sizeof(tItem));
    if (node->Data == NULL) {
        TZFree(node);
        return NULL;
    }
    return node;
}

// StandardLayerSend 基于标准头部发送.标准头部的长度可以不用定义,由本函数计算
bool StandardLayerSend(uint8_t* data, int dataLen, UtzStandardHeader* standardHeader, int pipe, uint32_t ip, 
    uint16_t port) {
    if (dataLen > 0xFFFF) {
        LE(TAG, "standard layer send failed!data len is too long:%d", dataLen);
        return false;
    }

    int frameSize = sizeof(UtzStandardHeader) + dataLen;
    uint8_t* frame = (uint8_t*)TZMalloc(gMid, frameSize);
    if (frame == NULL) {
        LE(TAG, "standard layer send failed!malloc fail");
        return false;
    }

    standardHeader->PayloadLen = dataLen;
    int offset = UtzStandardHeaderToBytes(standardHeader, frame, frameSize);
    memcpy(frame + offset, data, dataLen);

    VSocketTxParam txParam;
    txParam.Pipe = pipe;
    txParam.Bytes = frame;
    txParam.Size = frameSize;
    txParam.IP = ip;
    txParam.Port = port;
    bool result = VSocketTx(&txParam);

    TZFree(frame);
    return result;
}

// StandardLayerSendCcp 基于标准头部发送CCP帧
// 标准头部的长度可以不用定义,由本函数计算.同时本函数会自动为正文增加CCP头部
bool StandardLayerSendCcp(uint8_t* data, int dataLen, UtzStandardHeader* standardHeader, int pipe, uint32_t ip, 
    uint16_t port) {
    if (dataLen > 0xFFFF) {
        LE(TAG, "standard layer send failed!data len is too long:%d", dataLen);
        return false;
    }

    int frameSize = sizeof(UtzStandardHeader) + dataLen + 2;
    uint8_t* frame = (uint8_t*)TZMalloc(gMid, frameSize);
    if (frame == NULL) {
        LE(TAG, "standard layer send failed!malloc fail");
        return false;
    }

    standardHeader->PayloadLen = dataLen + 2;
    int offset = UtzStandardHeaderToBytes(standardHeader, frame, frameSize);
    UtzBytesToCcpFrame(data, dataLen, true, frame + offset, dataLen + 2);

    VSocketTxParam txParam;
    txParam.Pipe = pipe;
    txParam.Bytes = frame;
    txParam.Size = frameSize;
    txParam.IP = ip;
    txParam.Port = port;
    bool result = VSocketTx(&txParam);

    TZFree(frame);
    return result;
}

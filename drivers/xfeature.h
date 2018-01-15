// ==============================================================
// File generated by Vivado(TM) HLS - High-Level Synthesis from C, C++ and SystemC
// Version: 2015.4
// Copyright (C) 2015 Xilinx Inc. All rights reserved.
// 
// ==============================================================

#ifndef XFEATURE_H
#define XFEATURE_H

#ifdef __cplusplus
extern "C" {
#endif

/***************************** Include Files *********************************/
#ifndef __linux__
#include "xil_types.h"
#include "xil_assert.h"
#include "xstatus.h"
#include "xil_io.h"
#else
#include <stdint.h>
#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stddef.h>
#endif
#include "xfeature_hw.h"

/**************************** Type Definitions ******************************/
#ifdef __linux__
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
#else
typedef struct {
    u16 DeviceId;
    u32 Axilites_BaseAddress;
    u32 Crtl_bus_BaseAddress;
} XFeature_Config;
#endif

typedef struct {
    u32 Axilites_BaseAddress;
    u32 Crtl_bus_BaseAddress;
    u32 IsReady;
} XFeature;

/***************** Macros (Inline Functions) Definitions *********************/
#ifndef __linux__
#define XFeature_WriteReg(BaseAddress, RegOffset, Data) \
    Xil_Out32((BaseAddress) + (RegOffset), (u32)(Data))
#define XFeature_ReadReg(BaseAddress, RegOffset) \
    Xil_In32((BaseAddress) + (RegOffset))
#else
#define XFeature_WriteReg(BaseAddress, RegOffset, Data) \
    *(volatile u32*)((BaseAddress) + (RegOffset)) = (u32)(Data)
#define XFeature_ReadReg(BaseAddress, RegOffset) \
    *(volatile u32*)((BaseAddress) + (RegOffset))

#define Xil_AssertVoid(expr)    assert(expr)
#define Xil_AssertNonvoid(expr) assert(expr)

#define XST_SUCCESS             0
#define XST_DEVICE_NOT_FOUND    2
#define XST_OPEN_DEVICE_FAILED  3
#define XIL_COMPONENT_IS_READY  1
#endif

/************************** Function Prototypes *****************************/
#ifndef __linux__
int XFeature_Initialize(XFeature *InstancePtr, u16 DeviceId);
XFeature_Config* XFeature_LookupConfig(u16 DeviceId);
int XFeature_CfgInitialize(XFeature *InstancePtr, XFeature_Config *ConfigPtr);
#else
int XFeature_Initialize(XFeature *InstancePtr, const char* InstanceName);
int XFeature_Release(XFeature *InstancePtr);
#endif

void XFeature_Start(XFeature *InstancePtr);
u32 XFeature_IsDone(XFeature *InstancePtr);
u32 XFeature_IsIdle(XFeature *InstancePtr);
u32 XFeature_IsReady(XFeature *InstancePtr);
void XFeature_EnableAutoRestart(XFeature *InstancePtr);
void XFeature_DisableAutoRestart(XFeature *InstancePtr);

void XFeature_Set_frame_in(XFeature *InstancePtr, u32 Data);
u32 XFeature_Get_frame_in(XFeature *InstancePtr);
void XFeature_Set_mask_in(XFeature *InstancePtr, u32 Data);
u32 XFeature_Get_mask_in(XFeature *InstancePtr);
void XFeature_Set_bounding(XFeature *InstancePtr, u32 Data);
u32 XFeature_Get_bounding(XFeature *InstancePtr);
void XFeature_Set_featureh(XFeature *InstancePtr, u32 Data);
u32 XFeature_Get_featureh(XFeature *InstancePtr);

void XFeature_InterruptGlobalEnable(XFeature *InstancePtr);
void XFeature_InterruptGlobalDisable(XFeature *InstancePtr);
void XFeature_InterruptEnable(XFeature *InstancePtr, u32 Mask);
void XFeature_InterruptDisable(XFeature *InstancePtr, u32 Mask);
void XFeature_InterruptClear(XFeature *InstancePtr, u32 Mask);
u32 XFeature_InterruptGetEnabled(XFeature *InstancePtr);
u32 XFeature_InterruptGetStatus(XFeature *InstancePtr);

#ifdef __cplusplus
}
#endif

#endif

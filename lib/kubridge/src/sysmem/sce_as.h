/*
 * PS Vita Address Space Header
 * Copyright (C) 2021, Princess of Sleeping
 */

#ifndef _PSP2_KERNEL_AS_H_
#define _PSP2_KERNEL_AS_H_

#include <psp2kern/types.h>
#include <psp2kern/kernel/sysmem.h>

typedef struct SceUIDAddressSpaceObject SceUIDAddressSpaceObject;

typedef struct SceKernelAddressSpaceMMUContext SceKernelAddressSpaceMMUContext;

typedef struct SceUIDTinyPartitionObject { // size is 0x38-bytes
	void *pUserdata;
	SceClass *pClass;
	const char *name;
	int cpu_intr;
	void *unk_0x10;
	void *base_vaddr;
	SceSize base_size;
	SceKernelAddressSpaceMMUContext *pMMUContext; // proc cpu ctx
	int unk_0x20; // some flag?
	SceSize remain_size;
	int unk_0x28; // -1
	int unk_0x2C;
	int unk_0x30; // -1
	SceUInt32 magic; // 0xD946F262
} SceUIDTinyPartitionObject;

typedef struct SceUIDPartitionObject { // size is 0x80-bytes
	SceUIDTinyPartitionObject tiny;
	int unk_0x38;
	int unk_0x3C;
	int unk_0x40;
	void *unk_0x44;
	int unk_0x48;
	int unk_0x4C; // ex:8
	int unk_0x50;
	SceUIDAddressSpaceObject *unk_0x54;
	int unk_0x58;
	int unk_0x5C;
	int unk_0x60; // some bit mask
	SceUID pid;
	SceUID this_object_guid;
	int unk_0x6C;
	void *unk_0x70;
	int unk_0x74;
	int unk_0x78;
	int unk_0x7C;
} SceUIDPartitionObject;

typedef struct SceUIDPhyMemPartObject { // size is 0xAC-bytes
	void *pUserdata;
	SceClass *pClass;
	int data_0x08; // for cpu function
	int data_0x0C;

	SceUID data_0x10;
	int data_0x14;
	int data_0x18;
	void *data_0x1C;

	void *data_0x20;
	int data_0x24;
	int data_0x28;
	void *data_0x2C;

	void *data_0x30;
	int data_0x34;
	void *data_0x38;
	int data_0x3C;

	void *data_0x40;
	int data_0x44;
	void *data_0x48;
	int data_0x4C;

	void *data_0x50;
	int data_0x54;
	void *data_0x58;
	int data_0x5C;

	void *data_0x60;
	int data_0x64;
	void *data_0x68;
	int data_0x6C;

	void *data_0x70;
	int data_0x74;
	void *data_0x78;
	void *data_0x7C;

	int data_0x80;
	int data_0x84;
	int data_0x88;
	char name[0x20];
} SceUIDPhyMemPartObject;

typedef struct SceKernelProcessTTBR { // size is 0x14-bytes
	SceUInt32 unk_0x00;
	SceUIntPtr *pTTBR0;
	SceUIntPtr *pTTBR1;
	SceSize ttbr0_mgmt_size;
	SceSize ttbr1_mgmt_size;
} SceKernelProcessTTBR;

typedef struct SceKernelPTV { // size is 0x40-bytes
	SceUInt32 unk_0x00;
	SceInt32  unk_0x04;
	SceUInt32 unk_0x08;
	SceUInt32 unk_0x0C; // some flags
	SceUInt32 unk_0x10; // maybe some number
	SceUInt32 targetPA;
	SceUInt32 *pSecondLevelDescription;
	SceUInt32 unk_0x1C;
	SceUInt32 secondLevelDescriptionPA;
	SceUInt32 unk_0x24;
	void *unk_0x28;
	SceUInt32 unk_0x2C;
	SceInt32  unk_0x30;
	SceInt32  unk_0x34;
	SceInt32  unk_0x38;
	SceUInt32 magic;
} SceKernelPTV;

typedef struct SceKernelPTVVector { // size is 0x4000-bytes
	SceUInt32 vector[0x1000];
} SceKernelPTVVector;

typedef struct SceKernelAddressSpaceMMUContext { // size is 0x28-bytes
	SceKernelProcessContext cpu_ctx;
	SceKernelProcessTTBR *pProcessTTBR;
	SceKernelPTVVector *unk_0x10;
	int unk_0x14;
	int unk_0x18;
	int unk_0x1C;
	int unk_0x20;
	int unk_0x24;
} SceKernelAddressSpaceMMUContext;

typedef struct SceUIDAddressSpaceObject { // size is 0x170-bytes
	void *pUserdata;
	SceClass *pClass;
	int unk_0x08;		// for cpu function
	int unk_0x0C;
	int flag;		// kernel:0x30000002, user:0x10000001
	SceUID pid;
	SceKernelAddressSpaceMMUContext *unk_0x18;
	SceUIDPartitionObject *pProcAS[0x20];
	SceUID unk_uid[0x20];	// AS Info uid?
	SceUIDPhyMemPartObject *pPhyParts[16];
	SceUID unk_0x15C; // for user process? it guid
	SceUID unk_0x160; // for user process? it guid
	int unk_0x164;
	uint32_t unk_0x168;	// kernel:0x511389B0
	SceUInt32 magic;		// 0x4D95AEEC
} SceUIDAddressSpaceObject;

int x = sizeof(SceUIDAddressSpaceObject);

typedef struct SceSysmemAddressSpaceInfo {
	uintptr_t base;
	SceSize total;
	SceSize free;
	SceSize unkC;
} SceSysmemAddressSpaceInfo;

#endif /* _PSP2_KERNEL_AS_H_ */

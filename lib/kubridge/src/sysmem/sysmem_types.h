/*
 * faps-coredump sysmem_types.h
 * Copyright (C) 2021, Princess of Sleeping
 */

#ifndef _FAPS_COREDUMP_SYSMEM_TYPES_H_
#define _FAPS_COREDUMP_SYSMEM_TYPES_H_

#include <psp2kern/types.h>
#include "sce_as.h"
#include "memblock.h"

typedef struct SceKernelObject { // size is at least 0x24
	void *pObject;			// 0x0
	SceClass pClass;		// 0x4
	uint32_t type;			// 0x8
	uint16_t lcount;		// 0xC
	uint16_t ucount;		// 0xE
	uint16_t ccount;		// 0x10
	uint32_t unk_12;		// 0x12 - 0x54c3
	uint16_t unk_16;		// 0x16
	uint32_t uid;			// 0x18
	char unk_1C[4];			// 0x1C
	const char *name;		// 0x20
	uint32_t indexRaw;		// 0x24
} SceKernelObject;

typedef struct SceKernelMemBlockInfoExDetails {
  SceKernelMemBlockType type;
  SceUID memblk_uid;
  const char *name;
  void *mappedBase;
  SceSize mappedSize;
  SceSize memblock_some_size_or_alignment;
  int extraLow;
  int extraHigh;
  int unk20;
  SceUID unk24; // ex:0x10045
  SceKernelObject *SceUIDPhyMemPartClass_obj;
} SceKernelMemBlockInfoExDetails;

typedef struct SceKernelMemBlockInfoEx { // size is 0xAC on FW 0.990, 0xB8 on FW 3.60
  SceSize size;
  SceKernelMemBlockInfoExDetails details;
  SceSize unk30; // paddr num
  SceSize unk34; // paddr size num?
  void *paddr_list[0x10];
  SceSize align_list[0x10];
} SceKernelMemBlockInfoEx;

int ksceKernelMemBlockGetInfoEx(SceUID memblk_uid, SceKernelMemBlockInfoEx *pInfo);

#endif /* _FAPS_COREDUMP_SYSMEM_TYPES_H_ */

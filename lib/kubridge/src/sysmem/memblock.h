#ifndef _SYSMEM_MEMBLOCK_H_
#define _SYSMEM_MEMBLOCK_H_

#include <psp2common/types.h>

#define SCE_KERNEL_PHY_PAGE_SIZE_EXTRACT(pPhyPage) (((pPhyPage->word0 & 0xC0000000) == 0xC0000000) ? pPhyPage->size : 1 << ((pPhyPage->word0 & 0x1F00000) >> 0x14))
#define SCE_KERNEL_PHY_PAGE_ADDR_EXTRACT(pPhyPage) ((pPhyPage->word0 & 0xFFFFF) << 0xC)

typedef struct SceKernelPhyPage
{
    /**
     * 0x000FFFFF - Physical page index
     * 0x01F00000 - Size in log2 format
     * 0x30000000 - Some sort of state
     * 0xC0000000 - Type of page (0x40000000 - Small Page, 0xC0000000 - Large Page)
     */
    SceUInt32 word0;
    SceUInt32 size; // Only used for large pages (size >= 1 MiB)
} SceKernelPhyPage;

typedef struct SceKernelMemBlockPage
{ // size is 0x20, original name was SceKernelMemBlockAddressTree
    struct SceKernelMemBlockPage *next;
    int unk_04;
    int flags; // 0x20000:with align?
    void *vaddr;
    SceSize size;
    SceKernelPhyPage *phyPage; // size is 0x20
    unsigned int paddr;
    unsigned int magic;
} SceKernelMemBlockPage;

typedef struct SceUIDMemBlockObject
{
    union
    {
        struct
        {
            struct SceUIDMemBlockObject *pNext;
            void *pClass;
        };
        int sce_rsvd[2];
    };
    struct SceUIDMemBlockObject *pPrev;
    SceUInt32 otherFlags;
    SceUInt32 memBlockCode;
    int flags;
    void *vaddr;
    SceSize size; // non-aligned
    uint32_t spinLock;
    union
    {
        struct
        {
            struct SceUIDMemBlockObject *pPrevFree;
            struct SceUIDMemBlockObject *pNextFree;
        };
        struct
        {
            struct SceUIDPartitionObject *pPartition;
            struct SceUIDPhyMemPartObject *pPhyPart;
        };
    };
    void *name;
    SceKernelMemBlockPage *pages;
    union
    {
        struct
        {
            uint16_t unk_34;
            uint16_t refCount;
        };
        void *unk_0x34;
    };
    void *unk_38; /* Pointer to info related to base blocks */
    SceUID this_obj_uid;
} SceUIDMemBlockObject;

typedef struct MemBlockTypeInfo
{
    uint32_t memBlockType;
    uint32_t memBlockCode;
    uint32_t flags;
    uint32_t maximumAlignment;
} MemBlockTypeInfo;

#endif
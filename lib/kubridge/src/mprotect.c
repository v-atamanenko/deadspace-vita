#include <psp2kern/kernel/cpu.h>
#include <psp2kern/kernel/excpmgr.h>
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/proc_event.h>
#include <psp2kern/kernel/processmgr.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/kernel/sysmem/memtype.h>
#include <psp2kern/kernel/sysroot.h>
#include <psp2kern/kernel/threadmgr.h>

#include <psp2/kernel/error.h>

#include <taihen.h>

#include <stdbool.h>

#include "internal.h"
#include "sysmem/sysmem_types.h"

#define MIN(a, b) (a > b ? b : a)

// #define PRINT_MEMBLOCK_PAGES
#ifdef PRINT_MEMBLOCK_PAGES
const char *ksceKernelGetNameForUid2(SceUID guid);
const char *GetMemBlockName(SceUIDMemBlockObject *pMemBlock)
{
    if ((pMemBlock->this_obj_uid != 0) && (pMemBlock->this_obj_uid != -1))
        return ksceKernelGetNameForUid2(pMemBlock->this_obj_uid);

    if (pMemBlock->otherFlags & 0x80)
        return pMemBlock->name;

    return "SceNull";
}

static void InspectMemBlockPages(SceUIDMemBlockObject *pMemBlock)
{
    SceKernelMemBlockPage *pPage = pMemBlock->pages;

    ksceDebugPrintf("\n");
    LOG("Inspecting MemBlock %s VAddr = %p Size = 0x%X", GetMemBlockName(pMemBlock), pMemBlock->vaddr, pMemBlock->size);

    int i = 0;
    while (pPage != NULL)
    {
        LOG("Page %d VAddr = %p PAddr = %p Size = 0x%X Flags = 0x%X", i, pPage->vaddr, pPage->paddr, pPage->size, pPage->flags);
        if (pPage->phyPage)
        {
            LOG("PhyPage: Paddr: 0x%08X Size: 0x%08X Type: %d", SCE_KERNEL_PHY_PAGE_ADDR_EXTRACT(pPage->phyPage), SCE_KERNEL_PHY_PAGE_SIZE_EXTRACT(pPage->phyPage), pPage->phyPage->word0 >> 30);
        }
        pPage = pPage->next;
        i++;
    }
}
#endif

SceUIDAddressSpaceObject *ksceKernelSysrootGetCurrentAddressSpaceCB();
int module_get_offset(SceUID pid, SceUID modid, int segidx, size_t offset, uintptr_t *addr);
int ksceKernelCpuLockSuspendIntrStoreFlag(void *addr);
void ksceKernelCpuUnlockResumeIntrStoreFlag(void *addr, unsigned int prev_state);

static int (*AddressSpaceFindMemBlockCBByAddr)(void *pAS, void *vbase, SceSize size, SceUIDMemBlockObject **ppMemBlock);
static int (*AllocVirPageObject)(SceUIDPartitionObject *pPartition, SceKernelMemBlockPage **pPage);
static int (*FreeVirPageObject)(SceUIDPartitionObject *pPartition, void *pFixedHeap, SceKernelMemBlockPage *pPage);
static int (*MapVirPageCore)(void *pCommand, SceUIDPartitionObject *pPartition, void *param_3, SceUIDMemBlockObject *pMemBlock, void *vBase, SceSize size, SceUInt32 pBase);
static int (*UnmapVirPageWithOpt)(SceUIDPartitionObject *pPartition, SceUIDMemBlockObject *pMemBlock, SceKernelMemBlockPage *pPage, SceUInt32 flags);
static int (*VirPageAllocPhyPage)(SceKernelMemBlockPage *pPage, SceUIDPartitionObject *pPartition, SceUIDPhyMemPartObject *pPhyMem, SceKernelMemBlockPage **ppPage);
static int (*PhyMemPartFreePhyPage)(SceUIDPhyMemPartObject *pPhyPart, SceKernelPhyPage *pPhyPage);

uintptr_t MemBlockTypeToL1PTE_DebugContext;
uintptr_t MemBlockTypeToL2PTESmallPage_DebugContext;
uintptr_t sceKernelMMUMapLargePages_cont;
uintptr_t sceKernelMMUMapLargePages_DebugContext;

extern void MemBlockTypeToL1PTE_UserRWX();
extern void MemBlockTypeToL2PTESmallPage_UserRWX();
extern void MemBlockTypeToL2PTELargePage_UserRWX();

static int SplitPage(SceUIDMemBlockObject *pMemBlock, SceKernelMemBlockPage *pPage, void *targetAddr, SceSize targetSize, int targetType, bool remap)
{
    SceKernelMemBlockPage *pNewPage;
    int ret = 0;

    if ((ret = AllocVirPageObject(pMemBlock->pPartition, &pNewPage)) < 0)
    {
        LOG("Error: Failed to allocate page object");
        return ret;
    }

    pNewPage->magic = pPage->magic;
    pNewPage->flags = (pPage->flags & ~0x3F) | targetType;

    if (pPage->vaddr < targetAddr)
        pNewPage->size = pPage->size - (targetAddr - pPage->vaddr); // Old page will inhabit the area below the target
    else
        pNewPage->size = ((pPage->vaddr + pPage->size) - (targetAddr + targetSize)); // New page will inhabit the area beyond the target region

    pNewPage->vaddr = pPage->vaddr + (pPage->size - pNewPage->size);
    pNewPage->paddr = pPage->paddr + (pPage->size - pNewPage->size);
    pNewPage->phyPage = NULL;

    if ((pPage->flags & 0x14) && (ret = UnmapVirPageWithOpt(pMemBlock->pPartition, pMemBlock, pPage, 0)) < 0)
    {
        LOG("Error: Failed to unmap split page");
        FreeVirPageObject(pMemBlock->pPartition, (void *)pMemBlock->pPartition->tiny.pMMUContext->unk_0x1C, pNewPage);
        return ret;
    }

    pPage->size -= pNewPage->size;

    if ((pPage->flags & 0x14) && remap && (ret = MapVirPageCore(NULL, pMemBlock->pPartition, &pMemBlock->pPartition->tiny.pMMUContext->pProcessTTBR, pMemBlock, pPage->vaddr, pPage->size, pPage->paddr)) < 0)
    {
        LOG("Error: Failed to remap split page");
        FreeVirPageObject(pMemBlock->pPartition, (void *)pMemBlock->pPartition->tiny.pMMUContext->unk_0x1C, pNewPage);
        return ret;
    }

    if ((targetType & 0x14) && (ret = MapVirPageCore(NULL, pMemBlock->pPartition, &pMemBlock->pPartition->tiny.pMMUContext->pProcessTTBR, pMemBlock, pNewPage->vaddr, pNewPage->size, pNewPage->paddr)) < 0)
    {
        LOG("Error: Failed to map new page");
        FreeVirPageObject(pMemBlock->pPartition, (void *)pMemBlock->pPartition->tiny.pMMUContext->unk_0x1C, pNewPage);
        return ret;
    }

    pNewPage->next = pPage->next;
    pPage->next = pNewPage;

    return 0;
}

#define MERGE(pPage0, pPage1, unmap)                                                                                        \
    do                                                                                                               \
    {                                                                                                                \
        if (unmap && UnmapVirPageWithOpt(pMemBlock->pPartition, pMemBlock, pPage1, 0) < 0)                                    \
            LOG("Error: Failed to unmap merge page");                                                                \
        pPage0->size += pPage1->size;                                                                                \
        pPage0->next = pPage1->next;                                                                                 \
        FreeVirPageObject(pMemBlock->pPartition, (void *)pMemBlock->pPartition->tiny.pMMUContext->unk_0x1C, pPage1); \
    } while (0)

static SceKernelMemBlockPage *TryMergePages(SceUIDMemBlockObject *pMemBlock, SceKernelMemBlockPage *pPage, SceKernelMemBlockPage *pPrevPage, SceKernelMemBlockPage **ppNextPage, void **targetAddr, SceSize *targetSize, bool isMapped)
{
    SceKernelMemBlockPage *pNextPage = *ppNextPage;

    while (pNextPage && !pNextPage->phyPage && (pNextPage->vaddr == *targetAddr) &&
           (pNextPage->size <= *targetSize) && ((pPage->paddr + pPage->size) == pNextPage->paddr))
    {
        *targetSize -= pNextPage->size;
        *targetAddr += pNextPage->size;
        MERGE(pPage, pNextPage, true);
        pNextPage = pPage->next;
    }

    if (pPrevPage && !pPage->phyPage &&
        ((pPrevPage->vaddr + pPrevPage->size) == pPage->vaddr) &&
        ((pPrevPage->paddr + pPrevPage->size) == pPage->paddr))
    {
        if (UnmapVirPageWithOpt(pMemBlock->pPartition, pMemBlock, pPrevPage, 0) < 0)
            LOG("Error: Failed to unmap merge page");
        MERGE(pPrevPage, pPage, isMapped);
        pPage = pPrevPage;
    }

    *ppNextPage = pNextPage;

    return pPage;
}
#undef MERGE

int MemBlockProtectPages(SceUIDMemBlockObject *pMemBlock, SceUInt32 prot, void **addr, SceSize *len)
{
    SceKernelMemBlockPage *pPage, *pNextPage, *pPrevPage = NULL;
    SceUInt32 oldMemBlockCode;
    int ret = 0, intrState;
    void *curAddr;
    SceSize curSize;
    bool isMapped = true;

    if ((pMemBlock->otherFlags & 0xF0000) != 0x40000)
    {
        LOG("Error: MemBlock paging type unsupported (0x%X)", pMemBlock->otherFlags & 0xF0000);
        return SCE_KERNEL_ERROR_SYSMEM_MEMBLOCK_ERROR;
    }

    intrState = ksceKernelCpuLockSuspendIntrStoreFlag(&pMemBlock->spinLock);

    curAddr = *addr;
    curSize = *len;

    pPage = pMemBlock->pages;

#ifdef PRINT_MEMBLOCK_PAGES
    InspectMemBlockPages(pMemBlock);
#endif

    while ((pPage != NULL) && (curSize != 0))
    {
        pNextPage = pPage->next;
        if ((pPage->vaddr < curAddr && ((pPage->vaddr + pPage->size) <= curAddr)) ||
            (pPage->vaddr >= (curAddr + curSize))) // Page is not within the target region
            goto nextPage;

        if ((pPage->flags & 0x14) == 0) // Unmapped pages
        {
            SceSize incrSize = pPage->size - (pPage->vaddr > curAddr ? ((uintptr_t)pPage->vaddr - (uintptr_t)curAddr) : ((uintptr_t)curAddr - (uintptr_t)pPage->vaddr));
            
            curSize -= MIN(incrSize, curSize);
            curAddr += MIN(incrSize, curSize);
            goto nextPage;
        }

        if (pPage->vaddr < curAddr)
        {
            if ((ret = SplitPage(pMemBlock, pPage, curAddr, curSize, 0x10, true)) < 0)
                break;
            pNextPage = pPage->next;
            goto nextPage;
        }

        if ((pPage->vaddr + pPage->size) > (curAddr + curSize))
        {
            if ((ret = SplitPage(pMemBlock, pPage, curAddr, curSize, 0x10, false)) < 0)
                break;
            isMapped = false;
            pNextPage = pPage->next;
        }

        if (isMapped && (ret = UnmapVirPageWithOpt(pMemBlock->pPartition, pMemBlock, pPage, 0)) < 0)
        {
            LOG("Error: Failed to unmap page");
            break;
        }

        curSize -= pPage->size;
        curAddr += pPage->size;
        if (curSize)
            pPage = TryMergePages(pMemBlock, pPage, pPrevPage, &pNextPage, &curAddr, &curSize, isMapped);

        oldMemBlockCode = pMemBlock->memBlockCode;
        pMemBlock->memBlockCode = (pMemBlock->memBlockCode & 0xFFFFFF0F) | prot;
        if ((ret = MapVirPageCore(NULL, pMemBlock->pPartition, &pMemBlock->pPartition->tiny.pMMUContext->pProcessTTBR, pMemBlock, pPage->vaddr, pPage->size, pPage->paddr)) < 0)
        {
            LOG("Error: Failed to remap page");
            pMemBlock->memBlockCode = oldMemBlockCode;
            break;
        }
        pMemBlock->memBlockCode = oldMemBlockCode;
        pPrevPage = pPage;
    nextPage:
        isMapped = true;
        pPage = pNextPage;
    }

#ifdef PRINT_MEMBLOCK_PAGES
    InspectMemBlockPages(pMemBlock);
#endif

    ksceKernelCpuUnlockResumeIntrStoreFlag(&pMemBlock->spinLock, intrState);

    *addr = curAddr;
    *len = curSize;
    return ret;
}

int MemBlockCommitPages(SceUIDMemBlockObject *pMemBlock, SceUInt32 prot, void **addr, SceSize *len)
{
    SceKernelMemBlockPage *pPage, *pNextPage, *pNewPage;
    SceUInt32 oldMemBlockCode;
    int ret = 0, intrState;
    void *curAddr;
    SceSize curSize;

    if ((pMemBlock->otherFlags & 0xF0000) != 0x40000)
    {
        LOG("Error: MemBlock paging type unsupported (0x%X)", pMemBlock->otherFlags & 0xF0000);
        return SCE_KERNEL_ERROR_SYSMEM_MEMBLOCK_ERROR;
    }

    intrState = ksceKernelCpuLockSuspendIntrStoreFlag(&pMemBlock->spinLock);

    curAddr = *addr;
    curSize = *len;

    pPage = pMemBlock->pages;

#ifdef PRINT_MEMBLOCK_PAGES
    InspectMemBlockPages(pMemBlock);
#endif

    while ((pPage != NULL) && (curSize != 0))
    {
        pNextPage = pPage->next;
        if ((pPage->vaddr < curAddr && ((pPage->vaddr + pPage->size) <= curAddr)) ||
            (pPage->vaddr >= (curAddr + curSize))) // Page is not within the target region
            goto nextPage;

        if ((pPage->flags & 0x3F) == 0x4 || (pPage->flags & 0x3F) == 0x10)
        {
            if (pPage->vaddr == curAddr)
            {
                curSize -= (curSize >= pPage->size ? pPage->size : curSize);
                curAddr += (curSize >= pPage->size ? pPage->size : curSize);
            }
            goto nextPage;
        }

        if (pPage->vaddr < curAddr)
        {
            if ((ret = SplitPage(pMemBlock, pPage, curAddr, curSize, 0x1, true)) < 0)
                break;
            pNextPage = pPage->next;
            goto nextPage;
        }

        if ((pPage->vaddr + pPage->size) > (curAddr + curSize))
        {
            if ((ret = SplitPage(pMemBlock, pPage, curAddr, curSize, 0x1, true)) < 0)
                break;
            pNextPage = pPage->next;
        }

        if ((pPage->size & (pPage->size - 1)) != 0) // Size is not a power of two.
        {
            if ((ret = SplitPage(pMemBlock, pPage, curAddr, (1 << (31 - __builtin_clz(pPage->size))), 0x1, true)) < 0)
                break;
            pNextPage = pPage->next;
        }

        curSize -= pPage->size;
        curAddr += pPage->size;

        pPage->flags = (pPage->flags & ~0x3F) | 0x4;

        if ((ret = VirPageAllocPhyPage(pPage, pMemBlock->pPartition, pMemBlock->pPhyPart, &pNewPage)) < 0)
        {
            LOG("Error: Failed to allocate PhyPages");
            break;
        }

        oldMemBlockCode = pMemBlock->memBlockCode;
        pMemBlock->memBlockCode = (pMemBlock->memBlockCode & 0xFFFFFF0F) | prot;
        if ((ret = MapVirPageCore(NULL, pMemBlock->pPartition, &pMemBlock->pPartition->tiny.pMMUContext->pProcessTTBR, pMemBlock, pPage->vaddr, pPage->size, pPage->paddr)) < 0)
        {
            LOG("Error: Failed to map page");
            pMemBlock->memBlockCode = oldMemBlockCode;
            break;
        }
        while (pNewPage != pPage)
        {
            if ((ret = MapVirPageCore(NULL, pMemBlock->pPartition, &pMemBlock->pPartition->tiny.pMMUContext->pProcessTTBR, pMemBlock, pNewPage->vaddr, pNewPage->size, pNewPage->paddr)) < 0)
            {
                LOG("Error: Failed to map page");
                pMemBlock->memBlockCode = oldMemBlockCode;
                break;
            }
            pNewPage = pNewPage->next;
        }
        pMemBlock->memBlockCode = oldMemBlockCode;
    nextPage:
        pPage = pNextPage;
    }

#ifdef PRINT_MEMBLOCK_PAGES
    InspectMemBlockPages(pMemBlock);
#endif

    ksceKernelCpuUnlockResumeIntrStoreFlag(&pMemBlock->spinLock, intrState);

    *addr = curAddr;
    *len = curSize;
    return ret;
}

static int FindMirrorPage(SceUIDMemBlockObject *pMemBlock, SceUInt32 offset, SceUInt32 *paddr, SceSize *size)
{
    SceKernelMemBlockPage *pPage = pMemBlock->pages;
    void *targetAddr = pMemBlock->vaddr + offset;
    SceSize targetSize = *size;
    SceUInt32 mirrorPaddr;
    SceSize mirrorSize;
    while (pPage)
    {
        if ((pPage->vaddr < targetAddr && ((pPage->vaddr + pPage->size) <= targetAddr)) ||
            (pPage->vaddr >= (targetAddr + targetSize))) // Page is not within the target region
            goto nextPage;

        if ((pPage->flags & 0x14) == 0)
        {
            LOG("Error: Mirror page not physically backed");
            return SCE_KERNEL_ERROR_VIRPAGE_ERROR;
        }

        mirrorPaddr = pPage->paddr;
        mirrorSize = pPage->size;

        if (pPage->vaddr < targetAddr)
        {
            mirrorPaddr += ((uintptr_t)targetAddr - (uintptr_t)pPage->vaddr);
            mirrorSize -= ((uintptr_t)targetAddr - (uintptr_t)pPage->vaddr);
        }
        if ((pPage->vaddr + pPage->size) > (targetAddr + targetSize))
        {
            mirrorSize -= ((pPage->vaddr + pPage->size) - (targetAddr + targetSize));
        }

        break;
    nextPage:
        pPage = pPage->next;
    }
    if (pPage == NULL)
        return SCE_KERNEL_ERROR_VIRPAGE_ERROR;

    *paddr = mirrorPaddr;
    *size = mirrorSize;

    return 0;
}

int MemBlockCommitPagesWithBase(SceUIDMemBlockObject *pMemBlock, SceUInt32 prot, void **addr, SceSize *len, SceUIDMemBlockObject *pBaseMemBlock, SceUInt32 *offset)
{
    SceKernelMemBlockPage *pPage, *pNextPage;
    SceUInt32 oldMemBlockCode;
    int ret = 0, intrState[2];
    void *curAddr;
    SceSize curSize;
    SceUInt32 curOffset;
    SceUInt32 mirrorPaddr, mirrorSize;

    intrState[0] = ksceKernelCpuLockSuspendIntrStoreFlag(&pMemBlock->spinLock);
    intrState[1] = ksceKernelCpuLockSuspendIntrStoreFlag(&pBaseMemBlock->spinLock);

    if ((pMemBlock->otherFlags & 0xF0000) != 0x40000)
    {
        LOG("Error: MemBlock paging type unsupported (0x%X)", pMemBlock->otherFlags & 0xF0000);
        return SCE_KERNEL_ERROR_SYSMEM_MEMBLOCK_ERROR;
    }
    if ((pBaseMemBlock->otherFlags & 0xF0000) != 0x40000)
    {
        LOG("Error: MemBlock paging type unsupported (0x%X)", pBaseMemBlock->otherFlags & 0xF0000);
        return SCE_KERNEL_ERROR_SYSMEM_MEMBLOCK_ERROR;
    }

    curAddr = *addr;
    curSize = *len;
    curOffset = *offset;

    pPage = pMemBlock->pages;

#ifdef PRINT_MEMBLOCK_PAGES
    InspectMemBlockPages(pMemBlock);
    InspectMemBlockPages(pBaseMemBlock);
#endif

    while ((pPage != NULL) && (curSize != 0))
    {
        pNextPage = pPage->next;
        if ((pPage->vaddr < curAddr && ((pPage->vaddr + pPage->size) <= curAddr)) ||
            (pPage->vaddr >= (curAddr + curSize))) // Page is not within the target region
            goto nextPage;

        if ((pPage->flags & 0x3F) == 0x4) // Unmapped pages
        {
            LOG("Warning: Pages are already commited at addr %p (0x%X bytes). Skipping...", curAddr, pPage->size);
            SceSize incrSize = pPage->size - (pPage->vaddr > curAddr ? ((uintptr_t)pPage->vaddr - (uintptr_t)curAddr) : ((uintptr_t)curAddr - (uintptr_t)pPage->vaddr));

            curSize -= MIN(incrSize, curSize);
            curAddr += MIN(incrSize, curSize);
            curOffset += MIN(incrSize, curSize);
            goto nextPage;
        }

        if (pPage->vaddr < curAddr)
        {
            if ((ret = SplitPage(pMemBlock, pPage, curAddr, curSize, 0x1, true)) < 0)
                break;
            pNextPage = pPage->next;
            goto nextPage;
        }

        if ((pPage->vaddr + pPage->size) > (curAddr + curSize))
        {
            if ((ret = SplitPage(pMemBlock, pPage, curAddr, curSize, 0x1, true)) < 0)
                break;
            pNextPage = pPage->next;
        }

        mirrorSize = pPage->size;
        if ((ret = FindMirrorPage(pBaseMemBlock, curOffset, &mirrorPaddr, &mirrorSize)) < 0)
            break;

        if (pPage->size > mirrorSize)
        {
            if ((ret = SplitPage(pMemBlock, pPage, curAddr, mirrorSize, 0x1, true)) < 0)
                break;
            pNextPage = pPage->next;
        }

        pPage->flags = (pPage->flags & ~0x3F) | 0x10;
        pPage->paddr = mirrorPaddr;

        curSize -= pPage->size;
        curAddr += pPage->size;
        curOffset += pPage->size;

        oldMemBlockCode = pMemBlock->memBlockCode;
        pMemBlock->memBlockCode = (pMemBlock->memBlockCode & 0xFFFFFF0F) | prot;
        if ((ret = MapVirPageCore(NULL, pMemBlock->pPartition, &pMemBlock->pPartition->tiny.pMMUContext->pProcessTTBR, pMemBlock, pPage->vaddr, pPage->size, pPage->paddr)) < 0)
        {
            LOG("Error: Failed to map page");
            pMemBlock->memBlockCode = oldMemBlockCode;
            break;
        }
        pMemBlock->memBlockCode = oldMemBlockCode;
    nextPage:
        pPage = pNextPage;
    }

#ifdef PRINT_MEMBLOCK_PAGES
    InspectMemBlockPages(pMemBlock);
#endif

    ksceKernelCpuUnlockResumeIntrStoreFlag(&pBaseMemBlock->spinLock, intrState[1]);
    ksceKernelCpuUnlockResumeIntrStoreFlag(&pMemBlock->spinLock, intrState[0]);

    *addr = curAddr;
    *len = curSize;
    *offset = curOffset;
    return ret;
}

int MemBlockDecommitPages(SceUIDMemBlockObject *pMemBlock, void **addr, SceSize *len)
{
    SceKernelMemBlockPage *pPage, *pNextPage = NULL, *pPrevPage = NULL;
    int ret = 0, intrState;
    void *curAddr;
    SceSize curSize;

    if ((pMemBlock->otherFlags & 0xF0000) != 0x40000)
    {
        LOG("Error: MemBlock paging type unsupported (0x%X)", pMemBlock->otherFlags & 0xF0000);
        return SCE_KERNEL_ERROR_SYSMEM_MEMBLOCK_ERROR;
    }

    intrState = ksceKernelCpuLockSuspendIntrStoreFlag(&pMemBlock->spinLock);

    curAddr = *addr;
    curSize = *len;

    pPage = pMemBlock->pages;

#ifdef PRINT_MEMBLOCK_PAGES
    InspectMemBlockPages(pMemBlock);
#endif

    while ((pPage != NULL) && (curSize != 0))
    {
        pNextPage = pPage->next;
        if ((pPage->vaddr < curAddr && ((pPage->vaddr + pPage->size) <= curAddr)) ||
            (pPage->vaddr >= (curAddr + curSize))) // Page is not within the target region
            goto nextPage;

        if ((pPage->flags & 0x14) == 0) // Unmapped pages
        {
            SceSize incrSize = pPage->size - (pPage->vaddr > curAddr ? ((uintptr_t)pPage->vaddr - (uintptr_t)curAddr) : ((uintptr_t)curAddr - (uintptr_t)pPage->vaddr));
            
            curSize -= MIN(incrSize, curSize);
            curAddr += MIN(incrSize, curSize);
            goto nextPage;
        }
        else if ((pPage->flags & 0x3F) == 0x4 && pPage->vaddr != curAddr && pPage->size > curSize) // Not possible to split phy pages currently.
        {
            LOG("Warning: Attempted to partially decommit a page. Ignoring...");
            SceSize incrSize = pPage->size - (pPage->vaddr > curAddr ? ((uintptr_t)pPage->vaddr - (uintptr_t)curAddr) : ((uintptr_t)curAddr - (uintptr_t)pPage->vaddr));

            curSize -= MIN(incrSize, curSize);
            curAddr += MIN(incrSize, curSize);
            goto nextPage;
        }

        if (pPage->vaddr < curAddr)
        {
            if ((ret = SplitPage(pMemBlock, pPage, curAddr, curSize, pPage->flags & 0x3F, true)) < 0)
                break;
            pNextPage = pPage->next;
            goto nextPage;
        }

        if ((pPage->vaddr + pPage->size) > (curAddr + curSize))
        {
            if ((ret = SplitPage(pMemBlock, pPage, curAddr, curSize, pPage->flags & 0x3F, true)) < 0)
                break;
            pNextPage = pPage->next;
        }

        curSize -= pPage->size;
        curAddr += pPage->size;

        if ((ret = UnmapVirPageWithOpt(pMemBlock->pPartition, pMemBlock, pPage, 0)) < 0)
        {
            LOG("Error: Failed to unmap page");
            break;
        }
        if (pPage->phyPage != NULL)
        {
            PhyMemPartFreePhyPage(pMemBlock->pPhyPart, pPage->phyPage);
            pPage->phyPage = NULL;
        }
        if ((pPrevPage != NULL) && ((pPage->flags & 0x3F) != 0x4) && ((pPrevPage->flags & 0x14) == 0))
        {
            pPrevPage->size += pPage->size;
            FreeVirPageObject(pMemBlock->pPartition, (void *)pMemBlock->pPartition->tiny.pMMUContext->unk_0x1C, pPage);
            pPrevPage->next = pNextPage;
        }
        else
            pPage->flags = (pPage->flags & ~0x3F) | 0x1;

    nextPage:
        pPrevPage = pPage;
        pPage = pNextPage;
    }

#ifdef PRINT_MEMBLOCK_PAGES
    InspectMemBlockPages(pMemBlock);
#endif

    ksceKernelCpuUnlockResumeIntrStoreFlag(&pMemBlock->spinLock, intrState);

    *addr = curAddr;
    *len = curSize;
    return ret;
}

int kuKernelMemProtect(void *addr, SceSize len, SceUInt32 prot)
{
    SceUIDAddressSpaceObject *pAS;
    SceUIDMemBlockObject *pMemBlock;
    int ret = 0;

    if (len == 0)
        return 0;

    if (((uintptr_t)addr < 0x40000000) || ((uintptr_t)addr & 0xFFF))
    {
        LOG("Error: addr is invalid (%p)", addr);
        return SCE_KERNEL_ERROR_INVALID_ARGUMENT;
    }

    if (prot & ~(KU_KERNEL_PROT_READ | KU_KERNEL_PROT_WRITE | KU_KERNEL_PROT_EXEC))
    {
        LOG("Error: Invalid bits set in prot");
        return SCE_KERNEL_ERROR_INVALID_ARGUMENT;
    }

    void *curAddr = addr;
    SceSize curSize = (len + 0xFFF) & ~0xFFF; // Align to page size

    pAS = ksceKernelSysrootGetCurrentAddressSpaceCB();

    while (curSize)
    {
        if ((ret = AddressSpaceFindMemBlockCBByAddr(pAS, curAddr, 0, &pMemBlock)) < 0)
        {
            LOG("Error: Failed to find memBlock at address %p", curAddr);
            return ret;
        }
        
        if ((ret = MemBlockProtectPages(pMemBlock, prot, &curAddr, &curSize)) < 0)
        {
            LOG("Error: Failed to set protection for pages");
            return ret;
        }
    }

    return ret;
}

SceUID kuKernelMemReserve(void **addr, SceSize size, SceKernelMemBlockType memBlockType)
{
    void *allocAddr;
    int ret = 0, intrState;
    SceUID memBlock, userMemBlock;
    SceUIDMemBlockObject *pMemBlock;
    SceKernelMemBlockPage *pPage;
    SceKernelAllocMemBlockKernelOpt opt;
    opt.size = sizeof(opt);
    opt.attr = 0x40000; // NOPHYPAGE

    if ((ret = ksceKernelMemcpyFromUser(&allocAddr, addr, sizeof(void *))) < 0)
    {
        LOG("Error: Failed to copy addr from user space");
        return ret;
    }

    if (allocAddr != NULL)
    {
        opt.attr |= 0x1; // HAS_VBASE
        opt.field_C = (uintptr_t)allocAddr;
    }

    memBlock = ksceKernelAllocMemBlock("mem_reserve", memBlockType, size, &opt);
    if (memBlock < 0)
    {
        LOG("Error: Failed to allocate memBlock (0x%08X)", memBlock);
        return memBlock;
    }

    if ((allocAddr == NULL) && ((ret = ksceKernelGetMemBlockBase(memBlock, &allocAddr)) < 0))
    {
        LOG("Error: Failed to get base of memBlock (0x%08X)", ret);
        goto FreeBlock;
    }

    if ((ret = ksceKernelMemcpyToUser(addr, &allocAddr, sizeof(void *))) < 0)
    {
        LOG("Error: Failed to copy allocAddr to user space");
        goto FreeBlock;
    }

    if ((ret = ksceGUIDReferObject(memBlock, (SceObjectBase **)&pMemBlock)) < 0)
    {
        LOG("Error: Failed to refer memBlock object (0x%08X)", ret);
        goto FreeBlock;
    }

    intrState = ksceKernelCpuLockSuspendIntrStoreFlag(&pMemBlock->spinLock);

    pPage = pMemBlock->pages;
    while (pPage != NULL)
    {
        pPage->flags = (pPage->flags & ~0x3F) | 0x1;
        if ((ret = UnmapVirPageWithOpt(pMemBlock->pPartition, pMemBlock, pPage, 1)) < 0)
        {
            LOG("Error: Failed to unmap page");
            goto FreeBlock;
        }
        pPage = pPage->next;
    }

    ksceKernelCpuUnlockResumeIntrStoreFlag(&pMemBlock->spinLock, intrState);

    ksceGUIDReleaseObject(memBlock);

    userMemBlock = kscePUIDOpenByGUID(ksceKernelGetProcessId(), memBlock);
    if (userMemBlock < 0)
    {
        LOG("Error: Failed to create PUID for memBlock (0x%08X)", userMemBlock);
    }

    return userMemBlock;

FreeBlock:
    ksceKernelFreeMemBlock(memBlock);
    return ret;
}

int kuKernelMemCommit(void *addr, SceSize len, SceUInt32 prot, KuKernelMemCommitOpt *pOpt)
{
    KuKernelMemCommitOpt opt;
    SceUIDAddressSpaceObject *pAS;
    SceUIDMemBlockObject *pMemBlock, *pBaseMemBlock;
    SceUID baseMemBlock = -1;
    int ret = 0;

    if (len == 0)
        return 0;

    if (((uintptr_t)addr < 0x40000000) || ((uintptr_t)addr & 0xFFF))
    {
        LOG("Error: addr is invalid");
        return SCE_KERNEL_ERROR_INVALID_ARGUMENT;
    }

    if (prot & ~(KU_KERNEL_PROT_READ | KU_KERNEL_PROT_WRITE | KU_KERNEL_PROT_EXEC))
    {
        LOG("Error: Invalid bits set in prot");
        return SCE_KERNEL_ERROR_INVALID_ARGUMENT;
    }
    
    if (pOpt != NULL)
    {
        if ((ret = ksceKernelMemcpyFromUser(&opt, pOpt, sizeof(opt.size))) < 0)
        {
            LOG("Error: Failed to copy pOpt->size");
            return ret;
        }

        if (opt.size != sizeof(opt))
        {
            LOG("Error: pOpt->size is invalid");
            return SCE_KERNEL_ERROR_INVALID_ARGUMENT_SIZE;
        }

        if ((ret = ksceKernelMemcpyFromUser(&opt, pOpt, opt.size)) < 0)
        {
            LOG("Error: Failed to copy pOpt");
            return ret;
        }
    }
    else
    {
        memset(&opt, 0, sizeof(opt));
        opt.size = sizeof(opt);
        opt.attr = 0;
    }

    if (opt.attr & KU_KERNEL_MEM_COMMIT_ATTR_HAS_BASE)
    {
        if ((baseMemBlock = kscePUIDtoGUID(ksceKernelGetProcessId(), opt.baseBlock)) < 0)
        {
            LOG("Error: Failed to get GUID for baseBlock");
            return ret;
        }

        if ((ret = ksceGUIDReferObject(baseMemBlock, (SceObjectBase **)&pBaseMemBlock)) < 0)
        {
            LOG("Error: Failed to refer baseBlock");
            return ret;
        }
    }

    void *curAddr = addr;
    SceSize curSize = (len + 0xFFF) & ~0xFFF; // Align to page size
    SceUInt32 curOffset = opt.baseOffset;

    pAS = ksceKernelSysrootGetCurrentAddressSpaceCB();

    while (curSize)
    {
        if ((ret = AddressSpaceFindMemBlockCBByAddr(pAS, curAddr, 0, &pMemBlock)) < 0)
        {
            LOG("Error: Failed to find memBlock at address %p", curAddr);
            break;
        }

        if (opt.attr & KU_KERNEL_MEM_COMMIT_ATTR_HAS_BASE)
        {
            if ((ret = MemBlockCommitPagesWithBase(pMemBlock, prot, &curAddr, &curSize, pBaseMemBlock, &curOffset)) < 0)
            {
                LOG("Error: Failed to commit pages with base");
                break;
            }
        }
        else
        {
            if ((ret = MemBlockCommitPages(pMemBlock, prot, &curAddr, &curSize)) < 0)
            {
                LOG("Error: Failed to commit pages");
                break;
            }
        }
    }

    if (opt.attr & KU_KERNEL_MEM_COMMIT_ATTR_HAS_BASE)
        ksceGUIDReleaseObject(baseMemBlock);

    return ret;
}

int kuKernelMemDecommit(void *addr, SceSize len)
{
    SceUIDAddressSpaceObject *pAS;
    SceUIDMemBlockObject *pMemBlock;
    int ret = 0;

    if (len == 0)
        return 0;

    if (((uintptr_t)addr < 0x40000000) || ((uintptr_t)addr & 0xFFF))
    {
        LOG("Error: addr is invalid (%p)", addr);
        return SCE_KERNEL_ERROR_INVALID_ARGUMENT;
    }

    void *curAddr = addr;
    SceSize curSize = (len + 0xFFF) & ~0xFFF; // Align to page size

    pAS = ksceKernelSysrootGetCurrentAddressSpaceCB();

    while (curSize)
    {
        if ((ret = AddressSpaceFindMemBlockCBByAddr(pAS, curAddr, 0, &pMemBlock)) < 0)
        {
            LOG("Error: Failed to find memBlock at address %p", curAddr);
            return ret;
        }

        if ((ret = MemBlockDecommitPages(pMemBlock, &curAddr, &curSize)) < 0)
        {
            LOG("Error: Failed to decommit pages");
            return ret;
        }
    }

    return ret;
}

static SceUID injectUIDs[3];
void InitMemProtect()
{
    tai_module_info_t moduleInfo = {0};
    moduleInfo.size = sizeof(moduleInfo);
    taiGetModuleInfoForKernel(KERNEL_PID, "SceSysmem", &moduleInfo);

    union
    {
        uint16_t u16[6];
        uint32_t u32[3];
    } userRWXPatch;
    /**
     * 0xBF00     - nop
     * 0xC002F8DF - ldr r12, [pc, #0x2]
     * 0x4760     - bx r12
     * 0x00000000 - Address to jump to
     */
    userRWXPatch.u32[0] = 0xF8DFBF00;
    userRWXPatch.u32[1] = 0x4760C002;

    switch (moduleInfo.module_nid)
    {
    case 0x3380B323: // 3.60
        module_get_offset(KERNEL_PID, moduleInfo.modid, 0, 0xCEC4 | 1, (uintptr_t *)&AddressSpaceFindMemBlockCBByAddr);
        module_get_offset(KERNEL_PID, moduleInfo.modid, 0, 0xE09C | 1, (uintptr_t *)&AllocVirPageObject);
        module_get_offset(KERNEL_PID, moduleInfo.modid, 0, 0x3904 | 1, (uintptr_t *)&FreeVirPageObject);
        module_get_offset(KERNEL_PID, moduleInfo.modid, 0, 0x2060 | 1, (uintptr_t *)&MapVirPageCore);
        module_get_offset(KERNEL_PID, moduleInfo.modid, 0, 0x1F54 | 1, (uintptr_t *)&UnmapVirPageWithOpt);
        module_get_offset(KERNEL_PID, moduleInfo.modid, 0, 0x3674 | 1, (uintptr_t *)&VirPageAllocPhyPage);
        module_get_offset(KERNEL_PID, moduleInfo.modid, 0, 0x13640 | 1, (uintptr_t *)&PhyMemPartFreePhyPage);

        module_get_offset(KERNEL_PID, moduleInfo.modid, 0, 0x2B1F8, (uintptr_t *)&MemBlockTypeToL1PTE_DebugContext);
        module_get_offset(KERNEL_PID, moduleInfo.modid, 0, 0x2B2A0, (uintptr_t *)&MemBlockTypeToL2PTESmallPage_DebugContext);
        module_get_offset(KERNEL_PID, moduleInfo.modid, 0, 0x2B1E0, (uintptr_t *)&sceKernelMMUMapLargePages_DebugContext);
        module_get_offset(KERNEL_PID, moduleInfo.modid, 0, 0x23812 | 1, (uintptr_t *)&sceKernelMMUMapLargePages_cont);

        userRWXPatch.u32[2] = (uintptr_t)MemBlockTypeToL1PTE_UserRWX;
        injectUIDs[0] = taiInjectDataForKernel(KERNEL_PID, moduleInfo.modid, 0, 0x23172, &userRWXPatch.u32[0], sizeof(userRWXPatch));

        userRWXPatch.u32[2] = (uintptr_t)MemBlockTypeToL2PTESmallPage_UserRWX;
        injectUIDs[1] = taiInjectDataForKernel(KERNEL_PID, moduleInfo.modid, 0, 0x23274, &userRWXPatch.u16[1], sizeof(userRWXPatch) - 2);

        userRWXPatch.u32[2] = (uintptr_t)MemBlockTypeToL2PTELargePage_UserRWX;
        injectUIDs[2] = taiInjectDataForKernel(KERNEL_PID, moduleInfo.modid, 0, 0x2387A, &userRWXPatch.u32[0], sizeof(userRWXPatch));
        break;
    default: // TODO: Check the offsets & module NIDs for other firmwares
        LOG("Warning: Offsets are unknown for this firmware. Crashes may occur when attempting to use this library.");
    case 0x4DC73B57: // 3.65
        module_get_offset(KERNEL_PID, moduleInfo.modid, 0, 0x10CE0 | 1, (uintptr_t *)&AddressSpaceFindMemBlockCBByAddr);
        module_get_offset(KERNEL_PID, moduleInfo.modid, 0, 0xA114 | 1, (uintptr_t *)&AllocVirPageObject);
        module_get_offset(KERNEL_PID, moduleInfo.modid, 0, 0x15DAC | 1, (uintptr_t *)&FreeVirPageObject);
        module_get_offset(KERNEL_PID, moduleInfo.modid, 0, 0x149E4 | 1, (uintptr_t *)&MapVirPageCore);
        module_get_offset(KERNEL_PID, moduleInfo.modid, 0, 0x148D8 | 1, (uintptr_t *)&UnmapVirPageWithOpt);
        module_get_offset(KERNEL_PID, moduleInfo.modid, 0, 0x15B1C | 1, (uintptr_t *)&VirPageAllocPhyPage);
        module_get_offset(KERNEL_PID, moduleInfo.modid, 0, 0xE52C | 1, (uintptr_t *)&PhyMemPartFreePhyPage);

        module_get_offset(KERNEL_PID, moduleInfo.modid, 0, 0x2B0F8, (uintptr_t *)&MemBlockTypeToL1PTE_DebugContext);
        module_get_offset(KERNEL_PID, moduleInfo.modid, 0, 0x2B170, (uintptr_t *)&MemBlockTypeToL2PTESmallPage_DebugContext);
        module_get_offset(KERNEL_PID, moduleInfo.modid, 0, 0x2B0E0, (uintptr_t *)&sceKernelMMUMapLargePages_DebugContext);
        module_get_offset(KERNEL_PID, moduleInfo.modid, 0, 0x237DE | 1, (uintptr_t *)&sceKernelMMUMapLargePages_cont);

        userRWXPatch.u32[2] = (uintptr_t)MemBlockTypeToL1PTE_UserRWX;
        injectUIDs[0] = taiInjectDataForKernel(KERNEL_PID, moduleInfo.modid, 0, 0x2313E, &userRWXPatch.u32[0], sizeof(userRWXPatch));

        userRWXPatch.u32[2] = (uintptr_t)MemBlockTypeToL2PTESmallPage_UserRWX;
        injectUIDs[1] = taiInjectDataForKernel(KERNEL_PID, moduleInfo.modid, 0, 0x23240, &userRWXPatch.u16[1], sizeof(userRWXPatch) - 2);

        userRWXPatch.u32[2] = (uintptr_t)MemBlockTypeToL2PTELargePage_UserRWX;
        injectUIDs[2] = taiInjectDataForKernel(KERNEL_PID, moduleInfo.modid, 0, 0x23846, &userRWXPatch.u32[0], sizeof(userRWXPatch));
        break;
    }
}

void TermMemProtect()
{
    for (int i = 0; i < 3; i++)
        if (injectUIDs[i] > 0)
            taiInjectReleaseForKernel(injectUIDs[i]);
}
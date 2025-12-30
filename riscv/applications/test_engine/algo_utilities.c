/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "algo_utilities.h"

#ifndef CONFIG_BUILD_ENGINE_TINY
#include <rtthread.h>
#include <core_rv32.h>
#elif CONFIG_USING_WARPED_MEMORY
#include <ax_global_type.h>
#include <ax_os_mem_api.h>
#include <ax_sys_api.h>
#endif

#define ENGINE_CMM_SESSION_NAME "engine_demo"
#define ENGINE_CMM_ALIGN_SIZE   128


#ifndef CONFIG_BUILD_ENGINE_TINY
void* sys_malloc(size_t size)
{
    return rt_malloc((rt_size_t)size);
}


void sys_free(void* ptr)
{
    return rt_free(ptr);
}


int cmm_alloc(void** vir, uint64_t* phy, size_t size)
{
    *phy = (ptrdiff_t)rt_malloc_align((rt_size_t)size, ENGINE_CMM_ALIGN_SIZE);
    *vir = (void*)(ptrdiff_t)(*phy);
    return 0 == phy ? -1 : 0;
}


int cmm_alloc_cache(void** vir, uint64_t* phy, size_t size)
{
    return cmm_alloc(vir, phy, size);
}


int cmm_free(void* vir, uint64_t phy)
{
    rt_free_align((void*)(ptrdiff_t)phy);
    return 0;
}


int cmm_invalidate(void* vir, uint64_t phy, size_t size)
{
    csi_dcache_invalid_range((uint32_t*)((ptrdiff_t)phy), size);
    return 0;
}


int cmm_flush(void* vir, uint64_t phy, size_t size)
{
    csi_dcache_clean_range((uint32_t*)((ptrdiff_t)phy), size);
    return 0;
}
#else
int cmm_alloc(void** vir, uint64_t* phy, size_t size)
{
    return AX_SYS_MemAlloc((AX_U64*)phy, vir, size, ENGINE_CMM_ALIGN_SIZE, (AX_S8*)(ENGINE_CMM_SESSION_NAME));
}


int cmm_alloc_cache(void** vir, uint64_t* phy, size_t size)
{
    return AX_SYS_MemAllocCached((AX_U64*)phy, vir, size, ENGINE_CMM_ALIGN_SIZE, (AX_S8*)(ENGINE_CMM_SESSION_NAME));
}


int cmm_free(void* vir, uint64_t phy)
{
    return AX_SYS_MemFree(phy, vir);
}


int cmm_invalidate(void* vir, uint64_t phy, size_t size)
{
    return AX_SYS_MinvalidateCache(phy, vir, size);
}


int cmm_flush(void* vir, uint64_t phy, size_t size)
{
    return AX_SYS_MflushCache(phy, vir, size);
}

#ifdef CONFIG_USING_WARPED_MEMORY
void* sys_malloc(size_t size)
{
    return AX_OS_MEM_Malloc(AX_ID_ENGINE, size);
}

void sys_free(void* ptr)
{
    // for that free nullptr directly was not allowed in ax sys
    if (NULL != ptr)
    {
        return AX_OS_MEM_Free(AX_ID_ENGINE, ptr);
    }
}
#else
void* sys_malloc(size_t size)
{
    return malloc(size);
}


void sys_free(void* ptr)
{
    free(ptr);
}
#endif
#endif

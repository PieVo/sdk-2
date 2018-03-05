#include "utopia_private.h"
#include "utopia_module.h"
#include <stdio.h>
#include "utopia.h"
#include "utopia_dapi.h"
#include "string.h"
#include "MsOS.h"

//Global Variable===================================================
UTOPIA_PRIVATE* psUtopiaPrivate;

int lib_lowprintf(const char *fmt, ...);
#define printf lib_lowprintf
char moduleNames[][40] = {
#define PREFIX(MODULE) "MODULE_"#MODULE,
	INCLUDED_MODULE
#undef PREFIX
};

char rpoolNames[][40] = {
#define PREFIX(MODULE) "RPOOL_"#MODULE,
	INCLUDED_MODULE
#undef PREFIX
};

char ResourceNames[][40] = {
#define PREFIX(MODULE) "RESOURCE_"#MODULE,
	INCLUDED_MODULE
#undef PREFIX
};

void BDMARegisterToUtopia(FUtopiaOpen ModuleType);
void AESDMARegisterToUtopia(FUtopiaOpen ModuleType);
void MBXRegisterToUtopia(FUtopiaOpen ModuleType);
void DSCMBRegisterToUtopia(void);
void SYSRegisterToUtopia(FUtopiaOpen ModuleType);
void SEALRegisterToUtopia(FUtopiaOpen ModuleType);

MS_U32 UtopiaInit()
{
	MS_U32 u32Ret = 0;

	printf("utopia init\n" );
    psUtopiaPrivate = (UTOPIA_PRIVATE*)malloc(sizeof(UTOPIA_PRIVATE));
    memset(psUtopiaPrivate, 0, sizeof(UTOPIA_PRIVATE));
	psUtopiaPrivate->u32MutexID = MsOS_CreateMutex(E_MSOS_FIFO
			, "UtopiaPrivateMutex", MSOS_PROCESS_SHARED);

	MsOS_ObtainMutex(psUtopiaPrivate->u32MutexID, MSOS_WAIT_FOREVER);
	/*
	 * to avoid weak attribute malfunction in static lib,
	 * we explicitly call needed functions 
	 */
	UTOPIARegisterToUtopia((FUtopiaOpen)MODULE_TYPE_UTOPIA_FULL);
	BDMARegisterToUtopia((FUtopiaOpen)MODULE_TYPE_BDMA_FULL);
	AESDMARegisterToUtopia((FUtopiaOpen)MODULE_TYPE_AESDMA_FULL);
	MBXRegisterToUtopia((FUtopiaOpen)MODULE_TYPE_MBX_FULL);
	DSCMBRegisterToUtopia();
    SYSRegisterToUtopia(NULL);
        SEALRegisterToUtopia((FUtopiaOpen)MODULE_TYPE_SEAL_FULL);
	MsOS_ReleaseMutex(psUtopiaPrivate->u32MutexID);

	return u32Ret;
}

MS_U32 UtopiaOpen(MS_U32 u32ModuleID, void** ppInstanceTmp
		, MS_U32 u32ModuleVersion, const void* const pAttribute)
{
    UTOPIA_MODULE* psUtopiaModule = psUtopiaPrivate->psModuleHead;
    UTOPIA_INSTANCE** ppInstance = (UTOPIA_INSTANCE**)ppInstanceTmp;


    // psUtopiaInstant->u32OpenThreadName = pthred_ctrl(get_name); // FIXME: need to be discussed implementation
    while(psUtopiaModule != NULL)
    {
        if(psUtopiaModule->u32ModuleID == u32ModuleID)//need modify to link list
        {
            int ret = psUtopiaModule->fpOpen((void**)ppInstance, pAttribute);
            
            if(ret)
            {
                printf("[utopia error] fail to create instance\n");
                return ret;
            }
            
            (*ppInstance)->psModule = psUtopiaModule;
			(*ppInstance)->u32AppRequireModuleVersion = u32ModuleVersion;
            return ret;
        }
        psUtopiaModule = psUtopiaModule->psNext;
    }
	return 0xFFFFFFFF; // FIXME: 1. error code, 2. to avoid possible fpOpen return value
}

MS_U32 UtopiaIoctl(void* pInstanceTmp, MS_U32 u32Cmd, void* const pArgs)
{
	UTOPIA_INSTANCE* pInstance = (UTOPIA_INSTANCE*)pInstanceTmp;
    return ((UTOPIA_INSTANCE*)pInstance)->psModule->fpIoctl(pInstance, u32Cmd, pArgs);
}

MS_U32 UtopiaClose(void* pInstantTmp)
{
    UTOPIA_INSTANCE* pInstant = (UTOPIA_INSTANCE*)pInstantTmp;

    UTOPIA_MODULE* psUtopiaModule = psUtopiaPrivate->psModuleHead;
    while(psUtopiaModule != NULL)
    {
        if(psUtopiaModule->u32ModuleID == pInstant->psModule->u32ModuleID)
        {
            return psUtopiaModule->fpClose(pInstant);
        }
        psUtopiaModule = psUtopiaModule->psNext;
    }
	return 0;
}


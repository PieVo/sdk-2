#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <poll.h>
#include "mi_sys.h"

void *ChnOutputPortGetBuf1(void *arg)
{
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;

    int framecount = 50;
    int count0 = 0 , count1 = 0;
    int fd0  =-1 ,fd1 =-1;
    memset(&stBufInfo , 0 , sizeof(MI_SYS_BufInfo_t));

    MI_SYS_ChnPort_t stVifChn0OutputPort0;
    stVifChn0OutputPort0.eModId = E_MI_MODULE_ID_VIF;
    stVifChn0OutputPort0.u32DevId = 0;
    stVifChn0OutputPort0.u32ChnId = 0;
    stVifChn0OutputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stVifChn0OutputPort1;
    stVifChn0OutputPort1.eModId = E_MI_MODULE_ID_VIF;
    stVifChn0OutputPort1.u32DevId = 0;
    stVifChn0OutputPort1.u32ChnId = 0;
    stVifChn0OutputPort1.u32PortId = 1;

    MI_SYS_ChnPort_t stVpeChn0InputPort0;
    stVpeChn0InputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn0InputPort0.u32DevId = 0;
    stVpeChn0InputPort0.u32ChnId = 0;
    stVpeChn0InputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stVpeChn0OutputPort0;
    stVpeChn0OutputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn0OutputPort0.u32DevId = 0;
    stVpeChn0OutputPort0.u32ChnId = 0;
    stVpeChn0OutputPort0.u32PortId = 0;


    ///////////////////////////////
    //vb pool debug  start.
    MI_VB_PoolListConf_t stPoolListConf;
    int ret;
    int case_choose =1;

    /*
    N.B.
    In this test case,to avoid mi_sys_vbpool_allocator_suit_bufconfig
    go into if(list_empty(&pst_vbpool_allocator->list_of_free_allocations))
    here for mma_heap_name0 size 229MB,
    set:
    0x100000*28 + 0x300000*11 + 0x500000*15
    */

    stPoolListConf.u32PoolListCnt = 3;
    strcpy(&(stPoolListConf.stPoolConf[0].u8MMAHeapName[0]),"mma_heap_name0");

    stPoolListConf.stPoolConf[0].u32BlkSize = 0x100000;
    stPoolListConf.stPoolConf[0].u32BlkCnt = 28;
    strcpy(&(stPoolListConf.stPoolConf[1].u8MMAHeapName[0]),"mma_heap_name0");
    stPoolListConf.stPoolConf[1].u32BlkSize = 0x300000;
    stPoolListConf.stPoolConf[1].u32BlkCnt = 11;
    strcpy(&(stPoolListConf.stPoolConf[2].u8MMAHeapName[0]),"mma_heap_name0");
    stPoolListConf.stPoolConf[2].u32BlkSize = 0x500000;
    stPoolListConf.stPoolConf[2].u32BlkCnt = 15;
    if(1 == case_choose)
    {
        printf("%s:%d  will MI_SYS_ConfDevPubPools \n",__FUNCTION__,__LINE__);
        ret = MI_SYS_ConfDevPubPools(stVpeChn0OutputPort0.eModId, stVpeChn0OutputPort0.u32DevId,  stPoolListConf);
        if(ret != MI_SUCCESS)
            printf("%s:%d  MI_SYS_ConfDevPubPools fail\n",__FUNCTION__,__LINE__);
    }
    else if(2 == case_choose)
    {
        printf("%s:%d  will MI_SYS_ConfGloPubPools \n",__FUNCTION__,__LINE__);
        ret = MI_SYS_ConfGloPubPools(stPoolListConf);
        if(ret != MI_SUCCESS)
            printf("%s:%d  MI_SYS_ConfGloPubPools fail\n",__FUNCTION__,__LINE__);
    }
    //vb pool debug  end
    /////////////////////////////////



    MI_SYS_ChnPort_t stVpeChn1InputPort0;
    stVpeChn1InputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn1InputPort0.u32DevId = 0;
    stVpeChn1InputPort0.u32ChnId = 1;
    stVpeChn1InputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stVpeChn1OutputPort3;
    stVpeChn1OutputPort3.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn1OutputPort3.u32DevId = 0;
    stVpeChn1OutputPort3.u32ChnId = 1;
    stVpeChn1OutputPort3.u32PortId = 3;

    ///////////////////////////////
    //vb pool  debug  start.
    /*
    before already done for this [eModId,u32DevId].so no need again
    if(1 == case_choose)
    {
        printf("%s:%d  will MI_SYS_ConfDevPubPools \n",__FUNCTION__,__LINE__);
        ret = MI_SYS_ConfDevPubPools(stVpeChn1OutputPort3.eModId, stVpeChn1OutputPort3.u32DevId,  stPoolListConf);
    }
    */
    //vb pool debug  end
    /////////////////////////////////


    MI_SYS_SetChnOutputPortDepth(&stVpeChn0OutputPort0,3,5);
    MI_SYS_SetChnOutputPortDepth(&stVpeChn1OutputPort3,3,5);

    MI_SYS_BindChnPort(&stVifChn0OutputPort0,&stVpeChn0InputPort0,30,30);
    MI_SYS_BindChnPort(&stVifChn0OutputPort1,&stVpeChn1InputPort0,30,30);
    
    fd0 =open("/mnt/test/Vpe_ch0port0_yuv422.yuv",O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if(fd0 < 0)
    {
        printf("open file fail\n");

        /////vb pool debug start
        goto RETURN_POS;
        /////vb pool debug end
        return 0;      
    }

    fd1 =open("/mnt/test/Vpe_ch1port3_yuv422.yuv",O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if(fd1 < 0)
    {
        printf("open file fail\n");
        /////vb pool debug start
        goto RETURN_POS;
        /////vb pool debug end
        return 0;      
    }

    while(1)
    {

        if(MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stVpeChn0OutputPort0,&stBufInfo,&hHandle))
        {
            if(stBufInfo.eBufType != E_MI_SYS_BUFDATA_FRAME)
                printf("data read error\n");
            MI_U32 u32Size = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
            write(fd0,stBufInfo.stFrameData.pVirAddr[0],u32Size);
            MI_SYS_ChnOutputPortPutBuf(hHandle);
            count0 ++;
        }
        if(MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stVpeChn1OutputPort3,&stBufInfo,&hHandle))
        {
            if(stBufInfo.eBufType != E_MI_SYS_BUFDATA_FRAME)
                printf("data read error\n");
            MI_U32 u32Size = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
            write(fd1,stBufInfo.stFrameData.pVirAddr[0],u32Size);
            MI_SYS_ChnOutputPortPutBuf(hHandle);
            count1 ++;
        }

        if(count0 > framecount && count1 > framecount)
            break;

    }

    MI_SYS_SetChnOutputPortDepth(&stVpeChn0OutputPort0,0,5);
    MI_SYS_SetChnOutputPortDepth(&stVpeChn1OutputPort3,0,5);

    MI_SYS_UnBindChnPort(&stVifChn0OutputPort0,&stVpeChn0InputPort0);
    MI_SYS_UnBindChnPort(&stVifChn0OutputPort1,&stVpeChn1InputPort0);

    printf("vif & vpe test end\n");
    
    if(fd0 > 0) close(fd0);
    if(fd1 > 0) close(fd1);

/////vb pool debug start
RETURN_POS:
    if(1 == case_choose)
    {
        printf("%s:%d  will MI_SYS_ReleaseDevPubPools \n",__FUNCTION__,__LINE__);
        ret = MI_SYS_ReleaseDevPubPools(stVpeChn0OutputPort0.eModId, stVpeChn0OutputPort0.u32DevId);
        if(ret != MI_SUCCESS)
            printf("%s:%d  MI_SYS_ReleaseDevPubPools fail\n",__FUNCTION__,__LINE__);
    }
    else if(2 == case_choose)
    {
        printf("%s:%d  will MI_SYS_ReleaseGloPubPools \n",__FUNCTION__,__LINE__);
        ret = MI_SYS_ReleaseGloPubPools();
        if(ret != MI_SUCCESS)
            printf("%s:%d  MI_SYS_ReleaseGloPubPools fail\n",__FUNCTION__,__LINE__);
    }
    return 0;
/////vb pool debug end
}


void *ChnOutputPortGetBuf2(void *arg)
{
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;

    int framecount = 50;
    int count0 = 0 , count1 = 0;
    int fd0  =-1 ,fd1 =-1;

    memset(&stBufInfo , 0 , sizeof(MI_SYS_BufInfo_t));

    MI_SYS_ChnPort_t stVifChn1OutputPort0;
    stVifChn1OutputPort0.eModId = E_MI_MODULE_ID_VIF;
    stVifChn1OutputPort0.u32DevId = 0;
    stVifChn1OutputPort0.u32ChnId = 1;
    stVifChn1OutputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stVifChn1OutputPort1;
    stVifChn1OutputPort1.eModId = E_MI_MODULE_ID_VIF;
    stVifChn1OutputPort1.u32DevId = 0;
    stVifChn1OutputPort1.u32ChnId = 1;
    stVifChn1OutputPort1.u32PortId = 1;

    MI_SYS_ChnPort_t stVpeChn2InputPort0;
    stVpeChn2InputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn2InputPort0.u32DevId = 0;
    stVpeChn2InputPort0.u32ChnId = 2;
    stVpeChn2InputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stVpeChn2OutputPort0;
    stVpeChn2OutputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn2OutputPort0.u32DevId = 0;
    stVpeChn2OutputPort0.u32ChnId = 2;
    stVpeChn2OutputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stVpeChn3InputPort0;
    stVpeChn3InputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn3InputPort0.u32DevId = 0;
    stVpeChn3InputPort0.u32ChnId = 3;
    stVpeChn3InputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stVpeChn3OutputPort3;
    stVpeChn3OutputPort3.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn3OutputPort3.u32DevId = 0;
    stVpeChn3OutputPort3.u32ChnId = 3;
    stVpeChn3OutputPort3.u32PortId = 3;

    MI_SYS_SetChnOutputPortDepth(&stVpeChn2OutputPort0,3,5);
    MI_SYS_SetChnOutputPortDepth(&stVpeChn3OutputPort3,3,5);

    MI_SYS_BindChnPort(&stVifChn1OutputPort0,&stVpeChn2InputPort0,30,30);
    MI_SYS_BindChnPort(&stVifChn1OutputPort1,&stVpeChn3InputPort0,30,30);
    
    fd0 =open("/mnt/test/Vpe_ch2port0_yuv422.yuv",O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if(fd0 < 0)
    {
        printf("open file fail\n");
        return 0;      
    }

    fd1 =open("/mnt/test/Vpe_ch3port3_yuv422.yuv",O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if(fd1 < 0)
    {
        printf("open file fail\n");
        return 0;      
    }

    while(1)
    {

        if(MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stVpeChn2OutputPort0,&stBufInfo,&hHandle))
        {
            if(stBufInfo.eBufType != E_MI_SYS_BUFDATA_FRAME)
                printf("data read error\n");
            MI_U32 u32Size = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
            write(fd0,stBufInfo.stFrameData.pVirAddr[0],u32Size);
            MI_SYS_ChnOutputPortPutBuf(hHandle);
            count0 ++;
        }
        if(MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stVpeChn3OutputPort3,&stBufInfo,&hHandle))
        {
            if(stBufInfo.eBufType != E_MI_SYS_BUFDATA_FRAME)
                printf("data read error\n");
            MI_U32 u32Size = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
            write(fd1,stBufInfo.stFrameData.pVirAddr[0],u32Size);
            MI_SYS_ChnOutputPortPutBuf(hHandle);
            count1 ++;
        }
        if(count0 > framecount && count1 > framecount)
            break;

    }

    MI_SYS_SetChnOutputPortDepth(&stVpeChn2OutputPort0,0,5);
    MI_SYS_SetChnOutputPortDepth(&stVpeChn3OutputPort3,0,5);

    MI_SYS_UnBindChnPort(&stVifChn1OutputPort0,&stVpeChn2InputPort0);
    MI_SYS_UnBindChnPort(&stVifChn1OutputPort1,&stVpeChn3InputPort0);

    printf("vif & vpe test end\n");
    if(fd0 > 0) close(fd0);
    if(fd1 > 0) close(fd1);
}

void *ChnOutputPortGetBuf3(void *arg)
{
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;

    int framecount = 50;
    int count0 = 0 , count1 = 0;
    int fd0  =-1 ,fd1 =-1;

    memset(&stBufInfo , 0 , sizeof(MI_SYS_BufInfo_t));

    MI_SYS_ChnPort_t stVifChn2OutputPort0;
    stVifChn2OutputPort0.eModId = E_MI_MODULE_ID_VIF;
    stVifChn2OutputPort0.u32DevId = 0;
    stVifChn2OutputPort0.u32ChnId = 2;
    stVifChn2OutputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stVifChn2OutputPort1;
    stVifChn2OutputPort1.eModId = E_MI_MODULE_ID_VIF;
    stVifChn2OutputPort1.u32DevId = 0;
    stVifChn2OutputPort1.u32ChnId = 2;
    stVifChn2OutputPort1.u32PortId = 1;

    MI_SYS_ChnPort_t stVpeChn4InputPort0;
    stVpeChn4InputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn4InputPort0.u32DevId = 0;
    stVpeChn4InputPort0.u32ChnId = 4;
    stVpeChn4InputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stVpeChn4OutputPort0;
    stVpeChn4OutputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn4OutputPort0.u32DevId = 0;
    stVpeChn4OutputPort0.u32ChnId = 4;
    stVpeChn4OutputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stVpeChn5InputPort0;
    stVpeChn5InputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn5InputPort0.u32DevId = 0;
    stVpeChn5InputPort0.u32ChnId = 5;
    stVpeChn5InputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stVpeChn5OutputPort3;
    stVpeChn5OutputPort3.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn5OutputPort3.u32DevId = 0;
    stVpeChn5OutputPort3.u32ChnId = 5;
    stVpeChn5OutputPort3.u32PortId = 3;

    MI_SYS_SetChnOutputPortDepth(&stVpeChn4OutputPort0,3,5);
    MI_SYS_SetChnOutputPortDepth(&stVpeChn5OutputPort3,3,5);

    MI_SYS_BindChnPort(&stVifChn2OutputPort0,&stVpeChn4InputPort0,30,30);
    MI_SYS_BindChnPort(&stVifChn2OutputPort1,&stVpeChn5InputPort0,30,30);
    
    fd0 =open("/mnt/test/Vpe_ch4port0_yuv422.yuv",O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if(fd0 < 0)
    {
        printf("open file fail\n");
        return 0;      
    }

    fd1 =open("/mnt/test/Vpe_ch5port3_yuv422.yuv",O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if(fd1 < 0)
    {
        printf("open file fail\n");
        return 0;      
    }

    while(1)
    {

        if(MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stVpeChn4OutputPort0,&stBufInfo,&hHandle))
        {
            if(stBufInfo.eBufType != E_MI_SYS_BUFDATA_FRAME)
                printf("data read error\n");
            MI_U32 u32Size = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
            write(fd0,stBufInfo.stFrameData.pVirAddr[0],u32Size);
            MI_SYS_ChnOutputPortPutBuf(hHandle);
            count0 ++;
        }
        if(MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stVpeChn5OutputPort3,&stBufInfo,&hHandle))
        {
            if(stBufInfo.eBufType != E_MI_SYS_BUFDATA_FRAME)
                printf("data read error\n");
            MI_U32 u32Size = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
            write(fd1,stBufInfo.stFrameData.pVirAddr[0],u32Size);
            MI_SYS_ChnOutputPortPutBuf(hHandle);
            count1 ++;
        }
        if(count0 > framecount && count1 > framecount)
            break;
    }

    MI_SYS_SetChnOutputPortDepth(&stVpeChn4OutputPort0,0,5);
    MI_SYS_SetChnOutputPortDepth(&stVpeChn5OutputPort3,0,5);

    MI_SYS_UnBindChnPort(&stVifChn2OutputPort0,&stVpeChn4InputPort0);
    MI_SYS_UnBindChnPort(&stVifChn2OutputPort1,&stVpeChn5InputPort0);

    printf("vif & vpe test end\n");
    if(fd0 > 0)close(fd0);
    if(fd1 > 0)close(fd1);
}


void *ChnOutputPortGetBuf4(void *arg)
{
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;

    int framecount = 50;
    int count0 = 0 , count1 = 0;
    int fd0  =-1 ,fd1 =-1;

    memset(&stBufInfo , 0 , sizeof(MI_SYS_BufInfo_t));

    MI_SYS_ChnPort_t stVifChn3OutputPort0;
    stVifChn3OutputPort0.eModId = E_MI_MODULE_ID_VIF;
    stVifChn3OutputPort0.u32DevId = 0;
    stVifChn3OutputPort0.u32ChnId = 3;
    stVifChn3OutputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stVifChn3OutputPort1;
    stVifChn3OutputPort1.eModId = E_MI_MODULE_ID_VIF;
    stVifChn3OutputPort1.u32DevId = 0;
    stVifChn3OutputPort1.u32ChnId = 3;
    stVifChn3OutputPort1.u32PortId = 1;

    MI_SYS_ChnPort_t stVpeChn6InputPort0;
    stVpeChn6InputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn6InputPort0.u32DevId = 0;
    stVpeChn6InputPort0.u32ChnId = 6;
    stVpeChn6InputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stVpeChn6OutputPort0;
    stVpeChn6OutputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn6OutputPort0.u32DevId = 0;
    stVpeChn6OutputPort0.u32ChnId = 6;
    stVpeChn6OutputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stVpeChn7InputPort0;
    stVpeChn7InputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn7InputPort0.u32DevId = 0;
    stVpeChn7InputPort0.u32ChnId = 7;
    stVpeChn7InputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stVpeChn7OutputPort3;
    stVpeChn7OutputPort3.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn7OutputPort3.u32DevId = 0;
    stVpeChn7OutputPort3.u32ChnId = 7;
    stVpeChn7OutputPort3.u32PortId = 3;

    MI_SYS_SetChnOutputPortDepth(&stVpeChn6OutputPort0,3,5);
    MI_SYS_SetChnOutputPortDepth(&stVpeChn7OutputPort3,3,5);

    MI_SYS_BindChnPort(&stVifChn3OutputPort0,&stVpeChn6InputPort0,30,30);
    MI_SYS_BindChnPort(&stVifChn3OutputPort1,&stVpeChn7InputPort0,30,30);
    
    fd0 =open("/mnt/test/Vpe_ch6port0_yuv422.yuv",O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if(fd0 < 0)
    {
        printf("open file fail\n");
        return 0;      
    }

    fd1 =open("/mnt/test/Vpe_ch7port3_yuv422.yuv",O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if(fd1 < 0)
    {
        printf("open file fail\n");
        return 0;      
    }

    while(1)
    {

        if(MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stVpeChn6OutputPort0,&stBufInfo,&hHandle))
        {
            if(stBufInfo.eBufType != E_MI_SYS_BUFDATA_FRAME)
                printf("data read error\n");
            MI_U32 u32Size = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
            write(fd0,stBufInfo.stFrameData.pVirAddr[0],u32Size);
            MI_SYS_FlushCache(stBufInfo.stFrameData.pVirAddr[0],u32Size);
            MI_SYS_ChnOutputPortPutBuf(hHandle);
            count0 ++;
        }
        if(MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stVpeChn7OutputPort3,&stBufInfo,&hHandle))
        {
            if(stBufInfo.eBufType != E_MI_SYS_BUFDATA_FRAME)
                printf("data read error\n");
            MI_U32 u32Size = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
            write(fd1,stBufInfo.stFrameData.pVirAddr[0],u32Size);
            MI_SYS_FlushCache(stBufInfo.stFrameData.pVirAddr[0],u32Size);
            MI_SYS_ChnOutputPortPutBuf(hHandle);
            count1 ++;
        }
        if(count0 > framecount && count1 > framecount)
            break;
    }

    MI_SYS_SetChnOutputPortDepth(&stVpeChn6OutputPort0,0,5);
    MI_SYS_SetChnOutputPortDepth(&stVpeChn7OutputPort3,0,5);

    MI_SYS_UnBindChnPort(&stVifChn3OutputPort0,&stVpeChn6InputPort0);
    MI_SYS_UnBindChnPort(&stVifChn3OutputPort1,&stVpeChn7InputPort0);

    printf("vif & vpe test end\n");
    if(fd0 > 0)close(fd0);
    if(fd1 > 0)close(fd1);
}


int main(int argc, const char *argv[])
{

    MI_SYS_Init();

    pthread_t tid0 , tid1 ,tid2,tid3;
    pthread_create(&tid0, NULL, ChnOutputPortGetBuf1, NULL);
    pthread_create(&tid1, NULL, ChnOutputPortGetBuf2, NULL);
    pthread_create(&tid2, NULL, ChnOutputPortGetBuf3, NULL);
    pthread_create(&tid3, NULL, ChnOutputPortGetBuf4, NULL);

    pthread_join(tid0,NULL);
    pthread_join(tid1,NULL);
    pthread_join(tid2,NULL);
    pthread_join(tid3,NULL);
    
    return 0;
}

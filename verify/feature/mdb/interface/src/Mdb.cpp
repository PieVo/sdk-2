#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <sys/types.h>
#ifdef USE_SHM
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#endif
#ifdef USE_SOCKET
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#endif
#include "Mdb.h"
#include "Mdb_Base.h"

#define PTHREAD_STACK_SIZE   8*131072
#ifdef USE_SHM
struct shm_st
{
    char buf[TRANS_BUFFER];
    int bWrite;
};
#endif
Mdb *Mdb::pInstance = NULL;

void Mdb::Run(void)
{
    if (pInstance == NULL)
    {
        int intPTHChk;

        pInstance = new (std::nothrow)Mdb();
        ASSERT(pInstance);
        pthread_attr_init(&pInstance->tAttr);
        pthread_attr_setstacksize(&pInstance->tAttr, PTHREAD_STACK_SIZE);
        intPTHChk = PTH_RET_CHK(pthread_create(&pInstance->tTid, &pInstance->tAttr, TransThread, (void *)pInstance));
        ASSERT(intPTHChk == 0);
    }
}

void Mdb::Wait(void)
{
    void *pGetRet = NULL;

    if (pInstance == NULL)
    {
        printf("Mdb do not implement.!\n");
        return;
    }
    PTH_RET_CHK(pthread_join(pInstance->tTid, &pGetRet));
    delete pInstance;
    pInstance = NULL;
}
void Mdb::ProcessStrings(char *pInStr, char *pOutStr)
{
    if (pInStr == NULL || pOutStr == NULL)
    {
        return;
    }
    ParseStrings(pInStr);
    memset(pOutStr, 0, TRANS_BUFFER);

    if (strSubCmd.empty() == 0 && strSubCmd != "c")//strSubCmd not empty
    {
        if (strSubCmd == "q")
        {
            if (Mdb_Base::GetInstance() != NULL)
            {
                Mdb_Base::GetInstance()->Destroy();
            }
            else
            {
                bExitTransThread = 1;
            }
            strSubCmd.clear();
            strInStrings.clear();
            return;
        }
        if (strSubCmd == "w")
        {
            if (Mdb_Base::GetInstance() != NULL)
            {
                Mdb_Base::GetInstance()->Destroy();
            }
            ShowWelcome();
        }
        else
        {
            if (Mdb_Base::GetInstance() == NULL)
            {
                Mdb_Base::Create((MDB_MODULE)(Mdb_Base::Atoi(strSubCmd)));
                if (Mdb_Base::GetInstance() != NULL)
                {
                    Mdb_Base::GetInstance()->ShowWelcome(strOutString);
                }
            }
            else
            {
                if (strSubCmd == "n")
                {
                    Mdb_Base::GetInstance()->GetName(strOutString);
                }
                else if (strSubCmd == "t")
                {
                    Mdb_Base::GetInstance()->DumpCmd(strOutString);
                }
                else
                {
                    Mdb_Base::GetInstance()->SetPara(strSubCmd, strInStrings);
                    Mdb_Base::GetInstance()->DoCmd(strOutString);
                }
            }
        }
    }
    if (strOutString.empty() == 0)
    {
        if (strOutString.length() >= TRANS_BUFFER)
        {
            strncpy(pOutStr, strOutString.c_str(), TRANS_BUFFER - 1);
            strOutString.erase(0, TRANS_BUFFER - 1);
        }
        else
        {
            strncpy(pOutStr, strOutString.c_str(), strOutString.length());
            strOutString.clear();
        }
    }
    strSubCmd.clear();
    strInStrings.clear();
}

void Mdb::ParseStrings(char *pStr)
{
    char *pStr1 = NULL;
    char *pStr2 = NULL;
    int intCmdSize = 0;
    std::string strTmp;

    if (pStr == NULL)
    {
        printf("pStr is NULL!\n");
        return;
    }

    pStr1 = strstr(pStr, "mdb");
    if (pStr1 == NULL)
    {
        printf("Not parse mdb!\n");
        return;
    }
    pStr1 += strlen("mdb");
    if (*pStr1 != ' ')
    {
        return;
    }
    while (1)
    {
        while (*pStr1 == ' ' && *pStr1 != 0)
        {
            pStr1++;
        }
        pStr2 = pStr1;
        while (*pStr2 != ' ' && *pStr2 != 0)
        {
            pStr2++;
        }
        intCmdSize = pStr2 - pStr1;
        if (intCmdSize != 0)
        {
            if (strSubCmd.empty())
            {
                strSubCmd.assign(pStr1, intCmdSize);
            }
            else
            {
                strTmp.assign(pStr1, intCmdSize);
                strInStrings.push_back(strTmp);
            }
            pStr1 = pStr2;
        }
        else
        {
            break;
        }
    }
}
void Mdb::ShowWelcome(void)
{
    Mdb_Base::Print(strOutString, "Welcome to Mdebug\n", PRINT_COLOR_GREEN, PRINT_MODE_HIGHTLIGHT);
    Mdb_Base::Print(strOutString, "Waiting for your direction.\n", PRINT_COLOR_BLUE, PRINT_MODE_FLICK);
    Mdb_Base::PrepareModule(strOutString);
}
void *Mdb::TransThread(void *pPara)
{
    pInstance->bExitTransThread = 0;
    pInstance->TransGetData();

    return NULL;
}
void Mdb::TransGetData(void)
{
#ifdef USE_SHM
    struct shm_st shm_config;
    struct shm_st *p_shm_config = NULL;
    int shmid = 0;
    void *shm = NULL;

    prctl(PR_SET_NAME, (unsigned long)"MdbShmServer");
    shmid = shmget((key_t)88860611, sizeof(struct shm_st), 0666|IPC_CREAT);
    if(shmid == -1)
    {
        fprintf(stderr, "shmget failed\n");
        return;
    }
    memset(&shm_config, 0, sizeof(struct shm_st));

    shm = shmat(shmid, (void*)0, 0);
    if(shm == (void*)-1 || shm == NULL)
    {
        fprintf(stderr, "shmat failed\n");
        return;
    }
    p_shm_config = (struct shm_st *)shm;
    while (bExitTransThread == 0)
    {
        if (p_shm_config->bWrite == 1)
        {
            ProcessStrings(p_shm_config->buf, p_shm_config->buf);
            p_shm_config->bWrite = 0;
        }
        usleep(1000);
    }

    if(shmdt(shm) == -1)
    {
        fprintf(stderr, "shmdt failed\n");
        return;
    }

    return;
#endif
#ifdef USE_SOCKET
    int server_sockfd, client_sockfd;
    int server_len, client_len;
    struct sockaddr_un server_address;
    struct sockaddr_un client_address;
    char cTransBuffer[TRANS_BUFFER];

    prctl(PR_SET_NAME, (unsigned long)"MdbSocketServer");
    unlink (SOCKET_ADDR);
    server_sockfd = socket (AF_UNIX, SOCK_STREAM, 0);
    if (server_sockfd == -1)
    {
        perror ("socket");
        return;
    }
    server_address.sun_family = AF_UNIX;
    strcpy (server_address.sun_path, SOCKET_ADDR);
    server_len = sizeof (server_address);
    if (-1 == bind (server_sockfd, (struct sockaddr *)&server_address, server_len))
    {
        perror ("bind");
        return;
    }
    if (-1 == listen (server_sockfd, 5))
    {
        perror ("listen");
        return;
    }
    printf ("Server is waiting for client connect...\n");

    while (bExitTransThread == 0)
    {
        client_len = sizeof (client_address);
        client_sockfd = accept(server_sockfd, (struct sockaddr *)&server_address, (socklen_t *)&client_len);
        if (client_sockfd == -1)
        {
            perror ("accept");
            return;
        }
        printf ("The server is waiting for client data...\n");
        while (bExitTransThread == 0)
        {
            memset(cTransBuffer, 0, sizeof(cTransBuffer));
            if (recv(client_sockfd, cTransBuffer, sizeof(cTransBuffer), 0) <= 0)
            {
                perror("recv");
                break;
            }
            ProcessStrings(cTransBuffer, cTransBuffer);
            if (send(client_sockfd, cTransBuffer, sizeof(cTransBuffer), 0) < 0)
            {
                perror("send");
                break;
            }
        }
        close(client_sockfd);
    }
    close(server_sockfd);
    unlink (SOCKET_ADDR);
#endif
}

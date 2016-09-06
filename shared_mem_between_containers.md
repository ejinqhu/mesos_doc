# ����Docker��ͬ����ʵ����Ĺ����ڴ�ʾ��
------

## 1.����
����ĳЩ���еĳ����Ѿ�ʹ���˹����ڴ�ķ�ʽ���н��̼��ͨ�ţ���Dockerһ���ǽ�ĳһ������з�װ�����������������У�Ϊ�˽�����ì�ܣ�Docker������--ipcѡ����֧��Docker������Ŀ��Լ���ʹ�ù����ڴ�ķ�ʽ���Ӷ����������г�����Docker����Ǩ�ƵĸĶ���
���ĵ����ṩ��ͨ�Ĺ����ڴ����һ����shm_write,�������ڴ�����д��ǰʱ�䣻 һ����shm_read���ӹ����ڴ������ǰʱ�䣬����ʹ��docker logs������֤��Ȼ�󴴽�һ����򵥵�Dockerfile��Ȼ��build���������в��ԡ�

## 2. �����ڴ����
### 2.1 д����
    #include <stdio.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include <errno.h>
    #include <string.h>
    #include <sys/ipc.h>
    #include <sys/shm.h>
    #include <sys/types.h>
    #include <fcntl.h>
    #include <sys/stat.h>
    
    
    
    int main(void)
    {
        const int PROJ_ID  = 8;
        const int SHM_SIZE = 4096;
        const int STR_LEN  = 128;
        key_t     shm_key;
        int       shm_id;
        char      *shm_addr = NULL;
        mode_t    file_mode = S_IRWXU | S_IRGRP | S_IROTH ;
        char      shm_path[] = "/dev/shm/shm_file";
        char      shm_buf[STR_LEN]; 
        int       fd;

        // judge whether have created this file
        if ( 0 ==  access(shm_path, R_OK))
        {
            if ( 0 != remove(shm_path))
            {
                 printf("remove file failed. %s\n", strerror(errno));
                 exit(-1);
            }
        }
    
        // create file
        if ( creat(shm_path, file_mode) < 0)
        {
            printf("create file error: %s.\n", strerror(errno));
            exit(-1);
        }
        
        if ( (shm_key = ftok(shm_path, PROJ_ID)) < 0)
        {
            printf("create share file key id error: %s.\n", strerror(errno));
            exit(-1);
        }
        
        // here 0600 is very important, missing it, there is
        // permission denied error.
        if ( (shm_id = shmget(shm_key, SHM_SIZE, IPC_CREAT | 0600)) < 0)
        {
            printf("get shm key error: %s.\n", strerror(errno));
            exit(-1);
        }
        
        if ( (char *) -1 == (shm_addr = shmat(shm_id, 0, 0)))
        {
            printf("map the shm error: %s.\n", strerror(errno));
            exit(-1);
        }
        
        // write current timestamp into shm
        do 
        {
            snprintf(shm_addr, sizeof(shm_buf), "%ld", time(0));
            sleep(1);
            // here should insert some code to break this loop, ignore just for example
        } while (1);
    
        // delete shm memory
        if ( shmdt(shm_addr) < 0 )
        {
            printf("detach shm error: %s.\n", strerror(errno));
            exit(-1);
        }
    
        // delete shm id
        if ( shmctl(shm_id, IPC_RMID, NULL) < 0)
        {
            printf("delete shm error: %s.\n", strerror(errno));
            exit(-1);
        }
    
        return 0;
    }

### 2.2 ������

    #include <stdio.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include <errno.h>
    #include <string.h>
    #include <sys/ipc.h>
    #include <sys/shm.h>
    #include <sys/types.h>
    
    int main(void)
    {
       const int PROJ_ID    = 8;
       const int SHM_SIZE   = 4096;
       key_t     shm_key;
       int       shm_id;
       char*     shm_addr   = NULL;
       char      shm_path[] = "/dev/shm/shm_file";
    
    
    
       if ( (shm_key = ftok(shm_path, PROJ_ID)) < 0)
       {
           printf("create share file key id error: %s\n", strerror(errno));
           exit(-1);
       }
    
    
       if ( (shm_id = shmget(shm_key, SHM_SIZE, IPC_CREAT | 0600)) < 0)
       {
           printf("get shm key error: %s.\n", strerror(errno));
           exit(-1);
       }
    
    
       if ( (char *) -1 == (shm_addr = shmat(shm_id, NULL, 0)))
       {
           printf("map the shm error: %s.\n", strerror(errno));
           exit(-1);
       }
    
       do {
           printf("now time: %s\n", (char *)shm_addr);
           sleep(1);
       } while ( 1);
    
       if ( shmdt(shm_addr) < 0 )
       {
           printf("detach shm error: %s.\n", strerror(errno));
           exit(-1);
       }
    
       return 0;
    }

### 2.3 ����

    [root@dell test]# gcc shm_write.c -o shm_write
    [root@dell test]# gcc shm_read.c -o shm_read

### 2.4 ����

    [root@dell test]# ./shm_write &
    [2] 112431
    [root@dell test]# ./shm_read
    now time: 1470729771
    now time: 1470729772
    now time: 1470729773
    now time: 1470729774
    now time: 1470729775
    now time: 1470729776
    ^C
    [root@dell test]# 

## 3. ��������
### 3.1 ��дDockerfile
    [root@dell test]# cat Dockerfile 
    FROM centos:centos7
    MAINTAINER huangjinqiang "huangjq@chinatelecom.cn"
    COPY ./shm_write /bin/
    COPY ./shm_read  /bin/

### 3.2 Build����
    [root@dell test]# docker build -t shm ./
    Sending build context to Docker daemon 39.42 kB
    Step 1 : FROM centos:centos7
    centos7: Pulling from library/centos
    3d8673bd162a: Pull complete 
    Digest: sha256:a66ffcb73930584413de83311ca11a4cb4938c9b2521d331026dad970c19adf4
    Status: Downloaded newer image for centos:centos7
     ---> 970633036444
    Step 2 : MAINTAINER huangjinqiang "huangjq@chinatelecom.cn"
     ---> Running in 5229a2a5855e
     ---> 91fdde36af8f
    Removing intermediate container 5229a2a5855e
    Step 3 : COPY ./shm_write /bin/
     ---> 5fc8c36de0a0
    Removing intermediate container 91650043ea38
    Step 4 : COPY ./shm_read /bin/
     ---> 0f39ec149638
    Removing intermediate container 789b377b57bc
    Successfully built 0f39ec149638

### 3.3 ��龵��

    [root@dell test]# docker images |grep shm
    shm             latest         0f39ec149638        11 minutes ago      196.8 MB

## 4. ����
### 4.1 ����
#### 4.1.1 ����д��

    [root@dell test]# docker run -i -t shm:latest /bin/bash
    [root@e6c86be87be6 /]# ipcs
    
    ------ Message Queues --------
    key        msqid      owner      perms      used-bytes   messages    
    
    ------ Shared Memory Segments --------
    key        shmid      owner      perms      bytes      nattch     status      
    
    ------ Semaphore Arrays --------
    key        semid      owner      perms      nsems     
    
    [root@e6c86be87be6 /]#

ִ��д

    [root@e6c86be87be6 /]# /bin/shm_write&
    loop .. 
    [root@e6c86be87be6 /]# ipcs
    
    ------ Message Queues --------
    key        msqid      owner      perms      used-bytes   messages    
    
    ------ Shared Memory Segments --------
    key        shmid      owner      perms      bytes      nattch     status      
    0x083dd06c 0          root       600        4096       0                       
    0x083dbfd1 32769      root       600        4096       1                       
    
    ------ Semaphore Arrays --------
    key        semid      owner      perms      nsems     
    
    [root@e6c86be87be6 /]# 


#### 4.1.2 ����
��������һ������

    [root@dell ~]# docker run --ipc=container:e6c86be87be6 -i -t shm:latest /bin/bash

���ipcs���ᷢ�ִ����Ķ������ʵ������Ĺ����ڴ��key��д��ʵ������Ĺ����ڴ��key��һ�µģ���������������ִ�е���ͬһ�鹲���ڴ档����**--ipc=container:e6c86be87be6**ѡ������ؼ��ģ�����e6c86be87be6�ǶԶ�ʵ����id, Ҳ������ʵ�����ƴ��档

    [root@414efed7add6 /]# ipcs
    
    ------ Message Queues --------
    key        msqid      owner      perms      used-bytes   messages    
    
    ------ Shared Memory Segments --------
    key        shmid      owner      perms      bytes      nattch     status      
    0x083dd06c 0          root       600        4096       0                       
    0x083dbfd1 32769      root       600        4096       1                       
    
    ------ Semaphore Arrays --------
    key        semid      owner      perms      nsems     
    
    [root@414efed7add6 /]# 

ִ�ж�ʱ��

    [root@414efed7add6 /]# /bin/shm_read
    now time: 1470730373
    now time: 1470730374
    now time: 1470730375
    now time: 1470730376
    now time: 1470730377
    now time: 1470730378
    now time: 1470730379
    now time: 1470730380
    now time: 1470730381
    ^C
    [root@414efed7add6 /]# 




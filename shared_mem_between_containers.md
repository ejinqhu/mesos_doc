# 基于Docker不同容器实例间的共享内存示例
------

## 1.背景
由于某些已有的程序已经使用了共享内存的方式进行进程间的通信，而Docker一般是将某一程序进行封装独立于其他进程运行，为了解决这个矛盾，Docker引入了--ipc选项来支持Docker容器间的可以继续使用共享内存的方式，从而减少了已有程序向Docker容器迁移的改动。
本文档先提供普通的共享内存程序，一个是shm_write,往共享内存里面写当前时间； 一个是shm_read，从共享内存读出当前时间，方便使用docker logs进行验证。然后创建一个最简单的Dockerfile，然后build出镜像，运行测试。

## 2. 共享内存代码
### 2.1 写数据
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

### 2.2 读数据

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

### 2.3 编译

    [root@dell test]# gcc shm_write.c -o shm_write
    [root@dell test]# gcc shm_read.c -o shm_read

### 2.4 运行

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

## 3. 创建镜像
### 3.1 编写Dockerfile
    [root@dell test]# cat Dockerfile 
    FROM centos:centos7
    MAINTAINER huangjinqiang "huangjq@chinatelecom.cn"
    COPY ./shm_write /bin/
    COPY ./shm_read  /bin/

### 3.2 Build镜像
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

### 3.3 检查镜像

    [root@dell test]# docker images |grep shm
    shm             latest         0f39ec149638        11 minutes ago      196.8 MB

## 4. 测试
### 4.1 运行
#### 4.1.1 启动写端

    [root@dell test]# docker run -i -t shm:latest /bin/bash
    [root@e6c86be87be6 /]# ipcs
    
    ------ Message Queues --------
    key        msqid      owner      perms      used-bytes   messages    
    
    ------ Shared Memory Segments --------
    key        shmid      owner      perms      bytes      nattch     status      
    
    ------ Semaphore Arrays --------
    key        semid      owner      perms      nsems     
    
    [root@e6c86be87be6 /]#

执行写

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


#### 4.1.2 检查读
重新启动一个镜像：

    [root@dell ~]# docker run --ipc=container:e6c86be87be6 -i -t shm:latest /bin/bash

检查ipcs，会发现创建的读的这个实例里面的共享内存的key和写的实例里面的共享内存的key是一致的，表明这两个容器执行的是同一块共享内存。其中**--ipc=container:e6c86be87be6**选项是最关键的，其中e6c86be87be6是对端实例的id, 也可以用实例名称代替。

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

执行读时间

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




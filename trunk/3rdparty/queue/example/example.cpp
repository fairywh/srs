#include <string>
#include "cirmem.h"



int main(int argc, char** argv) {
    std::string cmd;
    unsigned int session_id = 1;
    if(argc <= 1) {
        printf("please input cmd\n");
        printf("./exampled init\n");
        printf("./exampled setsize [size](<=100000)\n");
        printf("./exampled producer [message]\n");
        printf("./exampled consumer\n");
        printf("./exampled producer_batch [message]\n");
        printf("./exampled consumer_batch\n");
        exit(0);
    } else {
        cmd = argv[1];
    }

    cir_mem queue;

    if(cmd == "init") {
        int ret = queue.Create(0x123456, 1000 * 1000);
        if(ret != 0) {
            printf("create shm error,ret=%d\n", ret);
            return 0;
        }
    } else if(cmd == "setsize") {
        if(argc <= 2) {
            printf("please input cmd\n");
            printf("./exampled init\n");
            printf("./exampled setsize [size](<=100000)\n");
    	    printf("./exampled producer [message]\n");
    	    printf("./exampled consumer\n");
    	    printf("./exampled producer_batch [message]\n");
	        printf("./exampled consumer_batch\n");
    	    
    	    exit(0);
        }
        int queue_size = atoll(argv[2]);
        if( queue_size > 0) {
            int ret = queue.Init(0x123456,queue_size);
            if(ret != 0) {
                printf("change shm size error,ret=%d\n", ret);
                return 0;
            }
        }
    } else if(cmd == "producer") {
        if(argc <= 2) {
            printf("please input cmd\n");
            printf("./exampled init\n");
            printf("./exampled setsize [size](<=100000)\n");
    	    printf("./exampled producer [message]\n");
    	    printf("./exampled consumer\n");
    	    printf("./exampled producer_batch [message]\n");
	        printf("./exampled consumer_batch\n");
    	    exit(0);
        }

        int ret = queue.Init(0x123456);
        if(ret != 0) {
            printf("init shm error,ret=%d\n", ret);
            return 0;
        }
        ret = queue.push(argv[2], strlen(argv[2]));
        if(ret != 0) {
            printf("queue is full, ret=%d\r\n", ret);
        } else {
            
        }
    } else if(cmd == "consumer") {
        char result[1024] = {0};
        uint64_t size = 0;
        int ret = queue.Init(0x123456);
        if(ret != 0) {
            printf("init shm error,ret=%d\n", ret);
            return 0;
        }
        ret = queue.pop(result, size);

        if(ret != 0) {
            printf("get no message, ret=%d\r\n", ret);
        } else {
            printf("get message, %s\r\n", result);
        }
    } else if(cmd == "producer_batch") {
        if(argc <= 2) {
            printf("please input cmd\n");
            printf("./exampled init\n");
            printf("./exampled setsize [size](<=100000)\n");
    	    printf("./exampled producer [message]\n");
    	    printf("./exampled consumer\n");
    	    printf("./exampled producer_batch [message]\n");
	        printf("./exampled consumer_batch\n");
    	    exit(0);
        }
        char result[1024] = {0};
        int ret = queue.Init(0x123456);
        if(ret != 0) {
            printf("init shm error,ret=%d\n", ret);
            return 0;
        }
        while(1) {
            unsigned int current_time = time(NULL);
            snprintf(result, sizeof(result), "%s-%u", argv[2], current_time);

            
            ret = queue.push(result, strlen(result));
            if(ret != 0) {
                printf("queue is full,ret=%d\r\n", ret);
                usleep(100);
            } else {
                
            }
        }

        
    } else if(cmd == "consumer_batch") {
        char result[1024] = {0};
        uint64_t size = 0;
        int ret = queue.Init(0x123456);
        if(ret != 0)
        {
            printf("init shm error,ret=%d\n", ret);

            return 0;
        }
        while(1) {
            if (queue.get_curr_nr() == 0) {
                continue;
            }
            ret = queue.pop(result, size);

            if(ret != 0) {
                printf("get no message, ret=%d\r\n", ret);
            } else {
                printf("get message, %s\r\n", result);
            }
        }
    } else if ( cmd == "show") {
        if(argc <= 2) {
            printf("please input cmd\n");
            printf("./exampled init\n");
            printf("./exampled setsize [size](<=100000)\n");
    	    printf("./exampled producer [message]\n");
    	    printf("./exampled consumer\n");
    	    printf("./exampled producer_batch [message]\n");
	        printf("./exampled consumer_batch\n");
    	    printf("./exampled show [size]\n");
    	    exit(0);
        }
        int size = atoll(argv[2]);
        int ret = queue.Init(0x123456);
        if(ret != 0) {
            printf("init shm error,ret=%d\n", ret);
            return 0;
        }
        queue.PrintMem(0, size);
        
    } else {
        printf("please input cmd\n");
        printf("./exampled init\n");
        printf("./exampled setsize [size](<=100000)\n");
	    printf("./exampled producer [message]\n");
	    printf("./exampled consumer\n");
	    printf("./exampled producer_batch [message]\n");
        printf("./exampled consumer_batch\n");
    }
}


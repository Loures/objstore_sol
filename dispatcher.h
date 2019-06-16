#ifndef OS_DISPATCHER_H

    extern pthread_t dispatcher_thread;

    void *dispatch(void *arg);

    #define OS_DISPATCHER_H

#endif
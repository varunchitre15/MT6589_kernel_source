#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/socket.h>

#ifdef _MTK_ENG_
#include "../bionic/fdleak_debug_common.h"

extern int  __close(int);
extern int  __pipe(int*);
extern int  __pipe2(int *, int);
extern int  __socket(int, int, int);
extern int  __accept(int, struct sockaddr *, socklen_t *);
extern int  __dup(int);
extern int  __dup2(int, int);
extern int  __eventfd(unsigned int initval, int flags);

FDLeak_Record_Backtrace fdleak_record_backtrace = NULL;
FDLeak_Remove_Backtrace fdleak_remove_backtrace = NULL;


int close(int fd) {
    if (fdleak_remove_backtrace){
        fdleak_remove_backtrace(fd);
    }
    return __close(fd);
}

int pipe(int *pipefds) {
    int ret;

    ret = __pipe(pipefds);
    if (fdleak_record_backtrace && (ret == 0)){
        fdleak_record_backtrace(pipefds[0]);
        fdleak_record_backtrace(pipefds[1]);
    }
    return ret;
}

#ifdef _GNU_SOURCE  /* GLibc compatibility */
int pipe2(int *pipefds, int flag) {
    int ret;
    
    ret = __pipe2(pipefds, flag);
    if (fdleak_record_backtrace && (ret == 0)){
        fdleak_record_backtrace(pipefds[0]);
        fdleak_record_backtrace(pipefds[1]);
    }
    return ret;    
    
}
#endif

int socket(int domain, int type, int protocol) {
    int fd;
    
    fd = __socket(domain, type, protocol);
    if (fdleak_record_backtrace) {
        fdleak_record_backtrace(fd);
    }
    return fd;
}

int accept(int serverfd, struct sockaddr *addr, socklen_t *addrlen) {
    int fd;
    
    fd = __accept(serverfd, addr, addrlen);
    if (fdleak_record_backtrace) {
        fdleak_record_backtrace(fd);
    }
    return fd;
}

int dup(int oldfd) { 
    int newfd = 0;
    newfd = __dup(oldfd);
    if (fdleak_record_backtrace) {
        fdleak_record_backtrace(newfd);
    }
    return newfd;   
}

int dup2(int oldfd, int targetfd) {
    int newfd = 0;
    
    //LCH need care targetfd, due to dup2 sys call will check targetfd open or not, if open will close it first.
    //mask here for fdleak_remove_backtrace overwriten
    //if (fdleak_remove_backtrace) {
    //    fdleak_remove_backtrace(targetfd);                
    //}    
    newfd = __dup2(oldfd, targetfd);
    if (fdleak_record_backtrace) {
        fdleak_record_backtrace(newfd);                
    }
    return newfd;   
}

int eventfd(unsigned int initval, int flags)
{
    int fd = __eventfd(initval, flags);
    if (fdleak_record_backtrace) {
        fdleak_record_backtrace(fd);
    }
    return fd;    
}
#else
//user load user original path through modify libc/Android.mk
int close(int fd) {
    return __close(fd);
}

int pipe(int *pipefds) {
    return __pipe(pipefds);
}

#ifdef _GNU_SOURCE  /* GLibc compatibility */
int pipe2(int *pipefds, int flag) {
    return __pipe2(pipefds, flag);
}
#endif

int socket(int domain, int type, int protocol) {
    return __socket(domain, type, protocol);
}

int accept(int serverfd, struct sockaddr *addr, socklen_t *addrlen) {
    return __accept(serverfd, addr, addrlen);
}

int dup(int oldfd) { 
    return __dup(oldfd);  
}

int dup2(int oldfd, int targetfd) {
    return __dup2(oldfd, targetfd);
}

int eventfd(unsigned int initval, int flags)
{
    return __eventfd(initval, flags);    
}
#endif

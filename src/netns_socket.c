
#include <linux/sched.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "netns_socket.h"

int create_socket_netns(int ns_fd)
{
    int orig_netns;
    int newfd;
    // save origin netns
    orig_netns = open("/proc/self/ns/net", O_RDONLY);
    if(orig_netns != 0)
    {
        printf("Open self netns failed!\n");
        exit(1);
    }

    //create socket in dst netns
    if(setns(ns_fd, CLONE_NEWNET) == -1)
    {
        printf("Set dest netns failed!\n");
        exit(1);
    }

    newfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    //back to origin netns
    if(setns(orig_netns, CLONE_NEWNET) == -1)
    {
        printf("Back to origin netns failed!\n");
        exit(1);
    }
    close(orig_netns);
    return newfd;
}
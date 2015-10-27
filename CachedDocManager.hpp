#ifndef CACHEDDOCMANAGER_HPP
#define CACHEDDOCMANAGER_HPP

#define FETCH_TYPE 1
#define REQUEST_TYPE 0
#define MAX_NODE 10
#define SIZE 10

#include <string>
#include <cstdlib>
#include <sys/socket.h>
#include <unistd.h> 
#include <map>
#include <iostream>
#include <fstream>

// struct to store file name and host name from http request
using namespace std;
struct info{
    char file[1024];
    char host[1024];
};
// struct to store parameters of each cache block
struct cache_block{
    int last;
    int inUse;  // count this cached_block is serving how many clients
    int expr;
    string expr_date;
    string host_file;
};

// struct to store paremeters of each request
struct parameters{
    int expires;
    int length;
    int cb;  // cache_block index
    char filename[200];
    int offset;
    bool del;
    bool conditional;
};
class CachedDocManager
{
public:
    map<int,int> requester;	// map to store requester of current request, socket_fd : socket_fd
    map<int,int> fetcher;	// map to store fetcher of current requester
    map<int,int> type;		// map to store type of socket : requester/fetcher
    map<int,struct parameters> req_paras;	// map to store progress of request and important parameters like cache block assigned
    map<string,int> whichBlock;	// map to store which cache block belongs to current URL
    map<int,string> request;	// map to store the requested url of current client
    map<int,bool> checkRand;	// map to check if random number generated already exists
    // declaring cache 
    struct cache_block cache[MAX_NODE];
    CachedDocManager();
    ~CachedDocManager();
    
    // methods
    void bringToFront(int block);
    int  getFreeBlock();
    int checkCache(string host_file);
    bool isExpired(int cb);
    struct info * parse_http_request(char *buf, int num_bytes);
    void sendPartOfFileToRequester( int requester_sockfd, fd_set &write_fds, fd_set &master, char * shared_charbuf);
};

#endif // CACHEDDOCMANAGER_HPP

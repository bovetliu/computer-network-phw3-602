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
#include <sstream>
#include <algorithm>

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



// start writing all new map


struct LRU_node{
    string node_name;  // node_name has the same name with the file name, which stores body
    string domain_name;
    string page_name;  
    int expr_time;           // if this is not 0, this has been downloaded one time, if expired then send conditional-get, if not, server
    int last_client_access;  // if this is 0, this is new node
    int web_sock_fd;         // the web_sock_fd responsible for filling it
    map<int, int> req_readpointer_map;    
}

class CachedDocManager
{
public:
    map<int,int> req_sockfd;	// map to store req_sockfd of current request, socket_fd : socket_fd
    map<int,int> fetcher;	// map to store fetcher of current req_sockfd
    map<int,int> type_of;		// map to store req_sockfd of socket : req_sockfd/fetcher
    map<int,struct parameters> req_paras;	// map to store progress of request and important parameters like cache block assigned
    map<string,int> whichBlock;	// map to store which cache block belongs to current URL
    map<int,string> request;	// map to store the requested url of current client
    map<int,bool> checkRand;	// map to check if random number generated already exists
    
    
    // Bowei: I will manly manage this page_to_node_map to realize LRU cache
    map<string, struct LRU_node> page_to_node_map;
    map<int, struct LRU_node> webfd_to_node_map;  
    map<int, struct LRU_node> clifd_map  // mainly for write_fds
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
    void analyzeHeaderOfFile( char * filename, bool & has_304, bool & has_expiration_header, string & expiration_str );
    
    
    struct LRU_node * allocOneNode( string request, int client_sock_fd);
    void prepareAdaptiveRequestForWeb(struct LRU_node * p_lru_node, int client_sock_fd, char * shared_buf);
    
    bool isExpiredTime(int inputtime);
    
    string castNumberToString( int number){
        stringstream temp_sstream;
        temp_sstream << number;
        return temp_sstream.str();
    }
};

#endif // CACHEDDOCMANAGER_HPP

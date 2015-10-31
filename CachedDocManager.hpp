#ifndef CACHEDDOCMANAGER_HPP
#define CACHEDDOCMANAGER_HPP

#define FETCH_TYPE 1
#define REQUEST_TYPE 0
#define MAX_NODE 10
#define SIZE 10
#include <cstring>
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

// start writing all new map


struct LRU_node{
    string node_name;  // node_name has the same name with the file name, which stores body
    string domain_name;
    string request;
    string page_name;  
    int expr_time;           // if this is not 0, this has been downloaded one time, if expired then send conditional-get, if not, server
    string expr_date;
    int last_client_access;  // if this is 0, this is new node
    int web_sock_fd;         // the web_sock_fd responsible for filling it
    int in_use;  //every time when it was allocated to one request it should add by one
    map<int, int> req_readpointer_map;    
};

class CachedDocManager
{
private:
    void evictOldestLastAccess ();
public:

    map<int,int> type_of;		// map to store req_sockfd of socket : req_sockfd/fetcher
    // Bowei: I will mainly manage this page_to_node_map to realize LRU cache
    map<string, struct LRU_node> page_to_node_map;
    map<int, struct LRU_node *> webfd_map;  
    map<int, struct LRU_node *> clifd_map;  // mainly for write_fds

    CachedDocManager();
    ~CachedDocManager();
    
    // methods
    struct info * parse_http_request(const char *buf, int num_bytes);
    void sendPartOfFileToRequester( int requester_sockfd, fd_set &write_fds, fd_set &master, char * shared_charbuf);
    void analyzeHeaderOfFile( const char * filename, bool & has_304, bool & has_expiration_header, string & expiration_str );
    
    
    struct LRU_node * allocOneNode( string request, struct info* temp_info,  int client_sock_fd);
    void prepareAdaptiveRequestForWeb(struct LRU_node * p_lru_node, int client_sock_fd, char * shared_buf);
    
    bool isExpiredTime(int inputtime);
    
    // one inline method
    string castNumberToString( int number){
        stringstream temp_sstream;
        temp_sstream << number;
        return temp_sstream.str();
    }
    void addReqSocketsOfNodeToWFD(struct LRU_node & target_nd, fd_set & write_fds, fd_set & master, int current_sock_fd);

    
};

#endif // CACHEDDOCMANAGER_HPP

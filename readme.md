# ECEN 602 Programming hw 3, proxy parts

I use codelite to build this. 
To run server: 
$ ./bowei_proxy 127.0.0.1 3000

One can use correctly implemented client to test,

$ ./client 127.0.0.1 3000 http://www.cplusplus.com/info/

The reason I choose http://www.cplusplus.com/info/ is that the server can do conditional get
The server can do Condition-GET, expiration check. 

brief architecture of this code
There is not much to say about client.
proxy has two dependency classes, one is Utility, the other one is CachedDocManager. Utility class holds mainly static utility methods, CachedDocManager is to help the main() method to manage the LRU_nodes. 

For CachedDocManager the core properties are following:
```cpp
map<int,int> type_of; // tell if one file descript is REQUEST type or FETCH type, REQUEST type is representing a GET request from client.
map<string, struct LRU_node> page_to_node_map;
map<int, struct LRU_node *> webfd_map;  
map<int, struct LRU_node *> clifd_map;  // mainly for write_fds
```    
and LRU_node
```cpp
struct LRU_node{
    string node_name;  // node_name has the same name with the file name, which stores body
    string domain_name;
    string request;
    string page_name;  
    int expr_time;           // if this is not 0, this has been downloaded one time, if expired then send conditional-get, if not, server
    string expr_date;
    int last_client_access;  // if this is 0, this is new node
    int web_sock_fd;         // the web_sock_fd responsible for filling it
    int in_use;  //every time when it was allocated to one request it should add by one, finished transfer be substracted by 1
    map<int, int> req_readpointer_map;   // important component of LRU_node  
};
```

`page_to_node_map` stores `LRU_node` keyed by requests URL, the `CachedDocManager` only keeps 10 URLs and its file cached.   
`webfd_map` linking web sockets to `LRU_node` which is stored in  `page_to_node_map`.   
The similar things apply to `clifd_map`, (client file descriptor map). When content of `LRU_node` is ready to be sent back to client.  The cient socket in key set of `req_readpointer_map` will be added to `write_fds` set, then the program can quickly find corresponding `LRU_node` using `clifd_map`.   


Every `LRU_node` has one `req_readpointer_map`, it is a `map<int,int>`, which is actually is mapping `<socket, file pos>`.   
The `req_readpointer_map` has three usages,   
1. use `req_readpointer_map.size()` to check how many clients are on this `LRU_node`.  
2. use file pos to track sending progress of each requesting client.  
3. `size() == 0` means the LRU_node is idle, eviction method: `evictOldestLastAccess()`  will use it.   

Main logic is like following:
```cpp
for(i = 0; i <= fdmax; i ++){
	if( FD_ISSET(i,&write_fds) ){ 
		//send part of file to request.
	} else if(FD_ISSET(i,&read_fds)){
		if(i == server_conn_info.sockfd){ 
			// accepting new request from client
		} else {
			if((num_bytes = recv(i,buf,sizeof(buf),0)) <=0) {
				if(cached_doc_mnger.type_of[i]== FETCH_TYPE){
					// handle end of web server transfer,
					// analyze header
					// updating cached files on disk if necessary
					// cached_doc_mnger.addReqSocketsOfNodeToWFD(...)  using webfd_map[i] to get LRU_node
				} else {  // handling REQUEST_TYPE
					// client stoped GET request transfering
				}
			} else{ 
				if(cached_doc_mnger.type_of[i] == FETCH_TYPE){
					// web server sent new contents of some file, updating temp_suffixed file of LRU_node
				} else {  // handling REQUEST_TYPE
					// handling request from client
					// prepare adaptive request according to cached file hitting situation
					// fetching new html doc is necessary				
				}
			}
		}
	}
}
```


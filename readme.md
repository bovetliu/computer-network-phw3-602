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
proxy has two dependency classes, one is Utility, the other one is CachedDocManager.
Utility is basically one static class(all methods are static), CachedDocManager is to help main() manage the LRU_nodes. 

For CachedDocManager the core properties are following:

    map<string, struct LRU_node> page_to_node_map;
    
    map<int, struct LRU_node *> webfd_map;  
    
    map<int, struct LRU_node *> clifd_map;  // mainly for write_fds
    

page_to_node_map stores LRU_node keyed by requests URL, the CachedDocManager only keeps 10 URL and its file cached. 
webfd_map linking web sockets with LRU_node is page_to_node_map. So every time when one of web sockets is ready, 
programming can use this websocket to quickly find the corresponding LRU_node, and do relevant file operation(writing)

The similar things apply to clifd_map, (client file descriptor map), when LRU_node is ready to send back to client. 
the cient socket will be added to "read_fds" set, then programming can quickly find corresponding LRU_node, when client socket
is ready to be written.


Every LRU_node has one req_readpointer_map: map<int,int>, which is actually is map<socket, file pos>.
The req_readpointer_map has three usage, 
1. use size() to check how many clients are on this LRU_node.
2. use file pos to track sending progress
3. size() == 0 means the LRU_node is idle, eviction method: evictOldestLastAccess  will use it.


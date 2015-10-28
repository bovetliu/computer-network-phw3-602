# ECEN 602 Programming hw 3, proxy parts

I use codelite to build this. 
To run server: 
$ ./bowei_proxy 127.0.0.1 3000

One can use correctly implemented client to test,

$ ./client 127.0.0.1 3000 http://www.cplusplus.com/info/

The reason I choose http://www.cplusplus.com/info/ is that the server can do conditional get
The server can do Condition-GET, expiration check. 
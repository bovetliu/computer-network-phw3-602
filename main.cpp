#include "Utility.hpp"
#include "CachedDocManager.hpp"

#include <sstream>

#define STDIN 0

// Thanks BEEJ.US for tutorial on how to use SELECT and pack and unpack functions
// Some parts of the code have been taken from BEEJ.US

using namespace std;
unsigned short int client_count; // global variable to store client count

int MAXCLIENTS;
int BACKLOG;

int main(int argc, char *argv[]){

    //char ipstr[INET_ADDRSTRLEN];		//INET6_ADDRSTRLEN for IPv6
    int error;
    connection_info server_conn_info;
    Utility::initialize_server(argc, argv, server_conn_info);
    CachedDocManager cached_doc_mnger;


    // Initializing the required variables
    char buf[2048],time_buf[256],tmpname[512];
    char web_port[3] = {'8','0','\0'};
    fd_set master, read_fds, write_fds; // Sets for Select Non-Blocking I/O
    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_SET(server_conn_info.sockfd,&master);

    int fdmax = server_conn_info.sockfd, c_sockfd, i, num_bytes;

    struct sockaddr_storage client_addr;
    socklen_t addr_len;

    client_count = 0;
    //initCache();

    struct tm temptime,*utc;
    time_t rawtime;

    while(1){
        read_fds = master; 
        if( (error = select(fdmax+1, &read_fds, &write_fds, NULL, NULL)==-1)){
            perror("server: select");
            exit(4);
        }
        for(i = 0; i <= fdmax; i ++){
            if( FD_ISSET(i,&write_fds) ){ // Write to file descriptor, the only possiblity that proxy is going to write, is to write to clients
                
                if( FD_ISSET(i,&master) == false){ // Handle case when client disconnects in middle of transfer
                    cout << "SERVER: Client "<< i << ": Disconnected in middle of transfer" << endl;
                    FD_CLR(i,&master);
                    FD_CLR(i,&write_fds);
                    close(i);
                    if(cached_doc_mnger.req_paras[i].cb==-1){
                        if(cached_doc_mnger.req_paras[i].del==true)
                            remove(cached_doc_mnger.req_paras[i].filename);	// Delete the tmp file
                    }
                    else if(cached_doc_mnger.req_paras[i].cb != -1){
                        cached_doc_mnger.bringToFront(cached_doc_mnger.req_paras[i].cb);
                        cached_doc_mnger.cache[cached_doc_mnger.req_paras[i].cb].inUse-=1;		// Decrease the in Use counter by 1
                    }
                    continue;
                }
                // the socket_fd for client can be in both master and write_fds
                cached_doc_mnger.sendPartOfFileToRequester(i, write_fds, master, buf);
                
            }
            else if(FD_ISSET(i,&read_fds)){ 
                if(i == server_conn_info.sockfd){  // handle new clients here
                    addr_len = sizeof(client_addr);		// handle new clients here
                    if( (c_sockfd = accept(server_conn_info.sockfd, (struct sockaddr *)&client_addr,&addr_len) ) == -1){	// accept a new connection
                        perror("server: accept");
                    }
                    else{
                        FD_SET(c_sockfd,&master);
                        cached_doc_mnger.type_of[c_sockfd] = REQUEST_TYPE;   // cached_doc_mnger.type_of is a map<int, int> used to store req_sockfd or cached_doc_mnger.fetcher,
                        client_count++; // increase client count
                        if(c_sockfd > fdmax)  // updating fdmax
                            fdmax = c_sockfd;
                        cout <<endl << endl << "=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-" << endl;
                        cout << "SERVER: New Client at Socket: " << c_sockfd << endl;
                    }
                }
                else{  // so now,  we all processig, web sockfds, all i here, should be web sockfds
                    if((num_bytes = recv(i,buf,sizeof(buf),0)) <=0){
                        if(num_bytes == 0){ // If received bytes are zero, it means client has disconnected, so remove its allocated resources
                        }
                        else{
                            perror("server: recv");
                        }
                        close(i);
                        FD_CLR(i,&master);
                        
                        if(cached_doc_mnger.type_of[i]== FETCH_TYPE){  // web server closed connection, needs to send file back now
                            FD_SET(cached_doc_mnger.req_sockfd[i],&write_fds);// if it was a cached_doc_mnger.fetcher(web server closed tcp now), then start sending back to the req_sockfd
                            
                            int initiating_client_socket = cached_doc_mnger.req_sockfd[i];
                            int bb = cached_doc_mnger.req_paras[initiating_client_socket].cb;
                            if(bb==-1){
                                // Got No Block return from tmp file
                            }
                            else{
                                // Copy tmp file to cache block and serve from cache block
                                string expiration_str;
                                bool has_304 = false;
                                bool has_expiration_header = false;
                                cached_doc_mnger.analyzeHeaderOfFile(cached_doc_mnger.req_paras[initiating_client_socket].filename,
                                                                    has_304, has_expiration_header,expiration_str);
                                memset(&temptime, 0, sizeof(struct tm));
                                
                                if (has_expiration_header){  // means expiration_str has value
                                    //strptime: convert  a  string  representation  of time to a time tm structure
                                    strptime(expiration_str.c_str(), "%a, %d %b %Y %H:%M:%S ", &temptime);
                                    cached_doc_mnger.cache[bb].expr = mktime(&temptime);
                                    cached_doc_mnger.cache[bb].expr_date = expiration_str;
                                } else {  // does not has expiration header, 
                                    time(&rawtime);
                                    utc = gmtime(&rawtime);
                                    strftime(time_buf, sizeof(time_buf), "%a, %d %b %Y %H:%M:%S ",utc);	// If Expires field was not present, set the current time as cache block expire time
                                    cached_doc_mnger.cache[bb].expr = mktime(utc);
                                    cached_doc_mnger.cache[bb].expr_date = string(time_buf) + string("GMT");
                                }

                                time(&rawtime);
                                utc = gmtime(&rawtime);
                                strftime(time_buf, sizeof(time_buf), "%a, %d %b %Y %H:%M:%S ",utc);
                                cout << "SERVER: Proxy Server Time: " << time_buf << "GMT" << endl;
                                cout << "SERVER: File Expires Time: " << cached_doc_mnger.cache[bb].expr_date << endl;

                                if(has_304){
                                    cout << "External server returned 304, no need to download" << endl;
                                    if(cached_doc_mnger.req_paras[initiating_client_socket].conditional){	// if 304 response comes, serve from Cache Block and delete the tmp file
                                        remove(cached_doc_mnger.req_paras[initiating_client_socket].filename);

                                        strcpy(tmpname,Utility::castNumberToString(bb).c_str());
                                        strcpy(cached_doc_mnger.req_paras[initiating_client_socket].filename,tmpname);									
                                        cached_doc_mnger.req_paras[initiating_client_socket].del = false;
                                    }
                                }
                                else { // If it is not 304 response, see if we can write to the cache block assigned, or get a new cache block
                                    if(cached_doc_mnger.cache[bb].inUse > 1) {
                                        cached_doc_mnger.cache[bb].inUse -= 1;
                                        int new_cb;
                                        new_cb = cached_doc_mnger.getFreeBlock();
                                        if(new_cb==-1){
                                            cached_doc_mnger.req_paras[initiating_client_socket].del = true;
                                            cached_doc_mnger.req_paras[initiating_client_socket].cb = -1;
                                            bb = -1;
                                            cout << "SERVER: Client " << i << ": Could Not find new free block" << endl;
                                        }
                                        else{
                                            cout << "SERVER: Client" << i << ": Got new block because earlier was in use and we got a 200" << endl;
                                            cached_doc_mnger.req_paras[initiating_client_socket].cb = new_cb;
                                            cached_doc_mnger.cache[new_cb].expr = cached_doc_mnger.cache[bb].expr;
                                            cached_doc_mnger.cache[new_cb].expr_date = cached_doc_mnger.cache[bb].expr_date;
                                            bb = new_cb;
                                        }
                                    
                                    }
                                    else{
                                        cached_doc_mnger.cache[bb].inUse = -2;
                                        //cout << "SOCKET: " << i << " ==> " << "Cache Block is free for writing" << endl;
                                    }
                                }
                                // has vaild bb and not in Use
                                if(bb!=-1 && cached_doc_mnger.cache[bb].inUse==-2) // Prepare the cache block and the parameters, remove the tmp file
                                {
                                    strcpy(tmpname,  Utility::castNumberToString(bb).c_str() );
                                    ofstream myfile4;
                                    myfile4.open(tmpname,ios::out | ios::binary);
                                    myfile4.close();
                                    remove(tmpname);
                                    rename(cached_doc_mnger.req_paras[initiating_client_socket].filename,tmpname);
                                    strcpy(cached_doc_mnger.req_paras[initiating_client_socket].filename,tmpname);
                                    cached_doc_mnger.cache[bb].host_file = string(cached_doc_mnger.request[cached_doc_mnger.req_sockfd[i]]);
                                    cached_doc_mnger.whichBlock[cached_doc_mnger.cache[bb].host_file] = bb;
                                    cached_doc_mnger.req_paras[initiating_client_socket].del = false;
                                    cached_doc_mnger.cache[bb].inUse = 1;
                                }
                            }
                        }
                        else{
                            cout << "SERVER: Client " << i << ": Disconnected" << endl;		// check if cached_doc_mnger.req_sockfd disconnected
                        }
                    }
                    // recved num_bytes > 0 situation
                    else if(cached_doc_mnger.type_of[i]== FETCH_TYPE){  
                        cout << "WRITING PARTS to Cache" << endl;
                        ofstream myfile2;		// Fetcher writes the data into a tmp file for sending back to cached_doc_mnger.req_sockfd later
                        myfile2.open(cached_doc_mnger.req_paras[cached_doc_mnger.req_sockfd[i]].filename,ios::out | ios::binary | ios::app);
                        if(!myfile2.is_open()){ 
                            cout << "some error in opening file" << endl;
                        }
                        else{
                            myfile2.write(buf,num_bytes);
                            myfile2.close();
                        }
                    }
                    else if(cached_doc_mnger.type_of[i]== REQUEST_TYPE){  //  this socket_fd is a req_sockfd
                        struct info* temp_request_info;
                        temp_request_info = cached_doc_mnger.parse_http_request(buf,num_bytes);
                        cached_doc_mnger.request[i] = string(temp_request_info->host)+string(temp_request_info->file);
                        cout << "SERVER: Client "<< i << ": GET Request: " << cached_doc_mnger.request[i] << endl;
                        int cb = cached_doc_mnger.checkCache(cached_doc_mnger.request[i]);
                        //cout << "SOCKET: " << i << " ==> " << "First cb" << cb <<endl;
                        bool expired = false;
                        cached_doc_mnger.req_paras[i].conditional = false;
                        if(cb!=-1){
                            if(cached_doc_mnger.cache[cb].inUse>=0){
                                expired = cached_doc_mnger.isExpired(cb);
                                if(expired){
                                    //cout << "SOCKET: " << i << " ==> " << "Expired Cache Block Send Contional Get" << endl;
                                    cached_doc_mnger.req_paras[i].conditional = true;		// Data is stale, so do conditional get
                                }
                                else{
                                    cached_doc_mnger.req_paras[i].cb = cb;		// Cache Hit happened, return data from cache
                                    cached_doc_mnger.req_paras[i].offset = 0;
                                    cached_doc_mnger.req_paras[i].del = false;
                                    cached_doc_mnger.cache[cb].inUse += 1;
                                    stringstream ss;
                                    ss << cb;
                                    strcpy(cached_doc_mnger.req_paras[i].filename,ss.str().c_str());
                                    FD_SET(i,&write_fds);
                                    cout << "SERVER: Client " << i << ": Cache Hit at Block "<< cb << ": **Not STALE**" << endl;
                                    //cout << "SOCKET: " << i << " ==> " << "RETURN FROM CACHE BLOCK" << endl;
                                    continue;
                                }
                            }
                        }
                        if(cb==-1 || expired){
                            string tmp_str = "GET "+string(temp_request_info->file)+" HTTP/1.0\r\nHost: "+string(temp_request_info->host)+"\r\n\r\n";
                            num_bytes = tmp_str.length();
                            strcpy(buf,tmp_str.c_str());
                            if(!expired){
                                //cout << "here" << endl;
                                cb = cached_doc_mnger.getFreeBlock();
                                cout << "SERVER: Client "<< i << ": Cache Miss: Allocated Block " << cb << endl;	// if Not Stale get new cache block
                                //cout <<buf << endl;
                            }
                            else{
                                cached_doc_mnger.cache[cb].inUse+=1;
                                buf[num_bytes-2]='\0';
                                string req = string(buf);
                                string new_req = req+"If-Modified-Since: "+cached_doc_mnger.cache[cb].expr_date +"\r\n\r\n\0";		// If Stale send If-Modified-Since field in the header
                                num_bytes=new_req.length();
                                strcpy(buf,new_req.c_str());
                                //cout << buf << endl;
                                cout << "SERVER: Client " << i << ": Cache Hit at Block "<< cb << ": **STALE** : **Conditional Get**" << endl;
                                cout << "REQ OUT: " << endl << new_req.c_str() << endl;
                            }
                            cached_doc_mnger.req_paras[i].cb = cb;
                            //cout << "SOCKET: " << i << " ==> " << "GOT BLOCK " << cached_doc_mnger.req_paras[i].cb << endl;
                            
                            int new_web_socket = Utility::connect_to_web(temp_request_info->host, web_port);
                            cached_doc_mnger.type_of[new_web_socket]=FETCH_TYPE;
                            cached_doc_mnger.fetcher[i] = new_web_socket;
                            cached_doc_mnger.req_sockfd[new_web_socket] = i;

                            cached_doc_mnger.req_paras[i].del = true; // setting the parameters of the request
                            cached_doc_mnger.req_paras[i].offset = 0;
                            stringstream ss;
                            int rr = Utility::getRandomNumber();		// generate random number for temp_request_info file name
                            while(cached_doc_mnger.checkRand.find(rr) != cached_doc_mnger.checkRand.end() ){
                                rr = (rr+1)%1000000007; // if requests are at same time the rand() will generate same random number so increment by 1 till we get a new number
                            }
                            cached_doc_mnger.checkRand[rr]=true;
                            ss << "tmp_"<< rr;
                            strcpy(cached_doc_mnger.req_paras[i].filename,ss.str().c_str());
                            //cout << "SERVER: Client " << i << " ==> " << "Tmp filename " << cached_doc_mnger.req_paras[i].filename << endl;
                            ofstream touch;
                            touch.open(cached_doc_mnger.req_paras[i].filename,ios::out | ios::binary); // touch the tmp file
                            if(touch.is_open())
                                touch.close();
                            // new_web_socket should be listened
                            FD_SET(new_web_socket,&master);
                            if(new_web_socket > fdmax)
                                fdmax = new_web_socket;

                            //cout << "SOCKET: " << i << " ==> " << "new socket created " << new_web_socket << endl;
                            cout << "SERVER: Fetcher started at socket " << new_web_socket << ": for client " << i << endl;
                            if( (error = send(new_web_socket,buf,num_bytes,0)) == -1){			// send the get or conditional get request
                                perror("client: send");
                            }
                        }
                        free(temp_request_info);
                    }
                }
            }
        }
    }
    return 0;
}

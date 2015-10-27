#include "Utility.hpp"
#include "CachedDocManager.hpp"
#include <algorithm>
#include <sstream>
#include <map>

#define STDIN 0

// Thanks BEEJ.US for tutorial on how to use SELECT and pack and unpack functions
// Some parts of the code have been taken from BEEJ.US

using namespace std;
unsigned short int client_count; // global variable to store client count

int MAXCLIENTS;
int BACKLOG;

int main(int argc, char *argv[]){

    //char ipstr[INET_ADDRSTRLEN];		//INET6_ADDRSTRLEN for IPv6
    int sockfd,error;
    connection_info server_conn_info;
    Utility::initialize_server(argc, argv, server_conn_info);
    CachedDocManager cached_doc_mnger;
    sockfd = server_conn_info.sockfd;

    // Initializing the required variables
    char buf[2048],time_buf[256],tmpname[512];
    char web_port[3] = {'8','0','\0'};
    fd_set master, read_fds, write_fds, master_write; // Sets for Select Non-Blocking I/O
    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_ZERO(&master_write);
    FD_SET(sockfd,&master);

    int fdmax = sockfd, c_sockfd, i, num_bytes;

    struct sockaddr_storage client_addr;
    socklen_t addr_len;

    client_count = 0;
    //initCache();

    struct tm tm,*utc;
    time_t rawtime;

    while(1){
        read_fds = master; // select between file descriptors, select changes the read_fds set so always make a copy from master set
        write_fds = master_write;
        if( (error = select(fdmax+1, &read_fds, &write_fds, NULL, NULL)==-1)){
            perror("server: select");
            exit(4);
        }
        for(i = 0; i <= fdmax; i ++){
            if( FD_ISSET(i,&write_fds) ){ // Write to file descriptor
                if( FD_ISSET(i,&master) == false){ // Handle case when client disconnects in middle of transfer
                    cout << "SERVER: Client "<< i << ": Disconnected in middle of transfer" << endl;
                    FD_CLR(i,&master);
                    FD_CLR(i,&master_write);
                    close(i);
                    if(cached_doc_mnger.gold[i].cb==-1){
                        if(cached_doc_mnger.gold[i].del==true)
                            remove(cached_doc_mnger.gold[i].filename);	// Delete the tmp file
                    }
                    else if(cached_doc_mnger.gold[i].cb != -1){
                        cached_doc_mnger.bringToFront(cached_doc_mnger.gold[i].cb);
                        cached_doc_mnger.cache[cached_doc_mnger.gold[i].cb].inUse-=1;		// Decrease the in Use counter by 1
                    }
                    continue;
                }
                ifstream myfile1;
                myfile1.open( cached_doc_mnger.gold[i].filename ,ios::in | ios::binary); // open the cache file or tmp file
                if( !myfile1.is_open() ){
                    cout << cached_doc_mnger.gold[i].filename << endl;
                    cout << "SOCKET: " << i << " ==> " << "Could Not Open Cache File" << endl;
                    FD_CLR(i,&master_write);
                }
                else{  // opened successfully
                    //seekg:Set position in input sequence
                    myfile1.seekg( (cached_doc_mnger.gold[i].offset)*2000 , ios::beg); // Seek the file with the offset in the parameters of the request and increase the offset by 1
                    cached_doc_mnger.gold[i].offset+=1;
                    myfile1.read(buf,2000);  // Read next 2000 bytes
                    num_bytes = myfile1.gcount();
                    //cout << num_bytes << endl;
                    if(num_bytes!=0){
                        if( (error = send(i,buf,num_bytes,0)) == -1){			// send the message to the server
                                perror("client: send");
                        }
                    }
                    if(num_bytes<2000){						// reached the end of FILE, now close the connection
                        FD_CLR(i,&master_write);
                        FD_CLR(i,&master);			
                        close(i);							// Connection closed with client and removed from master Read and Write sets
                        cout << "SERVER: **DONE** Connection closed to client " << i << ": **DONE**" << endl;
                        if(cached_doc_mnger.gold[i].cb==-1){
                            //cout << "SOCKET: " << i << " ==> " << "Remove Temp File" << endl;
                            if(cached_doc_mnger.gold[i].del==true)
                                remove(cached_doc_mnger.gold[i].filename);	// Delete the tmp file
                        }
                        else if(cached_doc_mnger.gold[i].cb != -1){
                            cached_doc_mnger.bringToFront(cached_doc_mnger.gold[i].cb);
                            cached_doc_mnger.cache[cached_doc_mnger.gold[i].cb].inUse-=1;		// Decrease the in Use counter by 1
                            //cout << "SOCKET: " << i << " ==> " << "Decrease inUse by 1 : " << cache[cached_doc_mnger.gold[i].cb].inUse<<endl;
                        }
                    }
                } // open successfully ends
                myfile1.close();
            }
            else if(FD_ISSET(i,&read_fds)){  // Bowei Liu: this block go to line 616, to the end almost
                if(i==sockfd){  // handle new clients here
                    addr_len = sizeof(client_addr);		// handle new clients here
                    if( (c_sockfd = accept(sockfd, (struct sockaddr *)&client_addr,&addr_len) ) == -1){	// accept a new connection
                        perror("server: accept");
                    }
                    else{
                        FD_SET(c_sockfd,&master);
                        cached_doc_mnger.type[c_sockfd] = REQUEST_TYPE;   // cached_doc_mnger.type is a map<int, int> used to store requester or cached_doc_mnger.fetcher,
                        client_count++; // increase client count
                        if(c_sockfd > fdmax)  // updating fdmax
                            fdmax = c_sockfd;
                        cout << "***************************************************************" << endl;
                        cout << "SERVER: New client connected at Socket: " << c_sockfd << endl;
                    }
                }
                else{
                    if((num_bytes = recv(i,buf,sizeof(buf),0)) <=0){
                        if(num_bytes == 0){ // If received bytes are zero, it means client has disconnected, so remove its allocated resources
                        }
                        else{
                            perror("server: recv");
                        }
                        close(i);
                        FD_CLR(i,&master);
                        
                        if(cached_doc_mnger.type[i]== FETCH_TYPE){  // web server closed connection, needs to send file back now
                            FD_SET(cached_doc_mnger.requester[i],&master_write);// if it was a cached_doc_mnger.fetcher(web server closed tcp now), then start sending back to the requester
                            
                            int initiating_client_socket = cached_doc_mnger.requester[i];
                            int bb = cached_doc_mnger.gold[initiating_client_socket].cb;
                            if(bb==-1){
                                // Got No Block return from tmp file
                            }
                            else{
                                // Copy tmp file to cache block and serve from cache block
                                ifstream myfile3;
                                myfile3.open(cached_doc_mnger.gold[initiating_client_socket].filename,ios::in | ios::binary);
                                string line;
                                bool tof=false;
                                if(myfile3.is_open()){
                                    getline(myfile3,line);
                                    cout << "SERVER: Fetcher "<<i <<": for client "<<cached_doc_mnger.requester[i] << ": " << line << endl;
                                    if(line.find("304")!=string::npos)		// Check for 304 responses
                                        tof = true;
                                    bool flag = false;
                                    while(!myfile3.eof()){
                                        getline(myfile3,line);
                                        if(line.compare("\r") == 0){
                                            break;
                                        }
                                        string line_cp = line;
                                        //cout << line << endl;
                                        transform(line.begin(), line.end(), line.begin(), ::tolower);
                                        if(line.find("expires:")==0){		// Check for the Expires Field
                                            flag = true;
                                            int pos = 8;
                                            if(line[pos]==' ')
                                                pos++;
                                            stringstream s2;
                                            s2 << line.substr(pos);
                                            getline(s2,line);
                                            //cache[bb].expr_date = line_cp.substr(pos,line_cp.find("GMT")-pos-1);
                                            line = line_cp.substr(pos);
                                            cached_doc_mnger.cache[bb].expr_date = line;
                                            //cout << cache[bb].expr_date << endl;
                                            memset(&tm, 0, sizeof(struct tm));
                                            strptime(line.c_str(), "%a, %d %b %Y %H:%M:%S ", &tm);		// Update the Cache Block expires field with received value
                                            cached_doc_mnger.cache[bb].expr = mktime(&tm);
                                            break;
                                        }
                                    }
                                    myfile3.close();
                                    if(!flag){
                                        time(&rawtime);
                                        utc = gmtime(&rawtime);
                                        strftime(time_buf, sizeof(time_buf), "%a, %d %b %Y %H:%M:%S ",utc);	// If Expires field was not present, set the current time as cache block expire time
                                        cached_doc_mnger.cache[bb].expr = mktime(utc);
                                        cached_doc_mnger.cache[bb].expr_date = string(time_buf) + string("GMT");
                                    }
                                }
                                time(&rawtime);
                                utc = gmtime(&rawtime);
                                strftime(time_buf, sizeof(time_buf), "%a, %d %b %Y %H:%M:%S ",utc);
                                cout << "SERVER: **Proxy Server Time**: " << time_buf << "GMT" << endl;
                                cout << "SERVER: **File Expires Time**: " << cached_doc_mnger.cache[bb].expr_date << endl;

                                if(tof==true){
                                    if(cached_doc_mnger.gold[initiating_client_socket].conditional){	// if 304 response comes, serve from Cache Block and delete the tmp file
                                        remove(cached_doc_mnger.gold[initiating_client_socket].filename);
                                        stringstream s4;
                                        s4 << bb;
                                        strcpy(tmpname,s4.str().c_str());
                                        strcpy(cached_doc_mnger.gold[initiating_client_socket].filename,tmpname);									
                                        cached_doc_mnger.gold[initiating_client_socket].del = false;
                                    }
                                }

                                if(tof==false){					// If it is not 304 response, see if we can write to the cache block assigned, or get a new cache block
                                    if(cached_doc_mnger.cache[bb].inUse!=1){
                                        if(cached_doc_mnger.cache[bb].inUse>1){
                                            cached_doc_mnger.cache[bb].inUse-=1;
                                            int new_cb;
                                            new_cb = cached_doc_mnger.getFreeBlock();
                                            if(new_cb==-1){
                                                cached_doc_mnger.gold[initiating_client_socket].del = true;
                                                cached_doc_mnger.gold[initiating_client_socket].cb = new_cb;
                                                bb = new_cb;
                                                cout << "SERVER: Client " << i << ": Could Not find new free block" << endl;
                                            }
                                            else{
                                                cout << "SERVER: Client" << i << ": Got new block because earlier was in use and we got a 200" << endl;
                                                cached_doc_mnger.gold[initiating_client_socket].cb = new_cb;
                                                cached_doc_mnger.cache[new_cb].expr = cached_doc_mnger.cache[bb].expr;
                                                cached_doc_mnger.cache[new_cb].expr_date = cached_doc_mnger.cache[bb].expr_date;
                                                bb = new_cb;
                                            }
                                        }
                                    }
                                    else{
                                        cached_doc_mnger.cache[bb].inUse = -2;
                                        //cout << "SOCKET: " << i << " ==> " << "Cache Block is free for writing" << endl;
                                    }
                                }

                                if(bb!=-1 && cached_doc_mnger.cache[bb].inUse==-2)	// Prepare the cache block and the parameters, remove the tmp file
                                {
                                    //cout << "Still came here" << endl;
                                    stringstream s3;
                                    s3 << bb;
                                    strcpy(tmpname,s3.str().c_str());
                                    ofstream myfile4;
                                    myfile4.open(tmpname,ios::out | ios::binary);
                                    myfile4.close();
                                    remove(tmpname);
                                    rename(cached_doc_mnger.gold[initiating_client_socket].filename,tmpname);
                                    strcpy(cached_doc_mnger.gold[initiating_client_socket].filename,tmpname);
                                    cached_doc_mnger.cache[bb].host_file = string(cached_doc_mnger.request[cached_doc_mnger.requester[i]]);
                                    cached_doc_mnger.whichBlock[cached_doc_mnger.cache[bb].host_file]=bb;
                                    cached_doc_mnger.gold[initiating_client_socket].del = false;
                                    cached_doc_mnger.cache[bb].inUse = 1;
                                }
                            }
                        }
                        else{
                            cout << "SERVER: Client " << i << ": Disconnected" << endl;		// check if cached_doc_mnger.requester disconnected
                        }
                    }
                    else if(cached_doc_mnger.type[i]== FETCH_TYPE){  // num_bytes > 0
                        cout << "WRITING PARTS" << endl;
                        ofstream myfile2;		// Fetcher writes the data into a tmp file for sending back to cached_doc_mnger.requester later
                        myfile2.open(cached_doc_mnger.gold[cached_doc_mnger.requester[i]].filename,ios::out | ios::binary | ios::app);
                        if(!myfile2.is_open()){ 
                            cout << "some error in opening file" << endl;
                        }
                        else{
                            myfile2.write(buf,num_bytes);
                            myfile2.close();
                        }
                    }
                    else if(cached_doc_mnger.type[i]== REQUEST_TYPE){  //  this socket_fd is a requester
                        struct info* temp_request_info;
                        temp_request_info = cached_doc_mnger.parse_http_request(buf,num_bytes);
                        cached_doc_mnger.request[i] = string(temp_request_info->host)+string(temp_request_info->file);
                        cout << "SERVER: Client "<< i << ": GET Request: " << cached_doc_mnger.request[i] << endl;
                        int cb = cached_doc_mnger.checkCache(cached_doc_mnger.request[i]);
                        //cout << "SOCKET: " << i << " ==> " << "First cb" << cb <<endl;
                        bool expired = false;
                        cached_doc_mnger.gold[i].conditional = false;
                        if(cb!=-1){
                            if(cached_doc_mnger.cache[cb].inUse>=0){
                                expired = cached_doc_mnger.isExpired(cb);
                                if(expired){
                                    //cout << "SOCKET: " << i << " ==> " << "Expired Cache Block Send Contional Get" << endl;
                                    cached_doc_mnger.gold[i].conditional = true;		// Data is stale, so do conditional get
                                }
                                else{
                                    cached_doc_mnger.gold[i].cb = cb;		// Cache Hit happened, return data from cache
                                    cached_doc_mnger.gold[i].offset = 0;
                                    cached_doc_mnger.gold[i].del = false;
                                    cached_doc_mnger.cache[cb].inUse += 1;
                                    stringstream ss;
                                    ss << cb;
                                    strcpy(cached_doc_mnger.gold[i].filename,ss.str().c_str());
                                    FD_SET(i,&master_write);
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
                                cout << "SERVER: Client " << i << ": If-Modified-Since: "<< cached_doc_mnger.cache[cb].expr_date << endl;
                            }
                            cached_doc_mnger.gold[i].cb = cb;
                            //cout << "SOCKET: " << i << " ==> " << "GOT BLOCK " << cached_doc_mnger.gold[i].cb << endl;
                            
                            int new_web_socket = Utility::connect_to_web(temp_request_info->host, web_port);
                            cached_doc_mnger.type[new_web_socket]=FETCH_TYPE;
                            cached_doc_mnger.fetcher[i] = new_web_socket;
                            cached_doc_mnger.requester[new_web_socket] = i;

                            cached_doc_mnger.gold[i].del = true; // setting the parameters of the request
                            cached_doc_mnger.gold[i].offset = 0;
                            stringstream ss;
                            int rr = Utility::getRandomNumber();		// generate random number for temp_request_info file name
                            while(cached_doc_mnger.checkRand.find(rr) != cached_doc_mnger.checkRand.end() ){
                                rr = (rr+1)%1000000007; // if requests are at same time the rand() will generate same random number so increment by 1 till we get a new number
                            }
                            cached_doc_mnger.checkRand[rr]=true;
                            ss << "tmp_"<< rr;
                            strcpy(cached_doc_mnger.gold[i].filename,ss.str().c_str());
                            //cout << "SERVER: Client " << i << " ==> " << "Tmp filename " << cached_doc_mnger.gold[i].filename << endl;
                            ofstream touch;
                            touch.open(cached_doc_mnger.gold[i].filename,ios::out | ios::binary); // touch the tmp file
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

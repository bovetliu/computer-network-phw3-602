#include "ProxyUtility.hpp"
#include "CachedDocManager.hpp"

#define STDIN 0

// Thanks BEEJ.US for tutorial on how to use SELECT and pack and unpack functions
// Some parts of the code have been taken from BEEJ.US

using namespace std;
unsigned short int client_count; // global variable to store client count

int main(int argc, char *argv[]){

    //char ipstr[INET_ADDRSTRLEN];		//INET6_ADDRSTRLEN for IPv6
    int error;
    connection_info server_conn_info;
    Utility::initialize_server(argc, argv, server_conn_info);
    CachedDocManager cached_doc_mnger;
    string temp_suffix("_temp_suffix");

    // Initializing the required variables
    char buf[2048],time_buf[256];
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
                    cached_doc_mnger.clifd_map.erase(i);
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
                else{  // so now,  we all process web sockets,  all i here, should be web sockfds
                    if((num_bytes = recv(i,buf,sizeof(buf),0)) <=0){
                        if(num_bytes == 0){ // If received bytes are zero, it means client has disconnected, so remove its allocated resources
                        }
                        else{
                            perror("server: recv");
                        }
                        close(i);
                        FD_CLR(i,&master);
                        
                        if(cached_doc_mnger.type_of[i]== FETCH_TYPE){  // web server closed connection, needs to send file back now

                            // Copy tmp file to cache block and serve from cache block
                            string expiration_str;
                            bool has_304 = false;
                            bool has_expiration_header = false;

                            cached_doc_mnger.analyzeHeaderOfFile( (cached_doc_mnger.webfd_map[i]->node_name + temp_suffix).c_str(),
                                                                has_304, has_expiration_header,expiration_str);
                            memset(&temptime, 0, sizeof(struct tm));
                            
                            if (has_expiration_header){  // means expiration_str has value
                                //strptime: convert a string  representation  of time to a time tm structure
                                strptime(expiration_str.c_str(), "%a, %d %b %Y %H:%M:%S ", &temptime);
                                cached_doc_mnger.webfd_map[i]->expr_time = mktime(&temptime);
                                cached_doc_mnger.webfd_map[i]->expr_date = expiration_str;
                            } else {  // does not has expiration header, 
                                time(&rawtime);
                                utc = gmtime(&rawtime);
                                strftime(time_buf, sizeof(time_buf), "%a, %d %b %Y %H:%M:%S ",utc);	// If Expires field was not present, set the current time as cache block expire time
                                cached_doc_mnger.webfd_map[i]->expr_time = mktime(utc);
                                cached_doc_mnger.webfd_map[i]->expr_date = string(time_buf) + string("GMT");
                            }
                            time(&rawtime);
                            utc = gmtime(&rawtime);
                            strftime(time_buf, sizeof(time_buf), "%a, %d %b %Y %H:%M:%S ",utc);
                            cached_doc_mnger.webfd_map[i]->last_client_access = mktime(utc);  // updating last access time
                            cout << "SERVER: Proxy Server Time: " << time_buf << "GMT" << endl;
                            cout << "SERVER: File Expires Time: " << cached_doc_mnger.webfd_map[i]->expr_date << endl;
                            cout << "SERVER: Last Access is time above: " << cached_doc_mnger.webfd_map[i]->request.c_str() << endl;

                            if(has_304){
                                //  This is just a 304 header 
                                cout << "SERVER: temp file has 304" <<endl;
                                cout << "SERVER: I just leave it there, will serve use core file" << endl;
                            }
                            else { // If it is not 304 response, see if we can write to the cache block assigned, or get a new cache block
                                cout << "SERVER: temp file does not have 304" <<endl;
                                // updating core
                                rename( ( cached_doc_mnger.webfd_map[i]->node_name + temp_suffix).c_str(), cached_doc_mnger.webfd_map[i]->node_name.c_str() );
                                
                            }
                            // following functin will clean webfd_map and add clifd_map
                            cached_doc_mnger.addReqSocketsOfNodeToWFD(*(cached_doc_mnger.webfd_map[i]), write_fds, master, i);
                        }
                        else{
                            client_count--;
                            cout << "SERVER: Client " << i << ": Disconnected" << endl;		// check if cached_doc_mnger.req_sockfd disconnected
                        }
                    } else{// recved num_bytes > 0 situation
                        if(cached_doc_mnger.type_of[i] == FETCH_TYPE){  
                            cout << "WRITING PARTS to Cache" <<(cached_doc_mnger.webfd_map[i]->node_name + temp_suffix).c_str()<< endl;
                            ofstream myfile2;		// Fetcher writes the data into a tmp file for sending back to cached_doc_mnger.req_sockfd later
                            myfile2.open( (cached_doc_mnger.webfd_map[i]->node_name + temp_suffix).c_str() ,ios::out | ios::binary | ios::app);
                            if(!myfile2.is_open()){ 
                                cout << "some error in opening file" << endl;
                            }
                            else{
                                myfile2.write(buf,num_bytes);
                                myfile2.close();
                            }
                        } else {  //  this socket_fd is a req_sockfd, from client
                            struct info* temp_request_info;
                            temp_request_info = cached_doc_mnger.parse_http_request(buf,num_bytes);
                            string new_request(string(temp_request_info->host)+string(temp_request_info->file) );
                            cout << "SERVER: Client "<< i << ": GET Request: " << new_request << endl;
                            struct LRU_node * plru_node = cached_doc_mnger.allocOneNode(new_request,temp_request_info, i);
                            cout << "[DEBUG]lru_node.expr_time: " <<plru_node->expr_time << endl;
                            memset(buf,'\0' , 2048);
                            
                            cached_doc_mnger.prepareAdaptiveRequestForWeb( plru_node, i, buf);
                            
                            if (strlen(buf) == 0){
                                FD_SET(i,&write_fds);  // this client_sock can be written back now
                                cout << "SERVER: serving " << i <<   " use valid LRU_node " << endl;
                                continue;
                            }
                            int new_web_socket = Utility::connect_to_web(temp_request_info->host, web_port);
                            cached_doc_mnger.type_of[new_web_socket]=FETCH_TYPE;
                            plru_node->web_sock_fd = new_web_socket;
                            
                            ofstream touch;
                            touch.open( (plru_node->node_name + temp_suffix).c_str() ,ios::out | ios::binary); 
                            if(touch.is_open())
                                touch.close();
                            FD_SET(new_web_socket,&master);
                            cached_doc_mnger.webfd_map[new_web_socket] = &(cached_doc_mnger.page_to_node_map[new_request]);
                            if(new_web_socket > fdmax)
                                fdmax = new_web_socket;
                            cout << "SERVER: Fetcher started at socket " << new_web_socket << ": for client " << i << endl;
                            
                            if( (error = send(new_web_socket,buf,strlen(buf),0)) == -1){ // send the get or conditional get request
                                perror("client: send");
                            }
                            free(temp_request_info);
                        }  // (num_bytes > 0 && req_type)  ends
                    }  // (num_bytes > 0) ends
                }  // handle active fds, instead of new connections
            } // handle read_fds
        } // for (all of fds)
    } // end of while(1)
    return 0;
}

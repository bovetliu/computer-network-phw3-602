#include "CachedDocManager.hpp"

CachedDocManager::CachedDocManager()
{

}

CachedDocManager::~CachedDocManager()
{
}



// function to parse the incoming HTTP request, decodes the File Name and Host Name 
struct info * CachedDocManager::parse_http_request(const char *buf, int num_bytes){
    struct info* output = (struct info *)malloc(sizeof(struct info));
    int i;
    for(i=0; i<num_bytes; i++){
        if(buf[i]==' '){
            i++;
            break;
        }
    }
    int k=0;
    output->file[0]='/';
    if(buf[i+8]=='\r' && buf[i+9]=='\n'){
        output->file[1]='\0';
    }
    else{
        while(buf[i]!=' ')
            output->file[k++]=buf[i++];
        output->file[k]='\0';
    }
    i+=1;
    while(buf[i++]!=' ');
    k=0;
    while(buf[i]!='\r')
        output->host[k++]=buf[i++];
    output->host[k]='\0';
    return output;
}

void CachedDocManager::sendPartOfFileToRequester(int requester_sockfd, fd_set &write_fds, fd_set &master, char * shared_charbuf){
    ifstream myfile1;
    int num_bytes, error;
    myfile1.open( clifd_map[requester_sockfd]->node_name.c_str() ,ios::in | ios::binary); // open the cache file or tmp file
    if( !myfile1.is_open() ){
        cout << clifd_map[requester_sockfd]->node_name << endl;
        cout << "SOCKET: " << requester_sockfd << " ==> " << "Could Not Open Cache File" << endl;
        FD_CLR(requester_sockfd,&write_fds);
    }
    else{  // opened successfully
        //seekg:Set position in input sequence
        myfile1.seekg( (clifd_map[requester_sockfd]->req_readpointer_map[requester_sockfd] )*2000 , ios::beg); // Seek the file with the offset in the parameters of the request and increase the offset by 1
        clifd_map[requester_sockfd]->req_readpointer_map[requester_sockfd] += 1;  // next retrieve next 2000 chacters
        myfile1.read(shared_charbuf,2000);  // Read next 2000 bytes
        num_bytes = myfile1.gcount();
        //cout << num_bytes << endl;
        if(num_bytes!=0){
            cout << "SERVER: SENDING a block to client "<< requester_sockfd << endl;
            if( (error = send(requester_sockfd,shared_charbuf,num_bytes,0)) == -1){ // send the message to the client
                    perror("client: send");
            }
        }
        if(num_bytes<2000){ // reached the EOF, now close the connection
            FD_CLR(requester_sockfd,&write_fds);
            FD_CLR(requester_sockfd,&master); 
            close(requester_sockfd); // Connection closed with client and removed from master Read and Write sets
            clifd_map[requester_sockfd]->req_readpointer_map.erase(requester_sockfd);
            cout << "SERVER: Finished Sending, Connection closed at client " << requester_sockfd << endl;
        }
    } // open successfully ends
    myfile1.close();
    
    
}

void CachedDocManager::analyzeHeaderOfFile(const char * filename, bool & has_304, bool & has_expiration_header, string & expiration_str ){
    ifstream myfile3;
    myfile3.open(filename,ios::in | ios::binary);
    int counter = 0;
    string line;
    has_304 = false;
    has_expiration_header = false;
    if(myfile3.is_open()){

        // following while mainly to search 304 or expires header
        while( !myfile3.eof() ){
            getline(myfile3,line);
            if(line.compare("\r") == 0){  // reached end of headers, no need to check
                break;
            }
            counter++;
            if(line.find("304")!=string::npos)// Check for 304 responses
                has_304 = true;

            //sample header Expires: Mon, 29 Apr 2013 21:44:55 GMT
            transform(line.begin(), line.end(), line.begin(), ::tolower);
            if(line.find("expires:")==0){ // Check for the Expires Field
                has_expiration_header = true;
                int pos = 8;
                while(line[pos] != ' '){
                    pos++;
                }
                expiration_str = line.substr( pos,line.length() );
                printf("found expires: %s\n", expiration_str.c_str());
                break;
            }
        }
        myfile3.close();
    }
    
}

struct LRU_node * CachedDocManager::allocOneNode( string request, struct info * tempinfo, int client_sock_fd){
    // request is like www.easysublease.com/howitworks   no protocol
    if (this->page_to_node_map.find(request) != this->page_to_node_map.end()){   
        page_to_node_map[request].req_readpointer_map[client_sock_fd] = 0;
        cout << "old node expr_date: "<< page_to_node_map[request].expr_date.c_str() << endl;
        return &(page_to_node_map[request]);
    }
    // did not find LRU_node matching this request
    struct LRU_node new_node;
    
    new_node.domain_name = string(tempinfo->host);
    new_node.page_name   = string(tempinfo->file);
    new_node.request = string(tempinfo->host)+ string(tempinfo->file);
    new_node.req_readpointer_map[client_sock_fd] = 0;
    new_node.node_name = this->castNumberToString( page_to_node_map.size() );
    new_node.in_use = 1;
    new_node.expr_time = 0 ;
    new_node.last_client_access = 0;
    new_node.web_sock_fd = 0; // the web_sock_fd responsible for filling it
    this->page_to_node_map[request] = new_node;
    
    return &(this->page_to_node_map[request]);
}

void CachedDocManager::prepareAdaptiveRequestForWeb(struct LRU_node * p_lru_node, int client_sock_fd, char * shared_buf ){
    string tmp_str = "GET "+string(p_lru_node->page_name)+" HTTP/1.0\r\nHost: "+string(p_lru_node->domain_name)+"\r\n\r\n";
    if ( p_lru_node->expr_time == 0 ){
        cout << "SERVER: \n" << tmp_str.c_str() << endl;
    } else if ( this->isExpiredTime(p_lru_node->expr_time) ){
        tmp_str = "GET "+string(p_lru_node->page_name)+" HTTP/1.0\r\nHost: "+string(p_lru_node->domain_name)+"\r\n" ;
        tmp_str = tmp_str + "If-Modified-Since: " + p_lru_node->expr_date+" GMT\r\n\r\n";		// If Stale send If-Modified-Since field in the header
        cout << "SERVER: \n" << tmp_str.c_str() << endl;
    } else {
        // No need to GEt
        cout << "SERVER: No need to get since not expired" << endl;
        clifd_map[client_sock_fd] = p_lru_node;
        tmp_str = "";
    }
    strncpy (shared_buf, tmp_str.c_str(),2047);

}

bool CachedDocManager::isExpiredTime(int input_time){
    time_t raw_time;
    time(&raw_time);
    struct tm * utc;
    utc = gmtime(&raw_time);
    raw_time = mktime(utc);  // return time_t
    //cout << "CACHE : " << cache[cb].expr << " :: CURRENT : " << raw_time << endl;
    if(input_time - raw_time > 0)
        return false;
    return true;
}

void CachedDocManager::addReqSocketsOfNodeToWFD(struct LRU_node & target_nd, fd_set & write_fds, fd_set & master, int current_sock_fd){
    map<int ,int> t_rq = target_nd.req_readpointer_map;
    for (map<int, int>::iterator it = t_rq.begin(); it!= t_rq.end(); ++ it){
        FD_SET(it->first, &write_fds);
        clifd_map[it->first] = &target_nd;

    }

    if (current_sock_fd == target_nd.web_sock_fd){
        webfd_map.erase(current_sock_fd);
    }
    
}
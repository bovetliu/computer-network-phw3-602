#include "CachedDocManager.hpp"

CachedDocManager::CachedDocManager()
{
    for(int i=0; i< MAX_NODE; i++){
        cache[i].last=i+1;
        cache[i].inUse=0;
    }
}

CachedDocManager::~CachedDocManager()
{
}

void CachedDocManager::bringToFront(int block){
    
    if(cache[block].last==1)
        return;
    //cout << "Bringing to Front " << block << endl;
    cache[block].last = 1;
    for(int i=0; i<SIZE; i++){
        if(i!=block){
            cache[i].last++;
            if(cache[i].last>SIZE)
                cache[i].last = SIZE;
        }
    }
    return;
}

int CachedDocManager::getFreeBlock(){
    int end = SIZE;
    int ans = -1;
    //for(int i=0; i<SIZE; i++)
    //	cout << cache[i].last << ",";
    //cout << endl;
    while(end>0){
        for(int i=0; i<SIZE; i++){
            if(cache[i].last==end && cache[i].inUse==0){
                ans = i;
                cache[ans].inUse=-2;
                //cache[ans].last=1;
                //cout << "Ans: "<< ans << endl;
                end=0;
                break;
            }
        }
        end--;
    }
    return ans;
}


// function to check if the current URL requested exists in cache or not
int CachedDocManager::checkCache(string host_file){
    if(whichBlock.find(host_file)==whichBlock.end()){
        return -1;
    }
    int cb = whichBlock[host_file];
    if(cache[cb].host_file == host_file)
        return cb;
    return -1;
}

// check if a block in cache is STALE or NOT
bool CachedDocManager::isExpired(int cb){
    time_t raw_time;
    time(&raw_time);
    struct tm * utc;
    utc = gmtime(&raw_time);
    raw_time = mktime(utc);
    //cout << "CACHE : " << cache[cb].expr << " :: CURRENT : " << raw_time << endl;
    if(cache[cb].expr-raw_time>0)
        return false;
    return true;
}


// function to parse the incoming HTTP request, decodes the File Name and Host Name 
struct info * CachedDocManager::parse_http_request(char *buf, int num_bytes){
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
    myfile1.open( req_paras[requester_sockfd].filename ,ios::in | ios::binary); // open the cache file or tmp file
    if( !myfile1.is_open() ){
        cout << req_paras[requester_sockfd].filename << endl;
        cout << "SOCKET: " << requester_sockfd << " ==> " << "Could Not Open Cache File" << endl;
        FD_CLR(requester_sockfd,&write_fds);
    }
    else{  // opened successfully
        //seekg:Set position in input sequence
        myfile1.seekg( (req_paras[requester_sockfd].offset)*2000 , ios::beg); // Seek the file with the offset in the parameters of the request and increase the offset by 1
        req_paras[requester_sockfd].offset+=1;  // next retrieve next 2000 chacters
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
            cout << "SERVER: Finished Sending, Connection closed at client " << requester_sockfd << endl;
            if(req_paras[requester_sockfd].cb==-1){
                //cout << "SOCKET: " << requester_sockfd << " ==> " << "Remove Temp File" << endl;
                if(req_paras[requester_sockfd].del==true)
                    remove(req_paras[requester_sockfd].filename);	// Delete the tmp file
            }
            else if(req_paras[requester_sockfd].cb != -1){
                bringToFront(req_paras[requester_sockfd].cb);
                cache[req_paras[requester_sockfd].cb].inUse-=1;		// Decrease the in Use counter by 1
                //cout << "SOCKET: " << requester_sockfd << " ==> " << "Decrease inUse by 1 : " << cache[req_paras[requester_sockfd].cb].inUse<<endl;
            }
        }
    } // open successfully ends
    myfile1.close();
    
    
}
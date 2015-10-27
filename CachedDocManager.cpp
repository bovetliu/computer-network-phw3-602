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
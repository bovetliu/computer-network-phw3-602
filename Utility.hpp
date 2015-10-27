#ifndef UTILITY_HPP
#define UTILITY_HPP

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>  // close (file_descriptor) needs this
#include <fcntl.h>
#include <cstring> //  memcpy({3}) need this
#include <arpa/inet.h>
#include <netinet/in.h>  
// above header is important, most networking programming stucts and MACRO constants such as INET_ADDRSTRLEN are defined within it

#include <stdio.h>

#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <cstdlib>
#include <ctime>

using namespace std;

typedef struct
{
    int sockfd;
    struct sockaddr_in address;
    struct addrinfo  address_info;

} connection_info;

struct web_file_info {
    char file[1024];
    char host[1024];
};

class Utility
{
public:
    Utility();
    ~Utility();
    
    
        /*
    ** from Beej
    ** packi16() -- store a 16-bit int into a char buffer (like htons())
    */
    static void packi16(char *buf, unsigned short int i)
    {
        // for example i = 0x34d6
        // buffer[0] = 34   buffer[1] = d6
        // Big Endian
        *buf++ = i>>8;
        *buf++ = i;
    }

    // copied from team8 homework2
    static uint16_t unpacki16(char *buf)     //change  network byte order to the host order (16bit)
    {
        uint16_t i;
        memcpy(&i,buf,2);  // require <cstring>
        i = ntohs(i);
        return i;
    }
    
    // used code from Kan Zheng, our group member
    static int connect_to_web(char* ipaddr,char* port)
    {
        int sockfd;
        int rv;
        struct addrinfo hints, *servinfo, *p;
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        if ((rv = getaddrinfo(ipaddr, port, &hints, &servinfo)) != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
            return -1;
        }
        // loop through all the results and connect to the first we can
        for(p = servinfo; p != NULL; p = p->ai_next) {
            if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
                perror("client: socket");
                continue;
            }
            if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
                close(sockfd);
                perror("client: connect");
                continue;
            }       
            break;
        }
        if (p == NULL) {
            fprintf(stderr, "client: failed to connect\n");
            return -1;
        }
        freeaddrinfo(servinfo); // all done with this structure
        return sockfd;
    }


    // the purpose of this function is get valid socket file descriptor
    // return 0, means it is functioning properly
    static int initialize_server( int argc, char* argv[], connection_info& server_info)
    {
        int sockfd;
        if (argc != 3)
        {
            printf ("usage: ./server <ip> <port> \n");
            return 1;
        }
        struct addrinfo hints, *servinfo, *p;

        int status = -1;
        int error;
        memset(&hints, 0, sizeof hints);

        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        //hints.ai_flags = AI_PASSIVE;   // use my own ip address

        if ( (status = getaddrinfo(argv[1], argv[2], &hints, &servinfo ) ) != 0 )
        {
            fprintf( stderr, "getaddrinfo: %s\n", gai_strerror(status));
            return 1;
        }
        // servinfo now points to result linked list
        for (p = servinfo; p != NULL; p = p -> ai_next )
        {
            if ( (sockfd = socket(p -> ai_family, p-> ai_socktype, p->ai_protocol) ) == -1)
            {
                perror("unable to listern at this *p");
                continue;
            }
            if (bind(sockfd, p->ai_addr, p-> ai_addrlen) == -1)
            {
                close(sockfd);
                perror("unable to bind this *p");
                continue;
            }
            // no error happened to this *sockfd, which means it is a valid one.
            break;
        }
        if (p == NULL)
        {
            fprintf(stderr, "listerner: failed to bind sockets\n");
            return 2;
        }
        if( (error = listen(sockfd, 10)) == -1){		// server starts listening with BACKLOG equal to the MAXCLIENTS
            perror("server: listen");
        }

        //starting to fill container arguemnts
        server_info.sockfd = sockfd;
        server_info.address = *( (struct sockaddr_in *) p->ai_addr);
        server_info.address_info = *p;
        printf("Proxy Server: started \n");
        freeaddrinfo(servinfo); // no longer need this
        return 0;
    }



    // from page 36 of beej
    // get pointer of sin_addr or sin6_addr , IPv4 or IPv6:
    static void *get_in_addr(struct sockaddr *sa)
    {
        if (sa->sa_family == AF_INET)
        {
            return &(((struct sockaddr_in*)sa)->sin_addr);
        }
        return &(((struct sockaddr_in6*)sa)->sin6_addr);
    }

    
    static struct web_file_info * parse_http_request(char *buf, int num_bytes){
        struct web_file_info* output = (struct web_file_info *)malloc(sizeof(struct web_file_info));
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
    // generate random number with current time as seed
    static int getRandomNumber(){
        srand(time(NULL));
        return rand();
    }

};

#endif // UTILITY_HPP

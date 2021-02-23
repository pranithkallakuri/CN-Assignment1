/* f20180249@hyderabad.bits-pilani.ac.in Pranith S Kallakuri */

/* Brief description of program...*/
/* ... */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>     //socket(); connect()
#include <sys/socket.h>    //socket(); connect; inet_addr();
#include <netinet/in.h>    //inet_addr();
#include <arpa/inet.h>     //inet_addr(); htons();
#include <string.h>        //memset();
#include <unistd.h>        //close(); read();
#define MAX_BUFFER_SIZE 1024
#define FINAL_STATE 4

//Global Variables
int state = 0;
int inside_header = 1;

int get_body_beginning(char* buffer, int rv)
{
    for(int i = 0; i < rv; i++)
    {
        if(state == FINAL_STATE)
        {
            inside_header = 0;
            printf("HEYYYY");
            return i;
        }
        switch(buffer[i])
        {
            case '\r':
                if(state == 0) state = 1;
                else if(state == 2) state = 3;
                else if(state == 3) state = 1;
                else state = 0;
                break;
            
            case '\n':
                if(state == 1) state = 2;
                else if(state == 3) state = 4;
                else state = 0;
                break;
            default:
                state = 0;
        }
    }

    return 0;
}

char* encode_to_base64(char *input_str) 
{ 
    int inp_len = strlen(input_str); 
    char* retstr = (char*)calloc(1000, sizeof(char)); 
    int shamt;
    int padding = 0;
    int k = 0; 

    char *list_of_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
      
    for (int i = 0; i < inp_len; i += 3) 
    { 
        int nbits = 0;
        int value = 0;
        int cnt = 0; 

        for (int j = i; j < inp_len && j < i + 3; j++) 
        { 
            value = value << 8;  
            value |= input_str[j];  
            cnt++; 
        } 

        nbits = cnt * 8;  
        padding = nbits % 3;
        int ind;  

        while (nbits != 0)  
        { 
            if (nbits > 5) 
            { 
                shamt = nbits - 6; 
                ind = (value >> shamt) & 63;  
                nbits -= 6;          
            } 
            else
            { 
                shamt = 6 - nbits; 
                ind = (value << shamt) & 63;  
                nbits = 0; 
            } 
            retstr[k++] = list_of_chars[ind]; 
        } 
    } 
    
    for (int i = 1; i < padding+1; i++)  
        retstr[k++] = '='; 
    retstr[k] = '\0'; 
  
    return retstr; 
}

char* change_website(int it, char *head_buffer, char *website)
{
    char* new_website = malloc(2000*sizeof(char));
    if(head_buffer[it] == 'h')
    {
        //skip "http://" len = 7
        //printf("Here_change_website");
        int k = 7;
        while(head_buffer[it+k] != '\r')
        {
            new_website[k-7] = head_buffer[it+k];
            k++;
        }
        new_website[k] = '\0';
        return new_website;
    }
    else if(head_buffer[it] == '/')
    {
        int k = 0;
        while(head_buffer[it+k] != '\r')
            k++;

        int slash_ind = -1;
        int web_len = strlen(website);
        for(int i = 0; i < web_len; i++)
        {
            if(website[i] == '/')
            {
                slash_ind = i;
                break;
            }
        }

        if(slash_ind == -1)
        {
            strncpy(new_website, website, web_len);
            strncat(new_website, head_buffer+it, k);
            new_website[k] = '\0';
            return new_website;

        }
        else
        {
            strncpy(new_website, website, web_len);
            for(int i = 0; i < k; i++)
                new_website[slash_ind + i] = head_buffer[it+slash_ind+i];

            new_website[k] = '\0';
            return new_website;
        }
    }
    return "Error";
}

//http://bits-judge-server.herokuapp.com/redirect
//info.in2p3.fr

int main(int argc, char* argv[])
{
    
    char *url = argv[1];
    const char *proxy_ip = argv[2];
    const int proxy_port = atoi(argv[3]);
    char *username = argv[4];
    char *password = argv[5];
    const char *html_filename = argv[6];
    char* img_filename = "garb.txt";
    if(argc == 8) img_filename = argv[7];

    char auth[strlen(username)];
    strncpy(auth, username, strlen(username));
    strncat(auth, ":", 1);
    strncat(auth, password, strlen(password));

    char *website = (char *)malloc(2000*sizeof(char));
    strncpy(website, url, strlen(url));

    char* auth_base64 = encode_to_base64(auth);

    // int sockfd;
    // sockfd = socket(AF_INET, SOCK_STREAM, 0);
    // if(sockfd == -1) printf("Sock_Error");

    struct sockaddr_in proxy;
    memset(&proxy, 0, sizeof(struct sockaddr_in));
    
    proxy.sin_addr.s_addr = inet_addr(proxy_ip);
    proxy.sin_family = AF_INET;
    proxy.sin_port = htons(proxy_port);

    // printf("Connecting...\n");
    // if(connect(sockfd, (struct sockaddr *)&proxy, sizeof(struct sockaddr_in)) == -1)
    //     printf("connect_error\n");
    // else
    //     printf("Connected\n");
    
    char request[MAX_BUFFER_SIZE];

    // memset(request, 0, MAX_BUFFER_SIZE);
    // sprintf(request, "HEAD http://%s HTTP/1.1\r\nProxy-Authorization: Basic %s\r\nConnection: close\r\n\r\n", website, auth_base64);

    char buffer[MAX_BUFFER_SIZE];
    size_t reply_len;
    size_t total_reply_len;

    //get html file //      HTTP/1.1 301 Moved Permanently //9 to 11 indices
    //check for redirects
    while(1)
    {
        //if HTTP code 200 - break;
        int sockfd;
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if(sockfd == -1) printf("Sock_Error");

        printf("Connecting...\n");
        if(connect(sockfd, (struct sockaddr *)&proxy, sizeof(struct sockaddr_in)) == -1)
            printf("connect_error\n");
        else
            printf("Connected\n");

        memset(request, 0, MAX_BUFFER_SIZE);
        sprintf(request, "HEAD http://%s HTTP/1.1\r\nProxy-Authorization: Basic %s\r\nConnection: close\r\n\r\n", website, auth_base64);
        size_t total = 0;
        size_t req_len = strlen(request);
        while(total != req_len)
        {
            size_t snd = send(sockfd, request, strlen(request), 0);
            if(snd == -1) printf("send_Error\n");
            total += snd;
        }

        total = 0;
        char* head_buffer = (char*)malloc(10000*sizeof(char)); 
        while(1)
        {
            size_t rv = recv(sockfd, head_buffer+total, 10000 - total, 0);
            printf("rv = %ld\n", rv);
            if(rv == -1){ printf("recv_error\n"); continue; }
            if(rv == 0) break;

            total += rv;           
        }

        char status_str[4];
        int code;
        fwrite(head_buffer, 1, total, stdout);
        strncpy(status_str, head_buffer+9, 3);
        code = atoi(status_str);
        printf("code = %d\n", code);
        close(sockfd);
        
        if(code == 200) break;

        if(code/10 == 30)
        {
            //Find Location Header
            int head_len = total;
            int i_start = 0;
            for(int i = 0; i < head_len; i++)
            {
                if(head_buffer[i] == '\r' && head_buffer[i+1] == '\n')
                {
                    //Check if line is "Location"
                    if(strncmp(head_buffer+i_start, "Location:", strlen("Location:")) == 0)
                    {
                        //In location line, from i_start(include) to i(exclude)
                        //website start is from i_start+10
                        website = change_website(i_start+10, head_buffer, website);
                        printf("Found\n");
                        break;
                    }
                    i_start = i+2;
                }
            }
        }
        break;
    }
    printf("%s\n", website);

    //return 0;
    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1) printf("Sock_Error");

    printf("Connecting...\n");
    if(connect(sockfd, (struct sockaddr *)&proxy, sizeof(struct sockaddr_in)) == -1)
        printf("connect_error\n");
    else
        printf("Connected\n");

    memset(request, 0, MAX_BUFFER_SIZE);
    sprintf(request, "GET http://%s HTTP/1.1\r\nProxy-Authorization: Basic %s\r\nConnection: close\r\n\r\n", website, auth_base64);
    
    size_t total = 0;
    size_t req_len = strlen(request);
    while(total != req_len)
    {
        size_t snd = send(sockfd, request, strlen(request), 0);
        if(snd == -1) printf("send_Error\n");
        total += snd;
    }

    FILE *fp = fopen(html_filename, "w");
    inside_header = 1;
    total = 0;

    while(1)
    {
        size_t rv = recv(sockfd, buffer, MAX_BUFFER_SIZE, 0);
        printf("rv = %ld\n", rv);
        if(rv == -1){ printf("recv_error\n"); continue; }
        if(rv == 0) break;

        // Parsing to remove header
        if(inside_header) 
        {
            int start = get_body_beginning(buffer, rv);
            printf("state = %d\n", state);
            printf("start = %d\n", start);
            if(!inside_header)
                fwrite((void*)(buffer+start), 1, rv-(size_t)start, fp);
        }
        else
            fwrite((void*)buffer, 1, rv, fp);

        fwrite((void*)buffer, 1, rv, stdout);
        total += rv;
    }

    printf("Closing socket...");
    fclose(fp);
    close(sockfd);

    if(strncmp(website, "info.in2p3.fr", strlen("info.in2p3.fr")) == 0)
    {  
        //get image file
        printf("Reopening closed socket\n");

        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        printf("Connecting...\n");
        if(connect(sockfd, (struct sockaddr *)&proxy, sizeof(struct sockaddr_in)) == -1)
            printf("connect_error\n");
        else
            printf("Connected\n");

        memset(request, 0, MAX_BUFFER_SIZE);
        sprintf(request, "GET http://info.in2p3.fr/cc.gif HTTP/1.1\r\nProxy-Authorization: Basic %s\r\nConnection: close\r\n\r\n", auth_base64);

        total = 0;
        size_t req_len = strlen(request);
        while(total != req_len)
        {
            size_t snd = send(sockfd, request, strlen(request), 0);
            if(snd == -1){ printf("send_Error\n"); continue; }
            total += snd;
        }

        FILE *fp = fopen(img_filename, "w");
        inside_header = 1;
        state = 0;

        while(1)
        {
            size_t rv = recv(sockfd, buffer, MAX_BUFFER_SIZE, 0);
            printf("rv = %ld\n", rv);
            if(rv == -1){ printf("recv_error\n"); continue; }
            if(rv == 0) break;

            // Parsing to remove header
            if(inside_header) 
            {
                int start = get_body_beginning(buffer, rv);
                printf("state = %d\n", state);
                printf("start = %d\n", start);
                if(!inside_header)
                    fwrite((void*)(buffer+start), 1, rv-(size_t)start, fp);
            }
            else
                fwrite((void*)buffer, 1, rv, fp);

            //fwrite((void*)buffer, 1, rv, stdout);
            total += rv;
        }

        printf("Closing socket...");
        fclose(fp);
        close(sockfd);
    }
 
    return 0;
}
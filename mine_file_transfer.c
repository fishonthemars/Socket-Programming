#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <arpa/inet.h>
#include <errno.h>

#define FILE_NAME_MAX_SIZE  512
#define BUFFER_SIZE 256 

void error(const char *msg) {
    perror(msg);
    exit(1);
}


int main(int argc, char const *argv[]){

	if(strcmp(argv[1], "tcp")==0) {

		if(strcmp(argv[2], "send")==0) {
			int sockfd, newsockfd, portN;
			portN = atoi(argv[4]);
		    socklen_t clilen;
		    char buffer[BUFFER_SIZE];
		    struct sockaddr_in serv_addr, cli_addr;
		    int n;
		    
		    sockfd = socket(AF_INET, SOCK_STREAM, 0); //Create Socket(AF_INET=internet IPv4, SOCK_STREAM=TCP) 
		    bzero((char *) &serv_addr, sizeof(serv_addr));

		    if (sockfd < 0) {
		       error("Socket Create Fail");
		    }

		    serv_addr.sin_family = AF_INET; //Socket通訊協定家族:AF_UNIX(Unix),AF_INET(Internet)
		    serv_addr.sin_addr.s_addr = INADDR_ANY;
		    serv_addr.sin_port = htons(portN);

		    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		    	error("Server Binding Fail");
		    }
		    
		    listen(sockfd,20); //listen()進入被動監聽狀態

		    while(1) {
		    	clilen = sizeof(cli_addr);
			    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
			    if (newsockfd < 0) {
			        error("Accept Fail");
			    }

			    char file_name[FILE_NAME_MAX_SIZE + 1];
			    bzero(file_name, sizeof(file_name));

			    if(strlen(argv[5]) <= FILE_NAME_MAX_SIZE){
                    strncpy(file_name, argv[5],strlen(argv[5]));
                } 
                else {
                    error("File Name too Long");
                }

                send(newsockfd, file_name, strlen(file_name), 0); //send File Nameto client

			    FILE *fp = fopen(file_name, "r");
			    if(fp==NULL) {
			    	error("Fopen Fail");
			    } 
			    else {
			    	
			    	int fileNum;
			    	fseek(fp, 0, SEEK_END);
			    	fileNum = ftell(fp);
			    	send(newsockfd, (void *)&fileNum, sizeof(fileNum), 0);
			    	fseek(fp, 0, SEEK_SET);
			    	bzero(buffer,BUFFER_SIZE);
			    	int file_block_length = 0;  
			    	while( (file_block_length = fread(buffer, sizeof(char), BUFFER_SIZE, fp)) > 0) {
			    		
			    		if (send(newsockfd, buffer, file_block_length, 0) < 0) {  
		                    printf("Send File Fail\n");  
		                    break;  
		                } 
		                bzero(buffer, sizeof(buffer));   
			    	}
			    	fclose(fp);
			    	printf("File Transfer Finished\n");  
			    }
			    close(newsockfd);
		    }
		    close(sockfd);

		    return 0;
		} 
		else if(strcmp(argv[2], "recv")==0) {
			int sockfd, portN, n;
			portN = atoi(argv[4]);
			sockfd = socket(AF_INET, SOCK_STREAM, 0);
		    struct sockaddr_in serv_addr;
		    struct hostent *server;
		    char buffer[BUFFER_SIZE];  
		    
		    if (sockfd < 0){ 
		        error("Socket Create Fail");
		    }
		    server = gethostbyname(argv[3]);
		    if (server == NULL) {
		        fprintf(stderr,"Host Error\n");
		        exit(0);
		    }
		    bzero((char *) &serv_addr, sizeof(serv_addr));
		    serv_addr.sin_family = AF_INET;
		    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
		    serv_addr.sin_port = htons(portN);

		    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0){ 
		        error("Connect Fail");
		    }

		    char file_name[FILE_NAME_MAX_SIZE + 1];  
    		bzero(file_name, sizeof(file_name));
    		bzero(buffer, sizeof(buffer));
    		
    		int message_length;
    		if((message_length = recv(sockfd, buffer, BUFFER_SIZE, 0))) {
    			strncpy(file_name, buffer, strlen(buffer));
                file_name[strlen(buffer)] = '\0';
    		}

    		int fileNum;
    		double fileSize;
    		recv(sockfd, (void *) &fileNum, sizeof(fileNum), 0);
            fileSize = fileNum/(1024*1024);
            
    		FILE *fp = fopen(file_name, "w");  
		    if (fp==NULL) {  
		        error("Fopen Fail");  
		    }  

		    bzero(buffer, sizeof(buffer));  
		    n=0;
		    clock_t t1, t2;
		    t1 = clock();

		    int total_length = 0;
		    int index = 0;
		    time_t now;
		    struct tm * timeinfo;
		    while((n=recv(sockfd, buffer, BUFFER_SIZE, 0))) {  
		        if (n < 0){  
		            printf("Data Recieve Fail\n");  
		            break;  
		        }  
		  
		        int write_length = fwrite(buffer, sizeof(char), n, fp);  
		        if (write_length < n) {  
		            printf("File Write Fail\n");  
		            break;  
		        }  
		        bzero(buffer, BUFFER_SIZE);

		        //Persentage calculate
		        total_length += n;
		        if(total_length >= (fileNum/4)*index) {
		        	time (&now);
					timeinfo = localtime (&now);
                    printf("%d%%",index*25);
                    printf(" %d/%d/%d %02d:%02d:%02d\n", timeinfo->tm_year+1900, timeinfo->tm_mon+1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
                    index++;
		        }
		    }  
		    t2 = clock();
		    printf("Finish\n");
		    printf("Total trans time: %lf ms\n", (t2-t1)/(double)(CLOCKS_PER_SEC));
		    printf("File size %f MB\n",fileSize);
		    fclose(fp);
		    close(sockfd);
		    return 0;
		} 
		else {
			fprintf(stderr,"Error Input\n");
			exit(1);
		}
	} 
	else if(strcmp(argv[1], "udp")==0) {
		if(strcmp(argv[2], "send")==0) {
			int sock;
    		if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
        		error("Socket Create Fail");
    		}

    		int portN;
    		portN = atoi(argv[4]);
    		struct sockaddr_in serv_addr;
    		bzero((char *) &serv_addr, sizeof(serv_addr));
		    serv_addr.sin_family = AF_INET;
		    serv_addr.sin_port = htons(portN);
		    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

		    if (bind(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        		error("Server Binding Fail");
		    }

		    //while(1) {
		    	char buffer[BUFFER_SIZE];
			    bzero(buffer,BUFFER_SIZE);
			    struct sockaddr_in peeraddr;
			    socklen_t peerlen;
			    peerlen = sizeof(peeraddr);

			    int n;
			    n = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&peeraddr, &peerlen);

				char file_name[FILE_NAME_MAX_SIZE + 1];
				bzero(file_name, sizeof(file_name));

				if(strlen(argv[5]) <= FILE_NAME_MAX_SIZE) {
	                strncpy(file_name, argv[5],strlen(argv[5]));
	            } else {
	                error("File Name too Long");
	            }
	            
	            sendto(sock, file_name, sizeof(file_name), 0, (struct sockaddr *)&peeraddr, peerlen);

	            FILE *fp = fopen(file_name, "r");
	            if(fp==NULL) {
			    	error("Fopen Fail");
			    } 
			    else {
			    	
			    	int fileNum;
			    	fseek(fp, 0, SEEK_END);
			    	fileNum = ftell(fp);
			    	sendto(sock, (void *)&fileNum, sizeof(fileNum), 0, (struct sockaddr *)&peeraddr, peerlen);
			    	fseek(fp, 0, SEEK_SET);

			    	bzero(buffer,BUFFER_SIZE);
			    	int file_block_length = 0;  
			    	while( (file_block_length = fread(buffer, sizeof(char), BUFFER_SIZE, fp)) > 0) {
			    		if (sendto(sock, buffer, file_block_length, 0, (struct sockaddr *)&peeraddr, peerlen) < 0) {  
		                    printf("Send File:\t%s Failed!\n", file_name);  
		                    break;  
		                } 
		                bzero(buffer, sizeof(buffer));   
			    	}
			    	strncpy(buffer, "transfer_finish", 15)

			    	for(int i=0;i<20;i++) {
			    		sendto(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&peeraddr, peerlen);
			    	}
			    	
			    	fclose(fp);
			    	printf("File:\t%s Transfer Finished!\n", file_name); 
			    }

			    close(sock);
		    //}
            return 0;
		} else if(strcmp(argv[2], "recv")==0) {
			int sock;
			if((sock = socket(PF_INET, SOCK_DGRAM, 0))<0) {
        		error("Socket Create Fail");
			}

			int portN = atoi(argv[4]);
			struct hostent *server;
			server = gethostbyname(argv[3]);
		    if (server == NULL) {
		        fprintf(stderr,"ERROR, no such host\n");
		        exit(0);
		    }
			struct sockaddr_in serv_addr;
		    memset(&serv_addr, 0, sizeof(serv_addr));

		    serv_addr.sin_family = AF_INET;
		    serv_addr.sin_port = htons(port);
		    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);

		    char buffer[BUFFER_SIZE];
		    bzero(buffer, BUFFER_SIZE);

		    char file_name[FILE_NAME_MAX_SIZE + 1];  
    		bzero(file_name, sizeof(file_name));
    		bzero(buffer, sizeof(buffer));

    		sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    		
    		int message_length;
    		if((message_length = recvfrom(sock, buffer, BUFFER_SIZE, 0, NULL, NULL))) {
    			strncpy(file_name, buffer, strlen(buffer));
                file_name[strlen(buffer)] = '\0';
    		}

    		
    		int fileNum;
    		double fileSize;
    		recvfrom(sock, (void *) &fileNum, sizeof(fileNum), 0, NULL, NULL);
            fileSize = fileNum/(1024*1024);

            FILE *fp = fopen(file_name, "w");  
		    if (fp==NULL) {  
		        error("Fopen Fail");  
		    }

		    bzero(buffer, sizeof(buffer));    

            int n=0;
            clock_t t1, t2;
		    t1 = clock();

		    int total_length = 0;
		    int index = 0;
		    time_t now;
		    struct tm * timeinfo;
            while(1) {  
            	n=recvfrom(sock, buffer, BUFFER_SIZE, 0, NULL, NULL);
            	if(n < 0) {  
		            printf("Recieve Data From Server Failed!\n");  
		            break;  
		        }
		        
		        if(strncmp(buffer, "transfer_finish", 15)==0) { //if transfer is over
		        	printf("%d%%",100);
                    printf(" %d/%d/%d %02d:%02d:%02d\n", timeinfo->tm_year+1900, timeinfo->tm_mon+1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
		        	break;
		        }
		  
		        int write_length = fwrite(buffer, sizeof(char), n, fp);  
		        if (write_length < n) {  
		            printf("File:\t%s Write Failed!\n", file_name);  
		            break;  
		        }  
		        bzero(buffer, BUFFER_SIZE);

		        //Persentage of transfer
		        total_length += n;
		        if(total_length >= (fileNum/4)*index) {
		        	time (&now);
					timeinfo = localtime (&now);
                    printf("%d%%",index*25);
                    printf(" %d/%d/%d %02d:%02d:%02d\n", timeinfo->tm_year+1900, timeinfo->tm_mon+1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
                    index++;
		        }
		    }
		    t2 = clock();
		    printf("Total trans time: %lf ms\n", ((t2-t1)/(double)(CLOCKS_PER_SEC))*1000);
		    int recvFileNum;
		    double recvFileSize;
			fseek(fp, 0, SEEK_END);
			recvFileNum = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			recvFileSize = recvFileNum/(1024*1024);
			
			double result = (double)(fileNum-recvFileNum)/fileNum;

			printf("File size %f MB\n",fileSize);
			printf("packet lost rate: %f%%\n", result*100);

    		fclose(fp);
		    close(sock);
		    return 0;
		} else {
			fprintf(stderr,"Wrong Input\n");
			exit(1);
		} 
	} else {
		fprintf(stderr,"Wrong Input\n");
		exit(1);
	}
	return 0;
}
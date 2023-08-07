#include <stdio.h> 
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
 
#define BUFLEN 256  //Max length of buffer

void die(char *s) {
    perror(s);
    exit(1);
}

struct Segment {

    unsigned short int source_port;
    unsigned short int destination_port;
    unsigned short int checksum;
    unsigned short int length;
    char payload[BUFLEN];

};

unsigned int getChecksum(struct Segment segment);
char* decimalToHex(unsigned int decimal);

int main(void) {
  
    struct sockaddr_in c_sock, s_sock;

    struct Segment segment; // declare segment to get from client
     
    int sockfd, i, slen = sizeof(c_sock) , recv_len;
    char buffer[BUFLEN];

    // get port number
    printf("Port: "); scanf("%s", buffer);
    unsigned short int port_number = atoi(buffer);
     
    //create a UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
      perror("socket");
      exit(1);
    }
     
    // zero out the structure
    memset((char *) &s_sock, 0, sizeof(s_sock));

    s_sock.sin_family = AF_INET;
    s_sock.sin_port = htons(port_number);
    s_sock.sin_addr.s_addr = htonl(INADDR_ANY);

    //bind socket to port
    if(bind(sockfd, (struct sockaddr*)&s_sock, sizeof(s_sock) ) == -1)
    {
      perror("bind");
      exit(1);
    }
     
    bzero (buffer, BUFLEN);        
    //get test from client
    while (strcmp(buffer, "Test") != 0) { // while the input isn't "Test" as expected from the right client
      if ((recv_len = recvfrom(sockfd, buffer, BUFLEN, 0, (struct sockaddr *) &c_sock, &slen)) == -1)
      {
        perror("recvfrom()");
        exit(1);
      }
    }

    // send port back to client
    bzero(buffer, sizeof(buffer)); // clear buffer for use
    sprintf(buffer, "%d", ntohs(c_sock.sin_port)); // store client port into buffer
    if (sendto(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*) &c_sock, slen) == -1) // send buffer
    {
      perror("sendto()");
      exit(1);
    }

    // get segment from client
    if ((recv_len = recvfrom(sockfd, (struct Segment*)&segment, (sizeof(segment)), 0, (struct sockaddr *) &c_sock, &slen)) == -1)
    {
      perror("recvfrom()");
      exit(1);
    }

    // open file
    FILE* ifile = fopen("server.log", "w");
    if (!ifile) { perror("server.log"); }

    // print info
    printf("Input text: %s\n", segment.payload); fprintf(ifile, "Input text: %s\n", segment.payload);
    printf("Source Port: %d\n", segment.source_port); fprintf(ifile, "Source Port: %d\n", segment.source_port);
    printf("Destination Port: %d\n", segment.destination_port); fprintf(ifile, "Destination Port: %d\n", segment.destination_port);
    printf("Length: %d bytes\n", segment.length); fprintf(ifile, "Length: %d bytes\n", segment.length); 
    printf("Checksum: %d\n", segment.checksum); fprintf(ifile, "Checksum: %d\n", segment.checksum);
    printf("Payload: %ld bytes\n", sizeof(segment.payload)); fprintf(ifile, "Payload: %ld bytes\n", sizeof(segment.payload));

    // compute checksum from segment
    int checksum = getChecksum(segment);
    char hex[16]; bzero(hex, sizeof(hex));
    strcpy(hex, decimalToHex(checksum));
    printf("Computed Checksum: %d (%s)\n", checksum, hex); fprintf(ifile, "Computed Checksum: %d (%s)\n", checksum, hex);

    // check if checksum matches and print results
    if (checksum == segment.checksum) {
      printf("Checksum matched!\n"); fprintf(ifile, "Checksum matched!\n");
    } else {
      printf("Checksum mismatch!\n"); fprintf(ifile, "Checksum mismatch!\n");
    }

    fclose(ifile);
    printf("Server.log written\n");
 
    close(sockfd);

    return 0;
}

unsigned int getChecksum(struct Segment segment) {

    unsigned short int stored_checksum = segment.checksum; // store old checksum
    segment.checksum = 0; // replace with 0

    unsigned short int arr[264/2]; // 16 bits, length of segment is 264
    memcpy(&arr, &segment, segment.length);
    unsigned int sum = 0; // 32 bits
    
    for (int i = 0; i < (segment.length / 2) - 1; i++) {
      sum += arr[i];
    }

    unsigned int temp = sum; // copy sum into temp
    temp = temp >> 16; // get rid of non-overflow bits by shifting right 16 bits

    while (temp > 0) { // while sum is more than 16 bits ; largest number that can be held in 16 bits is 65536
    
        // perform the addition and store in sum
        temp = sum;
        temp = temp >> 16; // shift temp right 16 to get just the overflow bits
        sum = sum << 16; // shfit sum left 16 bits to get rid of the overflow bits
        sum = sum >> 16; // shift sum back to the right 16 bits, now filling the overflow space with 0's
        sum += temp; // add the two together

        // store the overflow bits in temp
        temp = sum;
        temp = temp >> 16; // shift temp right 16 bits to get rid of the non-overflow numbers

    }

    unsigned int negation = 65535; // equal to 32 1 bits in binary
    sum = sum ^ negation; // flip all the bits to get the one's complement
    sum << 16; // get rid of overflow from flipping bits, if any
    sum >> 16; // move back

    segment.checksum = stored_checksum; // restore the checksum

    return sum;

}

char* decimalToHex(unsigned int decimal) {

    char hex[16];
    bzero(hex, sizeof(hex));
    unsigned int q = decimal;

    unsigned int i = 0;
    while (i < 16 && q != 0) {
        unsigned int r = q % 16;
        unsigned int char_r = r;
        // convert to char
        if (r < 10) { // if is a number
            char_r += 48;
        } else { // if is a char
            char_r += 55;
        }

        hex[i] = char_r;
        i += 1;
        q = q / 16;
    }

    // reverse the string
    // find actual size of hex
    unsigned int size = 0;
    for (int j = 0; j < 16; j++) {
        if (hex[j] == '\0') {
            size = j;
            break;
        }
    }

    hex[size] = 'x';
    hex[size + 1] = '0';
    size += 2;

    // reverse string
    char result[16];
    bzero(result, sizeof(result));
    i = 0;
    for (int j = size - 1; j >= 0; j--) {
        result[j] = hex[i];
        i++;
    }

    char* p = result;

    return p;
}
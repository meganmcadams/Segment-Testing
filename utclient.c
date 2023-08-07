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

    struct Segment segment;

    char buffer[BUFLEN]; bzero(buffer, sizeof(buffer));
    char ip_address[BUFLEN]; bzero(ip_address, sizeof(ip_address));
    char user_input[BUFLEN]; bzero(user_input, sizeof(user_input));

    printf("Server IP address: "); scanf("%s", ip_address);
    printf("Port: "); scanf("%s", buffer); segment.destination_port = atoi(buffer);
    printf("Input text: "); scanf("%s", user_input); 
    bzero(segment.payload, sizeof(segment.payload));
    strcpy(segment.payload, user_input);

    struct sockaddr_in si_other;
    int sockfd, i = 0, slen = sizeof(si_other);
    char buf[BUFLEN];
    char message[BUFLEN];
 
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        die("socket");
    }
 
    memset((char *) &si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(segment.destination_port);
     
    if (inet_aton(ip_address, &si_other.sin_addr) == 0) {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }
 
    bzero(message, sizeof(message));
    strcpy(message, "Test");
        
    //send the message
    if (sendto(sockfd, message, strlen(message), 0, (struct sockaddr *) &si_other, slen) == -1) {
        die("sendto()");
    }

        
    //receive a reply and print it
    //clear the buffer by filling null, it might have previously received data
    bzero(buf, sizeof(buf));
    //try to receive some data, this is a blocking call

    bzero(buffer, sizeof(buffer));
    if (recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *) &si_other, &slen) == -1) {
        die("recvfrom()");
    }

    // make and open client.log
    FILE* ifile = fopen("client.log", "w");
    if (!ifile) { perror("client.log"); }

    segment.source_port = atoi(buffer); 
    printf("Source Port: %d\n", segment.source_port); fprintf(ifile, "Source Port: %d\n", segment.source_port);
    printf("Destination Port: %d\n", segment.destination_port); fprintf(ifile, "Destination Port: %d\n", segment.destination_port);

    // compute and store length
    segment.length = (16 * 4)/8 + sizeof(segment.payload);
    printf("Length: %d bytes\n", segment.length); fprintf(ifile, "Length: %d bytes\n", segment.length);

    // compute and store checksum
    segment.checksum = getChecksum(segment);
    char hex[16]; bzero(hex, sizeof(hex));
    strcpy(hex, decimalToHex(segment.checksum));
    printf("Checksum: %d (%s)\n", segment.checksum, hex); fprintf(ifile, "Checksum: %d (%s)\n", segment.checksum, hex);
    printf("Payload: %ld bytes\n", sizeof(segment.payload)); fprintf(ifile, "Payload: %ld bytes\n", sizeof(segment.payload));

    // send segment
    if (sendto(sockfd, (struct Segment*)&segment, (1024+sizeof(segment)), 0, (struct sockaddr *) &si_other, slen) == -1) {
        die("sendto()");
    }

    // write to client.log
    fclose(ifile);
    printf("Client.log written\n");
 
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
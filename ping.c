#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define ICMP_ECHO_REQUEST 8
#define ICMP_ECHO_REPLY 0
#define ICMP 0

/*
 * Header:
 * +-------+-------+----------+----------------------------------------+--------+
 * |  Typ  | Code  | Checksum |                 Data                   | Total  |
 * +-------+-------+----------+----------------------------------------+--------+
 * | 8 Bit | 8 Bit | 16 Bit   | 16 Bit Identifier + 16 Bit Sequence ID | 64 Bit |
 * +-------+-------+----------+----------------------------------------+--------+
 */
struct icmp
{
    /* Header */
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t id;
    uint16_t seq;

    /* Data */
    double ts;
};

/* IP Header without options */
struct ip_header
{
    uint8_t version : 4;
    uint8_t ihl : 4;
    uint8_t tos;
    uint16_t len;
    uint16_t id;
    uint16_t flags : 2;
    uint16_t fragment_offset : 14;
    uint8_t ttl;
    uint8_t proto;
    uint16_t checksum;
    uint32_t source;
    uint32_t dest;
    /* Ignore Options and Padding*/

};

double time_current_microseconds()
{
    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);
    return tv.tv_sec + ((double) tv.tv_usec) / 1000000;
}


static uint16_t calc_checksum(unsigned char *buffer, int bytes)
{
    if (bytes <= 0)
    {
        return -1;
    }

    uint32_t checksum = 0;
    unsigned char *end = buffer + bytes;

    if (bytes % 2 == 1)
    {
        end = buffer + bytes - 1;
        checksum += (*end) << 8;
    }

    while (buffer < end)
    {
        checksum += (buffer[0] << 8) + buffer[1];
        buffer += 2;
    }

    checksum = (checksum >> 16) + (checksum & 0xffff);
    checksum += (checksum >> 16);
    return ~checksum;

}

int recv_echo_reply(int sock, int id)
{
    /* Allocate a buffer for up to 1500 bytes (MTU) */
    char buf[1500];
    /* Store the remote address */
    struct sockaddr_in remote_addr;

    /* Recv data from socket */
    unsigned int s = sizeof(remote_addr);
    int n_bytes_recv = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr *) &remote_addr, &s);

    if (n_bytes_recv == -1)
    {
        return -1;
    }

    /* Skip the IP header (20 bytes) */
    struct ip_header ip_header = *(struct ip_header *) buf;
    struct icmp icmp_recv = *(struct icmp *) (buf + 20);

    /* Check for echo reply packet */
    if (icmp_recv.type != ICMP_ECHO_REPLY || icmp_recv.code != 0)
    {
        return -1;
    }

    /* Check if ids match */
    if (icmp_recv.id != id)
    {
        return -1;
    }

    printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%lf ms\n", n_bytes_recv, inet_ntoa(remote_addr.sin_addr),
           icmp_recv.seq, ip_header.ttl, (time_current_microseconds() - icmp_recv.ts) * 1000);

    return 0;
}

int send_echo_request(int sock, struct sockaddr_in *addr, int id, int seq)
{
    /* Allocate memory and set all fields to 0 */
    struct icmp icmp;
    memset(&icmp, 0, sizeof(icmp));

    /* Header */
    icmp.type = ICMP_ECHO_REQUEST;
    icmp.code = ICMP;
    icmp.id = id;
    icmp.seq = seq;

    /* Store the timestamp for later use */
    icmp.ts = time_current_microseconds();

    /* Calculate the checksum */
    int checksum = calc_checksum((unsigned char *) &icmp, sizeof(icmp));

    /* Convert checksum from host byteorder to network byteorder */
    icmp.checksum = htons(checksum);

    /* Send it */
    if (sendto(sock, &icmp, sizeof(icmp), 0, (struct sockaddr *) addr, sizeof(addr)) == -1)
    {
        return -1;
    }
    return 0;
}

int ping(const char *ip)
{
    /* Open a socket */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    /* IPv4 + Port 0 */
    addr.sin_family = AF_INET;
    addr.sin_port = 0;

    /* Convert IPv4 address from dot notation into binary form */
    if (inet_aton(ip, (struct in_addr *) &addr.sin_addr.s_addr) == 0)
    {
        fprintf(stderr, "Invalid Ipv4 address: \"%s\"\n", ip);
        return -1;
    }

    /* Open a raw socket for IPv4 */
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock == -1)
    {
        fprintf(stderr, "Could not open socket.");
        return -1;
    }

    /* Set the timeout to RECV_TIMEOUT*/
    struct timeval tv = {0, 100000};
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1)
    {
        return -1;
    }
    int id = getpid();
    int seq = 1;
    double timeout = 0;
    double now;

    /* Start pinging */
    while (1)
    {
        now = time_current_microseconds();
        if (now >= timeout)
        {
            if (send_echo_request(sock, &addr, id, seq) == -1)
            {
                return -1;
            }

            timeout = now + 1;
        }
        // try to receive and print reply
        recv_echo_reply(sock, id);
    }
}

void print_usage()
{
    printf("Please supply a valid IPv4 address.");
}

int main(int argc, char *argv[])
{
    /* No or a invalid ip provided */
    if (argc != 2 || inet_aton(argv[1], NULL) == 0)
    {
        print_usage();
        return 0;
    }

    printf("Start pinging %s:\n", argv[1]);
    ping(argv[1]);
}

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

char *getPublicIP(int IPver) {
  int IPlen, addrLen;
  char *stunServer;
  char *res;
  struct sockaddr_storage addr;
  memset(&addr, 0, sizeof(struct sockaddr_storage));
  switch (IPver) {
  case 4:
    IPlen = 4;
    stunServer = "141.101.90.0";
    struct sockaddr_in *addr_v4 = (struct sockaddr_in *)&addr;
    addr_v4->sin_family = AF_INET;
    addr_v4->sin_port = htons(3478);
    inet_aton(stunServer, &(addr_v4->sin_addr));
    res = malloc(sizeof(char) * INET_ADDRSTRLEN);
    break;
  case 6:
    IPlen = 16;
    stunServer = "2a06:98c1:3200::";
    struct sockaddr_in6 *addr_v6 = (struct sockaddr_in6 *)&addr;
    addr_v6->sin6_family = AF_INET6;
    addr_v6->sin6_port = htons(3478);
    inet_pton(AF_INET6, stunServer, &(addr_v6->sin6_addr));
    res = malloc(sizeof(char) * INET6_ADDRSTRLEN);
    break;
  default:
    return NULL;
  }

  int sockfd = socket(AF_INET6, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    // perror("socket");
    free(res);
    return NULL;
  }

  unsigned char request[] = {0x00, 0x01, 0x00, 0x00, 0x21, 0x12, 0xA4,
                             0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  if (sendto(sockfd, request, sizeof(request), 0, (struct sockaddr *)&addr,
             sizeof(struct sockaddr_storage)) < 0) {
    // perror("sendto");
    shutdown(sockfd, SHUT_RDWR);
    return NULL;
  }

  unsigned char response[44];
  struct sockaddr_storage fromAddr;
  socklen_t fromLen = sizeof(fromAddr);
  if (recvfrom(sockfd, response, sizeof(response), 0,
               (struct sockaddr *)&fromAddr, &fromLen) < 0) {
    // perror("recvfrom");
    shutdown(sockfd, SHUT_RDWR);
    free(res);
    return NULL;
  }

  shutdown(sockfd, SHUT_RDWR);

  unsigned char ip[IPlen];
  memcpy(ip, &response[28], IPlen);
  if (response[21] == 0x20) {
    for (int i = 0; i < 4; i++) {
      ip[i] ^= request[4 + i];
    }
  }

  if (inet_ntop(addr.ss_family, ip, res,
                IPver == 4 ? INET_ADDRSTRLEN : INET6_ADDRSTRLEN) == NULL) {
    perror("inet_ntop");
    return NULL;
  }

  return res;
}

int main(int argc, char *argv[]) {
  char *ip;
  if (argc != 2) {
    if ((ip = getPublicIP(4)) != NULL) {
      printf("%s\n", ip);
    } else {
      printf("No IPv4\n");
      return 1;
    }
    return 0;
  }
  char *arg = argv[1];
  if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
    printf("Get public IP address from STUN server(default: "
           "stun.cloudflare.com:3478)\n");
    printf("Usage: %s [OPTIONS]\n", argv[0]);
    printf("OPTIONS: \n");
    printf("  -4, Get public IPv4 address\n");
    printf("  -6, Get public IPv6 address\n");
    printf("  -h, --help Display help information\n");
    return 0;
  }
  if (strlen(arg) != 2 || arg[0] != '-' || (arg[1] != '4' && arg[1] != '6')) {
    printf("Invalid IP argsion argument: %s\n", arg);
    printf("Excepted: -4 (IPv4) or -6 (IPv6)\n");
    return 1;
  }
  if ((ip = getPublicIP(arg[1] - '0')) != NULL) {
    printf("%s\n", ip);
  } else {
    printf("No IPv%d\n", arg[1] - '0');
    return 1;
  }

  return 0;
}

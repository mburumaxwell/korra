#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(korra_net_utils, LOG_LEVEL_INF);

#include "korra_net_utils.h"

int korra_resolve_address(const char *host, uint16_t port, int family, int socktype, struct sockaddr *addr,
                          socklen_t *addrlen) {
  struct addrinfo hints = {
      .ai_family = family,
      .ai_socktype = socktype,
  };

  struct addrinfo *res = NULL;
  LOG_DBG("getaddrinfo for %s", host);
  int ret = getaddrinfo(host, NULL, &hints, &res);
  if (ret) {
    LOG_INF("getaddrinfo for %s failed (%d, errno %d)", host, ret, errno);
    return ret;
  }

  // store the first result
  *addr = *(res->ai_addr);
  *addrlen = res->ai_addrlen;

  // free the allocated memory
  freeaddrinfo(res);

  // store the port
  net_sin(addr)->sin_port = htons(port);

  // print out the resolved address
  char addr_str[INET6_ADDRSTRLEN] = {0};
  inet_ntop(addr->sa_family, &net_sin(addr)->sin_addr, addr_str, sizeof(addr_str));
  LOG_DBG("%s -> %s", host, addr_str);

  return 0;
}

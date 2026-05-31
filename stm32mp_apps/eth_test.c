#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ETH_IFACE "end0"
#define ETH_PATH "/sys/class/net/end0"
#define ETH_OPERSTATE "/sys/class/net/end0/operstate"

int path_exists(const char *path) {
    return access(path, F_OK) == 0;
}

int read_first_line(const char *path, char *buf, size_t size) {
    FILE *f = fopen(path, "r");
    if (!f) return 0;

    if (!fgets(buf, size, f)) {
        fclose(f);
        return 0;
    }

    fclose(f);
    return 1;
}

int command_ok(const char *cmd) {
    int ret = system(cmd);
    return ret == 0;
}

int main(void) {
    int iface_ok = 0;
    int link_ok = 0;
    int ip_ok = 0;
    int internet_ok = 0;

    char state[128] = {0};

    printf("=== ETHERNET TEST ===\n");

    if (path_exists(ETH_PATH)) {
        iface_ok = 1;
        printf("Ethernet interface %s: found\n", ETH_IFACE);
    } else {
        printf("Ethernet interface %s: not found\n", ETH_IFACE);
    }

    if (read_first_line(ETH_OPERSTATE, state, sizeof(state))) {
        printf("Ethernet operstate: %s", state);

        if (strstr(state, "up"))
            link_ok = 1;
    } else {
        printf("Cannot read ethernet operstate\n");
    }

    ip_ok = command_ok("ip -4 addr show end0 | grep -q 'inet '");
    printf("Ethernet IPv4: %s\n", ip_ok ? "found" : "not found");

    internet_ok = command_ok("ping -c 1 -W 3 8.8.8.8 > /dev/null 2>&1");
    printf("Ethernet ping 8.8.8.8: %s\n", internet_ok ? "success" : "failed");

    printf("\n==============================\n");
    printf("ETH interface : %s\n", iface_ok ? "PASS" : "FAIL");
    printf("ETH link      : %s\n", link_ok ? "PASS" : "FAIL");
    printf("ETH IP        : %s\n", ip_ok ? "PASS" : "FAIL");
    printf("ETH internet  : %s\n", internet_ok ? "PASS" : "FAIL");

    if (iface_ok && link_ok && ip_ok && internet_ok)
        printf("RESULT        : ETHERNET PASS\n");
    else
        printf("RESULT        : ETHERNET FAIL\n");

    printf("==============================\n");

    return (iface_ok && link_ok && ip_ok && internet_ok) ? 0 : 1;
}
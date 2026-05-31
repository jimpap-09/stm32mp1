#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define HDMI_STATUS "/sys/class/drm/card0-HDMI-A-1/status"
#define HDMI_MODES  "/sys/class/drm/card0-HDMI-A-1/modes"
#define HDMI_EDID   "/sys/class/drm/card0-HDMI-A-1/edid"

int file_exists(const char *path) {
    return access(path, F_OK) == 0;
}

int text_file_has_content(const char *path) {
    FILE *f = fopen(path, "r");
    char buf[128];

    if (!f) return 0;

    if (fgets(buf, sizeof(buf), f) == NULL) {
        fclose(f);
        return 0;
    }

    fclose(f);
    return 1;
}

int binary_file_has_content(const char *path) {
    FILE *f = fopen(path, "rb");
    unsigned char buf[128];

    if (!f) return 0;

    size_t n = fread(buf, 1, sizeof(buf), f);
    fclose(f);

    return n > 0;
}

int main(void) {
    int status_ok = 0;
    int modes_ok = 0;
    int edid_ok = 0;

    char status[128] = {0};

    printf("=== HDMI TEST ===\n");

    FILE *f = fopen(HDMI_STATUS, "r");
    if (f) {
        if (fgets(status, sizeof(status), f)) {
            printf("HDMI status raw: %s", status);

            if (strstr(status, "connected"))
                status_ok = 1;
        }
        fclose(f);
    } else {
        printf("Cannot open HDMI status path\n");
    }

    if (file_exists(HDMI_MODES) && text_file_has_content(HDMI_MODES)) {
        modes_ok = 1;
        printf("HDMI modes: available\n");
    } else {
        printf("HDMI modes: not available\n");
    }

    if (file_exists(HDMI_EDID) && binary_file_has_content(HDMI_EDID)) {
        edid_ok = 1;
        printf("HDMI EDID: readable binary data\n");
    } else {
        printf("HDMI EDID: not readable\n");
    }

    printf("\n==============================\n");
    printf("HDMI status   : %s\n", status_ok ? "PASS" : "FAIL");
    printf("HDMI modes    : %s\n", modes_ok ? "PASS" : "FAIL");
    printf("EDID readable : %s\n", edid_ok ? "PASS" : "FAIL");

    if (status_ok && modes_ok && edid_ok)
        printf("RESULT        : HDMI PASS\n");
    else
        printf("RESULT        : HDMI FAIL\n");

    printf("==============================\n");

    return (status_ok && modes_ok && edid_ok) ? 0 : 1;
}
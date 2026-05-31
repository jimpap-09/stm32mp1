#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int path_exists(const char *path) {
    return access(path, F_OK) == 0;
}

int command_ok(const char *cmd) {
    int ret = system(cmd);
    return ret == 0;
}

int main(void) {
    int usb_bus_ok = 0;
    int hid_ok = 0;
    int storage_ok = 0;

    printf("=== USB TEST ===\n");

    usb_bus_ok = path_exists("/sys/bus/usb/devices");
    printf("USB bus sysfs: %s\n", usb_bus_ok ? "found" : "not found");

    hid_ok = command_ok("lsusb | grep -Ei 'keyboard|mouse|logitech|lenovo|hid' > /dev/null 2>&1");
    printf("USB HID device: %s\n", hid_ok ? "found" : "not found");

    storage_ok = path_exists("/dev/sda") || path_exists("/dev/sda1");
    printf("USB storage device: %s\n", storage_ok ? "found" : "not found");

    printf("\n==============================\n");
    printf("USB bus       : %s\n", usb_bus_ok ? "PASS" : "FAIL");
    printf("USB HID       : %s\n", hid_ok ? "PASS" : "FAIL");
    printf("USB storage   : %s\n", storage_ok ? "PASS" : "FAIL");

    if (usb_bus_ok && hid_ok && storage_ok)
        printf("RESULT        : USB PASS\n");
    else
        printf("RESULT        : USB FAIL\n");

    printf("==============================\n");

    return (usb_bus_ok && hid_ok && storage_ok) ? 0 : 1;
}
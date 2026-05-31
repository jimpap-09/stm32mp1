#!/bin/bash
source ~/stm32_sdk/environment-setup-cortexa7t2hf-neon-vfpv4-ostl-linux-gnueabi
$CC hdmi_test.c -o hdmi_test
$CC eth_test.c -o eth_test
$CC usb_test.c -o usb_test
file hdmi_test
file eth_test
file usb_test
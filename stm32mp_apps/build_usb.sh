#!/bin/bash
source ~/stm32_sdk/environment-setup-cortexa7t2hf-neon-vfpv4-ostl-linux-gnueabi
$CC usb_test.c -o usb_test
file usb_test

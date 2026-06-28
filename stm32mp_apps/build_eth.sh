#!/bin/bash
source ~/stm32_sdk/environment-setup-cortexa7t2hf-neon-vfpv4-ostl-linux-gnueabi
$CC eth_test.c -o eth_test
file eth_test

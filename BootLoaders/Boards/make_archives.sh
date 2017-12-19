#!/bin/bash

AVR_VERSION=$(grep "^version=[0-9].[0-9].[0-9]" "avr/platform.txt" | awk -F = '{ print $2 }')
STM_VERSION=$(grep "^version=[0-9].[0-9].[0-9]" "stm32/platform.txt" | awk -F = '{ print $2 }')

echo
echo "AVR Version: $AVR_VERSION"
echo "Creating archive 'package_multi_4in1_avr_board_v$AVR_VERSION.tar.gz'"
tar -czf ../Archives/package_multi_4in1_avr_board_v$AVR_VERSION.tar.gz --transform s/avr/package_multi_4in1_avr_board_v$AVR_VERSION/ avr/*
sleep 1s
echo
echo "Package: package_multi_4in1_avr_board_v$AVR_VERSION.tar.gz"
echo "SHA256:  `(sha256sum ../Archives/package_multi_4in1_avr_board_v$AVR_VERSION.tar.gz | awk -v N=1 '{print $N}')`"
echo "Size:    `(ls -al ../Archives/package_multi_4in1_avr_board_v$AVR_VERSION.tar.gz | awk -v N=5 '{print $N}')`"
echo
echo "STM Version: $STM_VERSION"
echo "Creating archive 'package_multi_4in1_stm32_board_v$STM_VERSION.tar.gz'"
tar -czf ../Archives/package_multi_4in1_stm32_board_v$STM_VERSION.tar.gz --transform s/stm32/package_multi_4in1_stm32_board_v$STM_VERSION/ stm32/*
sleep 1s
echo
echo "Package: package_multi_4in1_stm_board_v$STM_VERSION.tar.gz"
echo "SHA256:  `(sha256sum ../Archives/package_multi_4in1_stm32_board_v$STM_VERSION.tar.gz | awk -v N=1 '{print $N}')`"
echo "Size:    `(ls -al ../Archives/package_multi_4in1_stm32_board_v$STM_VERSION.tar.gz | awk -v N=5 '{print $N}')`"
echo
echo Done
echo

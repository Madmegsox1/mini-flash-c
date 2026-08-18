
clang \
    -Wall -Wextra -Wpedantic \
    -std=c11 \
    -g3 -O0 \
    -fsanitize=address,undefined \
    -fno-omit-frame-pointer \
    main.c usb.c flash.c pico.c \
    -o pico_flash \
    $(pkg-config --cflags --libs libusb-1.0)

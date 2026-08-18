#ifndef PICOBOOT_FLASH
#define PICOBOOT_FLASH
#include <libusb-1.0/libusb.h>
#include <stddef.h>
#include <stdint.h>
#define FLASH_ADDRESS 0x10000000u
#define FLASH_PAGE_SIZE 256u
#define FLASH_SECTOR_SIZE 4096u 

struct pico_firmware {
  size_t erase_size;
  size_t firmware_size;
  uint8_t ** firmware;
};

int verify_pico_firmware(libusb_device_handle *dev, int iface, uint8_t ep_in, uint8_t ep_out, struct pico_firmware firmware);
int flash_pico(libusb_device_handle *dev, int iface, uint8_t ep_in, uint8_t ep_out, struct pico_firmware firmware);


#endif

#include "flash.h"
#include "pico.h"
#include <libusb-1.0/libusb.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int flash_pico(libusb_device_handle *dev, int iface, uint8_t ep_in, uint8_t ep_out, struct pico_firmware firmware){
  if(picoboot_reset(dev, iface, ep_in, ep_out) != 0) {
    printf("Reset failed \n");
    return 1;
  }
  
  int ret = picoboot_exclusive(dev, iface, ep_in, ep_out, 1);

  if(ret != 0){
    printf("Exclusive command failed %d\n", ret);
    return 1;
  }

  ret = picoboot_exit_xip(dev, iface, ep_in, ep_out);

  if(ret != 0){
    printf("EXIT_XIP failed: %d\n", ret);
    return ret;
  }

  // Erase in blocks: the ACK only arrives once the erase finishes, so one big
  // erase can outlast the USB timeout
  for(size_t offset = 0; offset < firmware.erase_size; offset += FLASH_BLOCK_SIZE){
    size_t remaining = firmware.erase_size - offset;
    size_t erase_size = remaining < FLASH_BLOCK_SIZE ? remaining : FLASH_BLOCK_SIZE;

    uint32_t address = FLASH_ADDRESS + (uint32_t) offset;

    ret = picoboot_flash_erase(dev, iface, ep_in, ep_out, address, erase_size);

    if(ret != 0) {
      printf("\nFailed to erase the flash at 0x%08x : %d\n", address, ret);
      return ret;
    }

    printf("\rErasing: %zu / %zu", offset + erase_size, firmware.erase_size);

    fflush(stdout);
  }

  printf("\nErase finished\n");

  uint8_t page[FLASH_PAGE_SIZE];
  
  for(size_t offset = 0; offset < (size_t)firmware.firmware_size; offset += FLASH_PAGE_SIZE){
    memset(page, 0xff, sizeof(page));

    size_t remaining = firmware.firmware_size - offset;
    size_t copy_size = remaining < FLASH_PAGE_SIZE ? remaining : FLASH_PAGE_SIZE;

    memcpy(page, *(firmware.firmware) + offset, copy_size);

    uint32_t address = FLASH_ADDRESS + (uint32_t) offset;

    ret = picoboot_write(dev, iface, ep_in, ep_out, address, page, FLASH_PAGE_SIZE);

    if(ret != 0){
      printf("WRITE Failed at 0x%08x : %d\n", address, ret );
      return ret;
     }

     printf("\rWriting: %zu / %zu", offset + copy_size, firmware.firmware_size);

     fflush(stdout);
  }

  printf("\nWriting finished\n");

  ret = picoboot_enter_xip(dev, iface, ep_in, ep_out);

  if(ret != 0){
    printf("Failed to Enter XIP\n");
    return ret;
  }

  if(verify_pico_firmware(dev, iface, ep_in, ep_out, firmware) != 0){
    printf("Failed to verify firmware");
    return 1;
  }

  printf("Firmware verified\nRebooting\n");
  ret = picoboot_reboot_flash(dev, iface, ep_in, ep_out);

  if(ret != 0) {
    printf("Failed to reboot\n");
    return 1;
  }

  return ret;
}


int verify_pico_firmware(libusb_device_handle *dev, int iface, uint8_t ep_in, uint8_t ep_out, struct pico_firmware firmware){

  uint8_t verify[FLASH_PAGE_SIZE];
  int ret;

  for(size_t offset = 0; offset < (size_t)firmware.firmware_size; offset += FLASH_PAGE_SIZE) {
    size_t remaining = firmware.firmware_size - offset;
    size_t amount = remaining < FLASH_PAGE_SIZE ? remaining : FLASH_PAGE_SIZE;

    uint32_t address = FLASH_ADDRESS + (uint32_t) offset;

    ret = picoboot_read(dev, iface, ep_in, ep_out, address, verify, amount);
    if(ret != 0){
      printf("Failed to Read firmware\n");
      return ret;
    }

    if(memcmp(verify, *(firmware.firmware)+ offset, amount) != 0){
      printf("Failed to verify at 0x%08x\n", address);
      return 1;
    }
    
  }

  return 0;
}

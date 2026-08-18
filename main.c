#include <libusb-1.0/libusb.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "usb.h"
#include "flash.h"

#define RP_VID 0x2e8a
#define RP2350_BOOT_PID 0x000f


int main( void ){
  FILE *firmware = fopen("./test-firmware.bin", "rb");
  fseek(firmware, 0, SEEK_END);

  long firmware_size = ftell(firmware);
  rewind(firmware);

  uint8_t *firmware_byte = malloc(firmware_size);
  fread(firmware_byte, 1, firmware_size, firmware);

  fclose(firmware);

  uint32_t erase_size = (firmware_size + FLASH_SECTOR_SIZE) & ~(FLASH_SECTOR_SIZE - 1);
  
  printf("Firmware size: %zu bytes\n", firmware_size);
  printf("Erasing:       %u bytes\n", erase_size);

  libusb_device_handle *dev;
  libusb_context *ctx = NULL;
  usb_create_handle(RP_VID, RP2350_BOOT_PID, &dev, &ctx);

  if(!dev){
    printf("Failed to get device handle \n");
    return 1;
  }
  
  uint8_t eps[2] = {0,0};

  int iface = usb_enumirate_inf(dev, 0xff, eps, sizeof(eps));

  uint8_t ep_out, ep_in;

  if(eps[0] & LIBUSB_ENDPOINT_IN) ep_in = eps[0];
  else ep_out = eps[0];

  if(eps[1] & LIBUSB_ENDPOINT_IN) ep_in = eps[1];
  else ep_out = eps[1];


  if(iface < 0) {
    printf("Failed to get interface\n");
    return 1;
  }

  if(usb_claim_inf(dev,iface) != 0){
    printf("Failed to claim interface\n");
    return 1;
  }

  struct pico_firmware pico_firm = {erase_size, firmware_size, &firmware_byte};

  int flash = flash_pico(dev, iface, ep_in, ep_out, pico_firm);

  if(flash != 0){
    printf("Failed to flash pico\n");
    return 1;
  }

  free(firmware_byte);

  libusb_close(dev);
  libusb_exit(ctx);

  // we need to wait for the microcontroller to come online

  size_t count = 0;
  FILE* output = NULL;
  while(output == NULL)
  {
    output = fopen("/dev/ttyACM0", "rb");
    if(output != NULL) break;
    count++;
    printf("\rTying to open output (Trys: %zu)", count);
    fflush(stdout);
    sleep(1);
  }

  printf("\nSuccessfuly got connection from /dev/ttyACM0\n");

  if(output == NULL){
    printf("Failed to open device\n");
    return 1;
  }

  int c;

  while((c = fgetc(output)) != EOF) {
    putchar(c);
    fflush(stdout);
  }

  if(ferror(output)){
    perror("fread");
  }

  fclose(output);

  return 0;
}


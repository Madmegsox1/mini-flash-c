#include <stdio.h>
#include <string.h>
#include "pico.h"


int picoboot_reboot_flash(libusb_device_handle *dev, int iface, uint8_t ep_in, uint8_t ep_out){

  struct picoboot_cmd cmd = {0};
  cmd.command = PC_REBOOT2;
  cmd.command_size = 16;
  cmd.transfer_length = 0;

  cmd.args.reboot2.flags = 0x04;

  cmd.args.reboot2.delay_ms = 100;
  cmd.args.reboot2.param0 = FLASH_ADDRESS;
  cmd.args.reboot2.param1 = 0;

  return picoboot_command(dev, iface, ep_in, ep_out, &cmd, NULL);
}


int picoboot_enter_xip(libusb_device_handle *dev, int iface, uint8_t ep_in, uint8_t ep_out){
  struct picoboot_cmd cmd = {0};
  cmd.command = PC_ENTER_CMD_XIP;
  cmd.command_size = 0;
  cmd.transfer_length = 0;
  // i love you  awww


  return picoboot_command(dev, iface, ep_in, ep_out, &cmd, NULL);
  
}

int picoboot_exit_xip(libusb_device_handle *dev, int iface, uint8_t ep_in, uint8_t ep_out){
  struct picoboot_cmd cmd = {0};
  cmd.command = PC_EXIT_XIP;
  cmd.command_size = 0;
  cmd.transfer_length = 0;


  return picoboot_command(dev, iface, ep_in, ep_out, &cmd, NULL);
  
}

int picoboot_flash_erase(libusb_device_handle *dev, int iface, uint8_t ep_in, uint8_t ep_out, uint32_t address, uint32_t size) {
  struct picoboot_cmd cmd = {0};
  cmd.command = PC_FLASH_ERASE;
  cmd.command_size = 8;
  cmd.transfer_length = 0;

  cmd.args.range.addr = address;
  cmd.args.range.size = size;

  return picoboot_command(dev, iface, ep_in, ep_out, &cmd, NULL);
}

int picoboot_write(libusb_device_handle *dev, int iface, uint8_t ep_in, uint8_t ep_out, uint32_t address, void * data, uint32_t size) {
  struct picoboot_cmd cmd = {0};
  cmd.command = PC_WRITE;
  cmd.command_size = 8;
  cmd.transfer_length = size;

  cmd.args.range.addr = address;
  cmd.args.range.size = size;

  return picoboot_command(dev, iface, ep_in, ep_out, &cmd, data);
}

int picoboot_read(libusb_device_handle *dev, int iface, uint8_t ep_in, uint8_t ep_out, uint32_t address, void * buffer, uint32_t size){

  struct picoboot_cmd cmd = {0};
  cmd.command = PC_READ;
  cmd.command_size = 8;
  cmd.transfer_length = size;

  cmd.args.range.addr = address;
  cmd.args.range.size = size;

  return picoboot_command(dev, iface, ep_in, ep_out, &cmd, buffer);
}


int picoboot_exclusive(libusb_device_handle *dev, int iface, uint8_t ep_in, uint8_t ep_out, uint8_t exclusive){
  struct picoboot_cmd cmd = {0};
  cmd.command = PC_EXCLUSIVE_ACCESS;
  cmd.command_size = 1;
  cmd.transfer_length = 0;

  cmd.args.exclusive.exclusive = exclusive;
  
  return picoboot_command(dev, iface, ep_in, ep_out, &cmd, NULL);
}


// wrapper to reset pico
int picoboot_reset(libusb_device_handle *dev, int iface, uint8_t ep_in, uint8_t ep_out){
  libusb_clear_halt(dev, ep_in);
  libusb_clear_halt(dev, ep_out);

  int ret = libusb_control_transfer(dev,
                                     LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_OUT,
                                     PICOBOOT_INF_RESET,
                                      0,
                                       iface,
                                        NULL,
                                         0,
                                          1000);

  if(ret < 0){
    return ret;
  }
  return 0;
}


// Wrapper to get status
int picoboot_get_status (libusb_device_handle *dev, int iface, struct picoboot_cmd_status *status) {
  memset(status, 0, sizeof(*status));

  int ret = libusb_control_transfer(dev,
                                     LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR| LIBUSB_RECIPIENT_INTERFACE,
                                     PICOBOOT_INF_CMD_STATUS,
                                      0,
                                       iface,
                                        (unsigned char *) status,
                                         sizeof(*status),
                                         1000);
  if(ret < 0){
    return ret;
  }


  if(ret != sizeof(*status)){
    return 1;
  }

  
  return 0;
  
}


int picoboot_command (libusb_device_handle *dev, int iface, uint8_t ep_in, uint8_t ep_out, struct picoboot_cmd *cmd, void *data){
  int ret;
  int transferred = 0;

  cmd->magic = PICOBOOT_MAGIC;
  cmd->token = next_token++;

  // send inital command
  ret = libusb_bulk_transfer(dev, ep_out, (unsigned char *)cmd, sizeof(*cmd), &transferred, 3000);

  if(ret < 0){
    return ret;
  }

  if(transferred != sizeof(*cmd)){
    return LIBUSB_ERROR_IO;
  }

  // Apparently bit7 of the command ID means data direction is IN
  int command_is_in = (cmd->command & 0x80) != 0;

  // send data packet if we need to
  if(cmd->transfer_length != 0){
    transferred = 0;
    uint8_t endpoint = command_is_in ? ep_in : ep_out;

    libusb_bulk_transfer(dev, endpoint, (unsigned char *) data, cmd->transfer_length, &transferred, 10000);

    if(ret < 0){
      struct picoboot_cmd_status status;

      if(picoboot_get_status(dev, iface, &status) == 0){
        printf("Failed running command %u with status code %u", status.command, status.status_code);
      }
      
      return ret;
    }

    if((uint32_t)transferred != cmd->transfer_length){
      return LIBUSB_ERROR_IO;
    }
  }

  // ACK packets

  unsigned char empty = 0;
  transferred = 0;

  if(command_is_in){
    // Just got a command in so ACK goes back out
    ret = libusb_bulk_transfer(dev, ep_out, &empty, 0, &transferred, 3000);
  }
  else{
    // Just sent a command so should get a ACK back in
    ret = libusb_bulk_transfer(dev, ep_in, &empty, 1, &transferred, 3000);
  }


  if(ret < 0){
    printf("ACK FAILED");
    struct picoboot_cmd_status status;

    if(picoboot_get_status(dev, iface, &status) == 0){
      printf("Failed running command %u with status code %u", status.command, status.status_code);
    }

    return ret;
  }

  if(DEBUG_ALL_COMMANDS == 1){

    struct picoboot_cmd_status status;

    ret = picoboot_get_status(dev, iface, &status);

    if(ret < 0) return ret;

    if(VEBOSE_DEBUG_ALL_COMMANDS == 1){
      printf("Status Code: %u\nCommand: 0x%02x\nToken: %08x\n", status.status_code, status.command, status.token);
      
    }

    if(status.token != cmd->token){
      printf("Token Mismatch: send %08x got %08x\n", cmd->token, status.token);
    }

    if(status.status_code != PICOBOOT_OK){
      printf("Picoboot command 0x%02x failed, status = %u \n", status.command, status.status_code);
    }
    
  }

  return 0;
}

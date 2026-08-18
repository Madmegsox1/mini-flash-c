#ifndef PICOBOOT_PICO
#define PICOBOOT_PICO

#include <libusb-1.0/libusb.h>
#include "flash.h"

#define PICOBOOT_MAGIC 0x431fd10b
#define PICOBOOT_INF_RESET 0x41
#define PICOBOOT_INF_CMD_STATUS 0x42


#define PICOBOOT_FLASH_UPDATE_FLAG 0x04

#define DEBUG_ALL_COMMANDS 1
#define VEBOSE_DEBUG_ALL_COMMANDS 0

#pragma pack(push, 1)

struct picoboot_cmd_status {
  uint32_t token;
  uint32_t status_code;

  uint8_t command;
  uint8_t in_progress;

  uint8_t padding[6];
  
};


struct picoboot_cmd {
  uint32_t magic;
  uint32_t token;

  uint8_t command;
  uint8_t command_size;

  uint16_t reserved;

  uint32_t transfer_length;

  union {
    uint8_t raw[16];

    struct {
      uint32_t addr;
      uint32_t size;
    } range;

    struct {
      uint32_t flags;
      uint32_t delay_ms;
      uint32_t param0;
      uint32_t param1;
    } reboot2;

    struct {
      uint8_t exclusive;
    } exclusive;
    
  } args;
};

#pragma pack(pop)

_Static_assert(sizeof(struct picoboot_cmd) == 32,
               "picoboot_cmd must be 32 bytes");

_Static_assert(sizeof(struct picoboot_cmd_status) == 16,
               "picoboot_cmd_status must be 16 bytes");

// token counter
static uint32_t next_token = 1;
// resets pico
int picoboot_reset(libusb_device_handle *dev, int iface, uint8_t ep_in, uint8_t ep_out);
// gets status of last command
int picoboot_get_status (libusb_device_handle *dev, int iface, struct picoboot_cmd_status *status);
// send command 
int picoboot_command (libusb_device_handle *dev, int iface, uint8_t ep_in, uint8_t ep_out, struct picoboot_cmd *cmd, void *data);

int picoboot_exclusive(libusb_device_handle *dev, int iface, uint8_t ep_in, uint8_t ep_out, uint8_t exclusive);

int picoboot_read(libusb_device_handle *dev, int iface, uint8_t ep_in, uint8_t ep_out, uint32_t address, void * buffer, uint32_t size);

int picoboot_write(libusb_device_handle *dev, int iface, uint8_t ep_in, uint8_t ep_out, uint32_t address, void * data, uint32_t size);

int picoboot_flash_erase(libusb_device_handle *dev, int iface, uint8_t ep_in, uint8_t ep_out, uint32_t address, uint32_t size);

int picoboot_exit_xip(libusb_device_handle *dev, int iface, uint8_t ep_in, uint8_t ep_out);

int picoboot_enter_xip(libusb_device_handle *dev, int iface, uint8_t ep_in, uint8_t ep_out);

int picoboot_reboot_flash(libusb_device_handle *dev, int iface, uint8_t ep_in, uint8_t ep_out);

enum {
  PC_EXCLUSIVE_ACCESS  = 0x01,
  PC_FLASH_ERASE = 0x03,

  PC_REBOOT2 = 0x0a,

  PC_WRITE = 0x05,
  PC_EXIT_XIP = 0x06,
  PC_ENTER_CMD_XIP = 0x07,

  PC_READ = 0x84,
  PC_GET_INFO = 0x8b
};

enum {
  PICOBOOT_OK                       = 0,
  PICOBOOT_UNKNOWN_CMD              = 1,
  PICOBOOT_INVALID_CMD_LENGTH       = 2,
  PICOBOOT_INVALID_TRANSFER_LENGTH  = 3,
  PICOBOOT_INVALID_ADDRESS          = 4,
  PICOBOOT_BAD_ALIGNMENT            = 5,
  PICOBOOT_INTERLEAVED_WRITE        = 6,
  PICOBOOT_REBOOTING                = 7,
  PICOBOOT_UNKNOWN_ERROR            = 8,
  PICOBOOT_INVALID_STATE            = 9,
  PICOBOOT_NOT_PERMITTED            = 10,
  PICOBOOT_INVALID_ARG              = 11,
};


#endif

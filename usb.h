#ifndef PICOBOOT_USB
#define PICOBOOT_USB
#include <libusb-1.0/libusb.h>
#include <stdint.h>

void usb_create_handle(uint16_t vender_id, uint16_t device_id, libusb_device_handle **dev, libusb_context **ctx);
uint8_t usb_enumirate_inf(libusb_device_handle *dev, uint8_t interface_class_filter, uint8_t* eps, size_t ep_size);

int usb_claim_inf(libusb_device_handle *dev, int iface);

int usb_force_boot(uint16_t vender_id, uint16_t device_id, const char * tty ,libusb_device_handle **dev, libusb_context **ctx);

#endif

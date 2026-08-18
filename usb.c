#include <libusb-1.0/libusb.h>
#include <stddef.h>
#include <stdint.h>
#include "usb.h"

void usb_create_handle(uint16_t vender_id, uint16_t device_id, libusb_device_handle **dev, libusb_context **ctx) {
  libusb_init(ctx);

  *dev = libusb_open_device_with_vid_pid(*ctx, vender_id, device_id);
}

uint8_t usb_enumirate_inf(libusb_device_handle *dev, uint8_t interface_class_filter, uint8_t* eps, size_t ep_size) {
  struct libusb_device *usbdevice = libusb_get_device(dev);

  struct libusb_config_descriptor *cfg;

  libusb_get_active_config_descriptor(usbdevice, &cfg);
  int iface;

  for(int i = 0; i < cfg->bNumInterfaces; i++){
    const struct libusb_interface_descriptor *alt = &cfg->interface[i].altsetting[0];

    if(alt->bInterfaceClass != interface_class_filter) continue;
    if(alt->bNumEndpoints != ep_size) continue;

    for (size_t i = 0; i < ep_size; i++) {
      eps[i] = alt->endpoint[i].bEndpointAddress;
    }

    iface = alt->bInterfaceNumber;
  }
  libusb_free_config_descriptor(cfg);

  return iface;
}

int usb_claim_inf(libusb_device_handle *dev, int iface) {
  return libusb_claim_interface(dev, iface);
}

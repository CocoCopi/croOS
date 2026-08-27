/* croOS usb.h - USB 1.1/2.0 host controller basic support */
#ifndef _USB_H
#define _USB_H

#include "kernel/types.h"

#define USB_MAX_DEVICES 16
#define USB_MAX_ENDPOINTS 16

/* USB descriptors */
typedef struct __packed {
    uint8_t  length;
    uint8_t  descriptor_type;
    uint16_t usb_version;
    uint8_t  device_class;
    uint8_t  device_subclass;
    uint8_t  device_protocol;
    uint8_t  max_packet_size;
    uint16_t vendor_id;
    uint16_t product_id;
    uint16_t device_release;
    uint8_t  manufacturer_idx;
    uint8_t  product_idx;
    uint8_t  serial_idx;
    uint8_t  num_configs;
} usb_device_desc_t;

typedef struct __packed {
    uint8_t  length;
    uint8_t  descriptor_type;
    uint8_t  interface_number;
    uint8_t  alternate_setting;
    uint8_t  num_endpoints;
    uint8_t  interface_class;
    uint8_t  interface_subclass;
    uint8_t  interface_protocol;
    uint8_t  interface_idx;
} usb_interface_desc_t;

typedef struct __packed {
    uint8_t  length;
    uint8_t  descriptor_type;
    uint8_t  endpoint_address;
    uint8_t  attributes;
    uint16_t max_packet_size;
    uint8_t  interval;
} usb_endpoint_desc_t;

typedef struct {
    uint8_t  address;
    uint8_t  port;
    uint16_t vendor_id;
    uint16_t product_id;
    uint8_t  device_class;
    uint8_t  device_subclass;
    uint8_t  max_packet_size;
    uint8_t  num_configs;
    uint8_t  num_interfaces;
    char     product[64];
    char     manufacturer[64];
    uint8_t  in_use;
} usb_device_t;

typedef struct {
    uint32_t base;       /* MMIO base address */
    uint8_t  class_code;
    uint8_t  subclass;
    uint16_t port_count;
    uint8_t  in_use;
} usb_hci_t;

void usb_init(void);
int  usb_enumerate_device(int port);
int  usb_get_device_count(void);
usb_device_t *usb_get_device(int index);
void usb_dump_devices(void);
void usb_poll(void);

#endif

#ifndef USB_H
#define USB_H

#include <stdint.h>
#include <stdbool.h>

// USB Speeds
#define USB_SPEED_LOW       0
#define USB_SPEED_FULL      1
#define USB_SPEED_HIGH      2
#define USB_SPEED_SUPER     3
#define USB_SPEED_SUPER_PLUS 4

// USB Request Types
#define USB_REQ_GET_STATUS          0x00
#define USB_REQ_CLEAR_FEATURE       0x01
#define USB_REQ_SET_FEATURE         0x03
#define USB_REQ_SET_ADDRESS         0x05
#define USB_REQ_GET_DESCRIPTOR      0x06
#define USB_REQ_SET_DESCRIPTOR      0x07
#define USB_REQ_GET_CONFIGURATION   0x08
#define USB_REQ_SET_CONFIGURATION   0x09

// USB Descriptor Types
#define USB_DESC_DEVICE             0x01
#define USB_DESC_CONFIGURATION      0x02
#define USB_DESC_STRING             0x03
#define USB_DESC_INTERFACE          0x04
#define USB_DESC_ENDPOINT           0x05

// USB Device Classes
#define USB_CLASS_AUDIO             0x01
#define USB_CLASS_HID               0x03
#define USB_CLASS_STORAGE           0x08
#define USB_CLASS_HUB               0x09
#define USB_CLASS_VIDEO             0x0E

// USB Device Descriptor
typedef struct {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t bcdUSB;
    uint8_t  bDeviceClass;
    uint8_t  bDeviceSubClass;
    uint8_t  bDeviceProtocol;
    uint8_t  bMaxPacketSize;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t  iManufacturer;
    uint8_t  iProduct;
    uint8_t  iSerialNumber;
    uint8_t  bNumConfigurations;
} __attribute__((packed)) usb_device_descriptor_t;

// USB Configuration Descriptor
typedef struct {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t wTotalLength;
    uint8_t  bNumInterfaces;
    uint8_t  bConfigurationValue;
    uint8_t  iConfiguration;
    uint8_t  bmAttributes;
    uint8_t  bMaxPower;
} __attribute__((packed)) usb_config_descriptor_t;

// USB Endpoint Descriptor
typedef struct {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bEndpointAddress;
    uint8_t  bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t  bInterval;
} __attribute__((packed)) usb_endpoint_descriptor_t;

// USB Device Structure
typedef struct usb_device {
    int id;
    int port;
    int speed;
    int address;
    
    usb_device_descriptor_t descriptor;
    
    struct usb_controller* controller;
    void* controller_data;
    
    // AI-Enhanced Device Profiling
    uint64_t transfer_count;
    uint64_t bytes_transferred;
    uint32_t avg_latency_us;
    float predicted_bandwidth_mbps;
    
    struct usb_device* parent;
    struct usb_device* children[16];
    int num_children;
} usb_device_t;

// USB Controller Operations
typedef struct usb_controller {
    char name[32];
    int type; // UHCI, OHCI, EHCI, xHCI
    
    // Operations
    int (*reset_port)(struct usb_controller* ctrl, int port);
    int (*enable_port)(struct usb_controller* ctrl, int port);
    int (*control_transfer)(struct usb_controller* ctrl, usb_device_t* dev,
                           void* setup, void* data, int len);
    int (*bulk_transfer)(struct usb_controller* ctrl, usb_device_t* dev,
                        int endpoint, void* data, int len);
    int (*interrupt_transfer)(struct usb_controller* ctrl, usb_device_t* dev,
                             int endpoint, void* data, int len);
    
    void* private_data;
} usb_controller_t;

// Function Prototypes
void usb_init(void);
int usb_register_controller(usb_controller_t* controller);
usb_device_t* usb_enumerate_device(usb_controller_t* controller, int port);
int usb_get_descriptor(usb_device_t* dev, uint8_t type, uint8_t index, 
                       void* buffer, int length);
int usb_set_configuration(usb_device_t* dev, int config);

// AI-Enhanced USB Management
void usb_ai_optimize_transfers(usb_device_t* dev);
float usb_ai_predict_bandwidth(usb_device_t* dev);
void usb_ai_schedule_transfers(void);

#endif // USB_H
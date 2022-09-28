#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <libusb-1.0/libusb.h>

libusb_device_handle *devh;
static int ep_in_addr = 0x83;
static int ep_out_addr = 0x02;

int write_chars(const char *c) {
    int actual_length;
    int size = (int)(strlen(c));
    if (libusb_bulk_transfer(devh, ep_out_addr, (unsigned char *)c, size, &actual_length, 0) <0) {
        fprintf(stderr, "Error while sending char\n");
    }
    return actual_length;
}

int read_chars(unsigned char *data, int size) {
    int actual_length;
    int rc = libusb_bulk_transfer(devh, ep_in_addr, data, size, &actual_length, 1000);
    if (rc == LIBUSB_ERROR_TIMEOUT) {
        //printf("timeout (%d)\n", actual_length);
        return -1;
    } else if (rc < 0) {
        fprintf(stderr, "Error while waiting for char\n");
        return -1;
    }

    return actual_length;
}

void dump(unsigned char *data, int size) {
    int i, s;
    s = 0;
    for (i = 0; i < size; i++) {
        if (i % 16 == 0) {
            printf("  ");
        }
        printf("%02X ", data[i]);
        if (i % 16 == 15) {
            printf(" ");
            for (int j = s; j <= i; j++) {
                if (0x20 <= data[j] && data[j] <= 0x7E) {
                    printf("%c", data[j]);
                } else {
                    printf(".");
                }
            }
            printf("\n");
            s = i + 1;
        }
    }
    int k;
    k = 16 - ((i - 1) % 16) - 1;
    for (int j = 0; j < k; j++) {
        printf("   ");
    }
    printf(" ");


    for (int j = s; j < i; j++) {
        if (0x20 <= data[j] && data[j] <= 0x7E) {
            printf("%c", data[j]);
        } else {
            printf(".");
        }
    }
    printf("\n");
    s = i;
}


#define MSC_ENDPOINT_OUT (0x05)
#define MSC_ENDPOINT_IN  (0x84)
int msc_out(char *msg, unsigned char *data, int size, int verbose) {
    int actual_length;
    int rc = libusb_bulk_transfer(devh, MSC_ENDPOINT_OUT, data, size, &actual_length, 1000);
    if (rc < 0) {
        fprintf(stderr, "Error during bulk transfer: %s\n", libusb_error_name(rc));
    }
    if (verbose > 0) {
        printf("%s : %d\n", msg, actual_length);
    }
    if (verbose >= 10) {
        dump(data, actual_length);
    }
    return rc;
}

int msc_in(char *msg, unsigned char *data, int *size, int verbose) {
    int sz = *size;
    int rc = libusb_bulk_transfer(devh, MSC_ENDPOINT_IN, data, sz, size, 1000);
    if (rc < 0) {
        fprintf(stderr, "Error during bulk transfer: %s\n", libusb_error_name(rc));
    }
    if (verbose > 0) {
        printf("%s : %d\n", msg, *size);
    }
    if (verbose >= 10) {
        dump(data, *size);
    }
    return rc;
}

int msc_0x12_inquiry(unsigned char lun, int verbose) {
    int rc;
    // msc : Inquiry LUN: 0x00
    unsigned char data[64] = {
        0x55, 0x53, 0x42, 0x43, // Signature
        0xA0, 0x59, 0x9b, 0x18, // Tag
        0xFF, 0x00, 0x00, 0x00, // DataTransferLength
        0x80, 0x00 | lun, 0x06, // Flag, LUN, CBWCBLength
        0x12, 0x01, 0x80, 0x00, 0xFF, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    int size;

    rc = msc_out("SCSI: Inquiry LUN: 0x00", data, 31, verbose);
    if (rc < 0) {
        return rc;
    }

    size = 64;
    rc = msc_in("SCSI: Data In LUN", data, &size, verbose);
    if (rc < 0) {
        return rc;
    }

    size = 64;
    msc_in("SCSI: Data In LUN", data, &size, verbose);
    if (rc < 0) {
        return rc;
    }

    return rc;
}

int msc_0x25_readcapacity(unsigned char lun, int verbose) {
    int rc;
    // msc : Inquiry LUN: 0x00
    unsigned char data[64] = {
        0x55, 0x53, 0x42, 0x43, // Signature
        0xA0, 0x59, 0x9b, 0x18, // Tag
        0x08, 0x00, 0x00, 0x00, // DataTransferLength
        0x80, 0x00 | lun, 0x0A, // Flag, LUN, CBWCBLength
        0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    int size;

    rc = msc_out("SCSI: Read Capacity LUN: 0x00", data, 31, verbose);
    if (rc < 0) {
        return rc;
    }

    size = 64;
    rc = msc_in("SCSI: Data In LUN", data, &size, verbose);
    if (rc < 0) {
        return rc;
    }

    if (verbose >= 10) {
        unsigned long llba = (data[0] << 24) + (data[1] << 16) + (data[2] << 8) + (data[3]);
        unsigned long blib = (data[4] << 24) + (data[5] << 16) + (data[6] << 8) + (data[7]);
        printf("  Toal: %d, Block Length in Bytes: %d\n", llba * blib, blib);
    }

    size = 64;
    msc_in("SCSI: Data In LUN", data, &size, verbose);
    if (rc < 0) {
        return rc;
    }

    return rc;
}




int main(int argc, char **argv) {
    libusb_context *context;
    libusb_device_handle *handle;
    libusb_device *device;
    struct libusb_device_descriptor desc;
    unsigned char buffer[256];
    int fd;
    assert((argc > 1) && (sscanf(argv[1], "%d", &fd) == 1));
    libusb_set_option(NULL, LIBUSB_OPTION_WEAK_AUTHORITY);
    assert(!libusb_init(&context));
    assert(!libusb_wrap_sys_device(context, (intptr_t) fd, &handle));
    device = libusb_get_device(handle);
    assert(!libusb_get_device_descriptor(device, &desc));
    printf("Vendor ID: %04x\n", desc.idVendor);
    printf("Product ID: %04x\n", desc.idProduct);
    assert(libusb_get_string_descriptor_ascii(handle, desc.iManufacturer, buffer, 256) >= 0);
    printf("Manufacturer: %s\n", buffer);
    assert(libusb_get_string_descriptor_ascii(handle, desc.iProduct, buffer, 256) >= 0);
    printf("Product: %s\n", buffer);
    if (libusb_get_string_descriptor_ascii(handle, desc.iSerialNumber, buffer, 256) >= 0)
        printf("Serial No: %s\n", buffer);

    devh = handle;
    int rc;
    for (int if_num = 0; if_num < 3; if_num++) {
        if (libusb_kernel_driver_active(devh, if_num)) {
            libusb_detach_kernel_driver(devh, if_num);
        }
        rc = libusb_claim_interface(devh, if_num);
        if (rc < 0) {
            fprintf(stderr, "Error claiming interface: %s\n", libusb_error_name(rc));
            goto out;
        }
    }

    msc_0x12_inquiry(0, 0);
    msc_0x25_readcapacity(0, 10);

    if (0) {
#define ACM_CTRL_DTR 0x001
#define ACM_CTRL_RTS 0x002
        rc = libusb_control_transfer(devh, 0x21, 0x22, ACM_CTRL_DTR | ACM_CTRL_RTS, 0, NULL, 0, 0);
        if (rc < 0) {
            fprintf(stderr, "Error during control transfer: %s\n", libusb_error_name(rc));
        }

        if (1) {
            // for USBCDC : 9600bps : 0x8025
            unsigned char encoding[] = { 0x80, 0x25, 0x00, 0x00, 0x00, 0x00, 0x08 };
            rc = libusb_control_transfer(devh, 0x21, 0x20, 0, 0, encoding, sizeof(encoding), 0);
        } else {
            // for enter boot loader : 1200bps : 0x04B0
            unsigned char encoding[] = { 0xB0, 0x04, 0x00, 0x00, 0x00, 0x00, 0x08 };
            rc = libusb_control_transfer(devh, 0x21, 0x22, 0, 0, encoding, sizeof(encoding), 0);
        }
        if (rc < 0) {
            fprintf(stderr, "Error during control transfer2: %s\n", libusb_error_name(rc));
        }

        // tytouf/libusb-cdc-example
        unsigned char buf[1024];
        int len;

        while (1) {
            write_chars("tinygo\r\n");
            len = read_chars(buf, 1024);
            buf[len] = 0;
            fprintf(stdout, "%s", buf);
            sleep(1);
        }
    }

out:
    libusb_exit(context);
}


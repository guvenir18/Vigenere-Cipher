#ifndef VIG_H
#define VIG_H
#include <linux/ioctl.h>

#define VIG_MAJOR 0
#define VIG_NUM_DEVS 1
#define VIG_BUFFER_SIZE 4 * 1024
#define VIG_KEY "A"
#define VIG_KEY_MAX_CHAR_COUNT 10

#define VIG_IOC_MAGIC 0xB8
#define VIG_RESET _IO(VIG_IOC_MAGIC, 0)
#define VIGENERE_MODE_DECRYPT _IOW(VIG_IOC_MAGIC, 1, char *)
#define VIGENERE_MODE_SIMPLE _IOW(VIG_IOC_MAGIC, 2, char *)
#define VIG_IOC_MAXNR 3

#endif
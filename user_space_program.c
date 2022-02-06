#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "vigenere.h"

void print_ioctl_error(int errstatus)
{
    switch (errstatus)
    {
    case ENOTTY:
        printf("No such command for vigenere device\n");
        break;

    case EFAULT:
        printf("Bad address\n");
        break;

    case EINVAL:
        printf("Keys did not match\n");
        break;

    default:
        printf("Unspecified error\n");
    }
}

int main(int argc, char **argv)
{
    int fd = open("/dev/vigenere", O_RDWR);
    if (fd == -1)
    {
        perror("Cannot access device");
        exit(EXIT_FAILURE);
    }

    printf("Choose one operation:\n");
    printf("0 for reset\n");
    printf("1 for SIMPLE Mode\n");
    printf("2 for DECRYPT Mode\n");
    printf("3 for seek\n");
    printf("4 for read\n");
    printf("5 for write\n");
    printf("6 for exit\n");

    int op;
    printf("Operation: ");

    while (scanf("%d", &op))
    {
        if (op == 0)
        {
            int status = ioctl(fd, VIG_RESET);
            if (status == -1)
            {
                int errstatus = errno;
                print_ioctl_error(errstatus);
            }
            else
            {
                printf("Successfully reset\n");
            }
        }
        else if (op == 1)
        {
            int a;
            char key[VIG_KEY_MAX_CHAR_COUNT];
            printf("Key: ");
            scanf("%s", key);

            int status = ioctl(fd, VIGENERE_MODE_SIMPLE, key);
            if (status == -1)
            {
                int errstatus = errno;
                print_ioctl_error(errstatus);
            }
            else
            {
                printf("Successfully set to mod SIMPLE\n");
            }
        }

        else if (op == 2)
        {
            char key[VIG_KEY_MAX_CHAR_COUNT];
            printf("Key: ");
            scanf("%s", key);

            int status = ioctl(fd, VIGENERE_MODE_DECRYPT, key);
            if (status == -1)
            {
                int errstatus = errno;
                print_ioctl_error(errstatus);
            }
            else
            {
                printf("Successfully set to mod DECRYPT\n");
            }
        }
        else if (op == 3)
        {
        label:
            printf("Choose one seek method:\n");
            printf("1 for SEEK_SET (f_pos = newpos)\n");
            printf("2 for SEEK_CUR (f_pos = f_pos + newpos)\n");
            printf("3 for SEEK_END (f_pos = SIZE + newpos)\n");

            int seek_method;
            printf("Seek method: ");
            scanf("%d", &seek_method);

            if (seek_method == 1)
            {
                int off;
                printf("Offset: ");
                scanf("%d", &off);
                long newpos = lseek(fd, off, SEEK_SET);

                if (newpos < 0)
                {
                    printf("Invalid argument\n");
                }
                else
                {
                    printf("f_pos is %ld now\n", newpos);
                }
            }

            else if (seek_method == 2)
            {
                int off;
                printf("Offset: ");
                scanf("%d", &off);
                long newpos = lseek(fd, off, SEEK_CUR);

                if (newpos < 0)
                {
                    printf("Invalid argument\n");
                }
                else
                {
                    printf("f_pos is %ld now\n", newpos);
                }
            }
            else if (seek_method == 3)
            {
                int off;
                printf("Offset: ");
                scanf("%d", &off);
                long newpos = lseek(fd, off, SEEK_END);

                if (newpos < 0)
                {
                    printf("Invalid argument\n");
                }
                else
                {
                    printf("f_pos is %ld now\n", newpos);
                }
            }
            else
            {
                goto label;
            }
        }
        else if (op == 4)
        {
            int n_bytes;
            char *data;
            printf("Number of bytes to read: ");
            scanf("%d", &n_bytes);
            data = (char *)malloc(sizeof(char) * n_bytes);

            ssize_t n_read = read(fd, data, n_bytes);

            if (n_read < 0)
            {
                printf("Could not read, bad address\n");
            }
            else if (n_read == 0)
            {
                printf("EOF - Nothing to read\n");
            }
            else
            {
                printf("%d bytes has been read\nData: %s\n", n_read, data);
            }
        }
        else if (op == 5)
        {
            char data[VIG_BUFFER_SIZE];
            printf("Data to write: ");
            scanf("%s", data);

            ssize_t n_written = write(fd, data, strlen(data));

            if (n_written < 0)
            {
                printf("Could not write, bad address\n");
            }
            else if (n_written == 0)
            {
                printf("EOF - Buffer is full, could not write\n");
            }
            else
            {
                printf("%d bytes was written to buffer\n", n_written);
            }
        }
        else if (op == 6)
        {
            printf("Goodbye\n");
            break;
        }
        else
        {
            printf("Invalid operation\n");
        }

        printf("Operation: ");
    }

    close(fd);
}

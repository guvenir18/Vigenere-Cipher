#include "vigenere.h"
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/module.h> // THIS_MODULE
#include <linux/types.h>  // struct dev_t
#include <linux/kdev_t.h> // macros (MAJOR, MINOR, MKDEV)
#include <linux/fs.h>
/*
int register_chrdev_region(...) -> static
int alloc_chrdev_region(...) -> dynamic
void unregister_chrdev_region(...) -> free
struct file_operations
struct file, filp is a pointer to file
*/
#include <linux/cdev.h>
#include <linux/kernel.h>
/*
container_of(...)
*/
#include <linux/slab.h>
/*
kmalloc(...)
kfree(...)
*/
#include <linux/uaccess.h>
/*
copy_to_user(...)
copy_from_user(...)
access_ok(...)
*/
#include <linux/errno.h>
/*
-EINTR
-EFAULT
*/
#include <asm/atomic.h>
/*
atomic_t
*/
#include <linux/string.h>
/*
strcmp()
*/

MODULE_AUTHOR("Furkan Pala, Ilke Anil Guvenir");
MODULE_LICENSE("Dual BSD/GPL");

int vig_major = VIG_MAJOR;
int vig_minor = 0;
int vig_num_devs = VIG_NUM_DEVS;
int vig_buffer_size = VIG_BUFFER_SIZE;
char *vig_key = VIG_KEY;

module_param(vig_major, int, S_IRUGO);
module_param(vig_minor, int, S_IRUGO);
module_param(vig_buffer_size, int, S_IRUGO);
module_param(vig_key, charp, S_IRUGO);

struct vig_dev
{
	char *data;
	unsigned long size;
	int mode;
	atomic_t available;
	struct cdev cdev;
};

struct vig_dev *vig_devices;

loff_t vig_llseek(struct file *filp, loff_t off, int whence)
{
	struct vig_dev *dev = filp->private_data;
	loff_t newpos;
	switch (whence)
	{
	case SEEK_SET:
		newpos = off;
		break;

	case SEEK_CUR:
		newpos = filp->f_pos + off;
		break;

	case SEEK_END:
		newpos = dev->size + off;
		break;

	default:
		return -EINVAL;
	}

	if (newpos < 0)
	{
		return -EINVAL;
	}

	filp->f_pos = newpos;

	return newpos;
}

ssize_t vig_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	int i;
	struct vig_dev *dev = filp->private_data;

	if (dev->data == NULL)
	{
		return 0;
	}

	if (*f_pos >= dev->size)
	{
		return 0;
	}

	if (*f_pos + count > dev->size)
	{
		count = dev->size - *f_pos;
	}

	if (dev->mode == VIGENERE_MODE_SIMPLE)
	{
		if (copy_to_user(buf, dev->data + *f_pos, count))
		{
			return -EFAULT;
		}
	}
	// Decrypt
	else
	{
		char *data = (char *)kmalloc(sizeof(char) * count, GFP_KERNEL);

		for (i = *f_pos; i < *f_pos + count; i++)
		{
			data[i] = ((dev->data[i] - vig_key[i % strlen(vig_key)] + 26) % 26) + 65;
		}

		if (copy_to_user(buf, data, count))
		{
			return -EFAULT;
		}
	}

	*f_pos += count;

	return count;
}

ssize_t vig_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	int i;
	struct vig_dev *dev = filp->private_data;

	if (!dev->data)
	{
		dev->data = (char *)kmalloc(sizeof(char) * vig_buffer_size, GFP_KERNEL);
		if (!dev->data)
		{
			return -ENOMEM;
		}
	}

	if (*f_pos >= vig_buffer_size)
	{
		return 0;
	}

	if (*f_pos + count > vig_buffer_size)
	{
		count = vig_buffer_size - *f_pos;
	}

	if (copy_from_user(dev->data + *f_pos, buf, count))
	{
		return -EFAULT;
	}

	// Encrypt
	for (i = *f_pos; i < *f_pos + count; i++)
	{
		dev->data[i] = ((dev->data[i] + vig_key[i % strlen(vig_key)]) % 26) + 65;
	}

	// Update file pointer
	*f_pos += count;

	if (dev->size < *f_pos)
	{
		dev->size = *f_pos;
	}

	return count;
}

long vig_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int err = 0;
	struct vig_dev *dev;
	char *key;

	if (_IOC_TYPE(cmd) != VIG_IOC_MAGIC)
	{
		return -ENOTTY;
	}
	if (_IOC_NR(cmd) > VIG_IOC_MAXNR)
	{
		return -ENOTTY;
	}

	if (_IOC_DIR(cmd) & _IOC_READ)
	{
		err = !access_ok((void __user *)arg, _IOC_SIZE(cmd));
	}
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
	{
		err = !access_ok((void __user *)arg, _IOC_SIZE(cmd));
	}
	if (err)
	{
		return -EFAULT;
	}

	dev = filp->private_data;
	key = kmalloc(sizeof(char) * VIG_KEY_MAX_CHAR_COUNT, GFP_KERNEL);
	//TODO: key 0 initalize??

	switch (cmd)
	{
	case VIG_RESET:
		dev->mode = VIGENERE_MODE_SIMPLE;
		dev->size = 0;
		memset(dev->data, 0, sizeof(char) * vig_buffer_size);

		break;

	case VIGENERE_MODE_DECRYPT:
		//TODO: free key in case of an error
		if (copy_from_user(key, (void __user *)arg, VIG_KEY_MAX_CHAR_COUNT))
		{
			return -EFAULT;
		}

		if (strcmp(key, vig_key) != 0)
		{
			return -EINVAL;
		}

		dev->mode = VIGENERE_MODE_DECRYPT;
		break;

	case VIGENERE_MODE_SIMPLE:
		//TODO: free key in case of an error
		if (copy_from_user(key, (void __user *)arg, VIG_KEY_MAX_CHAR_COUNT))
		{
			return -EFAULT;
		}

		if (strcmp(key, vig_key) != 0)
		{
			return -EINVAL;
		}

		dev->mode = VIGENERE_MODE_SIMPLE;
		break;

	default:
		return -ENOTTY;
	}

	return 0;
}

int vig_open(struct inode *inode, struct file *filp)
{
	struct vig_dev *dev;

	dev = container_of(inode->i_cdev, struct vig_dev, cdev);
	if (!atomic_dec_and_test(&dev->available))
	{
		atomic_inc(&dev->available);
		return -EBUSY;
	}

	filp->private_data = dev;
	return 0;
}

int vig_release(struct inode *inode, struct file *filp)
{
	struct vig_dev *dev;
	dev = container_of(inode->i_cdev, struct vig_dev, cdev);
	atomic_inc(&dev->available);

	return 0;
}

struct file_operations vig_fops = {
	.owner = THIS_MODULE,
	.llseek = vig_llseek,
	.read = vig_read,
	.write = vig_write,
	.unlocked_ioctl = vig_ioctl,
	.open = vig_open,
	.release = vig_release,
};

static void vig_cleanup(void)
{
	dev_t devno = MKDEV(vig_major, vig_minor);
	if (vig_devices)
	{
		int i;
		for (i = 0; i < vig_num_devs; i++)
		{
			cdev_del(&vig_devices[i].cdev);
			kfree(vig_devices[i].data);
		}
		kfree(vig_devices);
	}

	unregister_chrdev_region(devno, vig_num_devs);
}

static int vig_init(void)
{
	int result, i, err;
	dev_t devno = 0;
	struct vig_dev *dev;

	if (vig_major)
	{
		devno = MKDEV(vig_major, vig_minor);
		result = register_chrdev_region(devno, vig_num_devs, "vigenere");
	}
	else
	{
		result = alloc_chrdev_region(&devno, vig_minor, vig_num_devs, "vigenere");
		vig_major = MAJOR(devno);
	}
	if (result < 0)
	{
		printk(KERN_WARNING "vigenere: can't get major %d\n", vig_major);
		return result;
	}

	vig_devices = kmalloc(vig_num_devs * sizeof(struct vig_dev), GFP_KERNEL);

	if (!vig_devices)
	{
		result = -ENOMEM;
		goto fail;
	}
	memset(vig_devices, 0, vig_num_devs * sizeof(struct vig_dev));

	// Initalize each device
	for (i = 0; i < vig_num_devs; i++)
	{
		dev = &vig_devices[i];
		devno = MKDEV(vig_major, vig_minor + i);
		cdev_init(&dev->cdev, &vig_fops);
		err = cdev_add(&dev->cdev, devno, 1);
		if (err)
		{
			printk(KERN_NOTICE "Error %d adding vigenere device no: %d", err, i);
		}

		dev->data = (char *)kmalloc(sizeof(char) * vig_buffer_size, GFP_KERNEL);
		memset(dev->data, 0, sizeof(char) * vig_buffer_size);
		dev->size = 0;
		dev->mode = VIGENERE_MODE_SIMPLE;
		atomic_set(&dev->available, 1);
	}

	return 0;

fail:
	vig_cleanup();
	return result;
}

module_init(vig_init);
module_exit(vig_cleanup);
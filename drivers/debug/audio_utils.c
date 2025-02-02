// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

//#define DEBUG
#include <linux/module.h>
#include <linux/seq_file.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/vmalloc.h>
#include <linux/highmem.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/file.h>

#define DEVICE_NAME		"audio_utils"
#define TEST_IOC_MAGIC		'T'
#define TEST_IOC_SET_LIB_SIZE	_IOW(TEST_IOC_MAGIC, 0x00, uint32_t)
#define TEST_IOC_WRITE_LIB	_IOW(TEST_IOC_MAGIC, 0x01, uint32_t)
#define TEST_IOC_FREE_LIB	_IOW(TEST_IOC_MAGIC, 0x02, uint32_t)

static LIST_HEAD(code_list);
static int written;
static int write_size;
static int lib_size;

static int audio_utils_open(struct inode *inode, struct file *file)
{
	pr_debug("%s\n", __func__);
	return 0;
}

static ssize_t audio_utils_write(struct file *file, const char __user *buffer,
			      size_t _count, loff_t *ppos)
{
	struct page *page;
	int ret, count, write = 0;
	void *tmp;

	write_size = 0;
	count = _count;
	while (count > 0) {
		page = alloc_page(GFP_HIGHUSER);
		if (!page) {
			pr_err("%s-%d\n", __func__, __LINE__);
			return -ENOMEM;
		}
		pr_debug("%s page: %lx, count: %d\n",
			 __func__, page_to_pfn(page), count);
		tmp = kmap(page);
		WARN_ON(!tmp);
		if (count >= PAGE_SIZE) {
			ret = copy_from_user(tmp, buffer + write, PAGE_SIZE);
			if (ret) {
				pr_err("%s-%d write offset=%d size=%d total size=%zu ret=%d\n",
				       __func__, __LINE__, write, (int)PAGE_SIZE, _count, ret);
				kunmap(page);
				__free_page(page);
				return -EINVAL;
			}
			count -= PAGE_SIZE;
			write += PAGE_SIZE;
		} else if (count) { /* remain */
			ret = copy_from_user(tmp, buffer + write, count);
			if (ret) {
				pr_err("%s-%d write offset=%d size=%d total size=%zu ret=%d\n",
				       __func__, __LINE__, write, count, _count, ret);
				kunmap(page);
				__free_page(page);
				return -EINVAL;
			}
			count -= count;
			write += count;
		}
		list_add_tail(&page->lru, &code_list);
		kunmap(page);
	}
	write_size = _count;
	file->f_inode->i_size = write_size;
	pr_debug("%s write_size: %d\n", __func__, write_size);

	return _count;
}

static long audio_utils_ioctl(struct file *f,
			   unsigned int cmd, unsigned long arg)
{
	struct page *page, *next;
	void __user *argp = (void __user *)arg;

	/*
	 * TODO: here just free maintained pages, need more ioctrl commands
	 */
	switch (cmd) {
	case TEST_IOC_SET_LIB_SIZE:
		pr_info("%s SET_LIB_SIZE %ld\n", __func__, arg);
		lib_size = arg;
		break;

	case TEST_IOC_WRITE_LIB:
		pr_info("%s WRITE_LIB\n", __func__);
		if (written) {
			pr_err("%s have written\n", __func__);
			return -EINVAL;
		}

		if (audio_utils_write(f, argp, lib_size, NULL) != lib_size) {
			pr_err("%s, %d\n", __func__, __LINE__);
			list_for_each_entry_safe(page, next, &code_list, lru) {
				__free_page(page);
			}
			INIT_LIST_HEAD(&code_list);
			f->f_inode->i_size = 0;
			lib_size = 0;
			return -ENOMEM;
		}
		written = 1;
		break;

	case TEST_IOC_FREE_LIB:
		pr_info("%s FREE_LIB\n", __func__);
		written = 0;
		list_for_each_entry_safe(page, next, &code_list, lru) {
			__free_page(page);
		}
		INIT_LIST_HEAD(&code_list);
		f->f_inode->i_size = 0;
		lib_size = 0;
		break;
	default:
		break;
	}

	return 0;
}

static int audio_utils_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct page *page;
	unsigned long addr, size;
	int ret, total_pages;

	addr = vma->vm_start;
	size = vma->vm_end - vma->vm_start;
	total_pages = ALIGN(write_size, PAGE_SIZE) / PAGE_SIZE;
	if (vma->vm_pgoff > total_pages) {
		pr_err("%s wrong page off:%ld, total:%d, fsize:%d\n",
		       __func__, vma->vm_pgoff, total_pages, write_size);
		return -EINVAL;
	}
	page = list_first_entry(&code_list, struct page, lru);
	for (ret = 0; ret < vma->vm_pgoff; ret++) {	/* move to right page */
		page = list_next_entry(page, lru);
	}

	while (addr < vma->vm_end) {
		ret = vm_insert_page(vma, addr, page);
		pr_debug("%s insert page %5lx for %lx, ret:%d\n",
			 __func__, page_to_pfn(page), addr, ret);
		if (ret < 0) {
			pr_err("%s-%d\n", __func__, __LINE__);
			return ret;
		}
		addr += PAGE_SIZE;
		page = list_next_entry(page, lru);
		if (&page->lru == &code_list)
			break;
	}

	return 0;
}

#ifdef CONFIG_COMPAT
static long audio_utils_compact_ioctl(struct file *f,
				   unsigned int cmd, unsigned long arg)
{
	arg = (unsigned long)compat_ptr(arg);
	return audio_utils_ioctl(f, cmd, arg);
}
#endif

ssize_t audio_utils_read(struct file *file, char __user *buf,
		      size_t size, loff_t *ppos)
{
	int off = 0, off1, rd_size = 0, can_read, to = 0;
	struct page *page;
	void *tmp;

	pr_debug("%s off:%lx, size:%ld\n",
		 __func__, (unsigned long)*ppos, (unsigned long)size);
	page = list_first_entry(&code_list, struct page, lru);
	while (off < *ppos) {
		off += PAGE_SIZE;
		page = list_next_entry(page, lru);
	}
	off1 = *ppos & ~(PAGE_MASK);
	if (off1)
		can_read = PAGE_SIZE - off1;
	else
		can_read = PAGE_SIZE;
	rd_size = size;
	if (can_read >= rd_size)	/* 1st read align to page size */
		can_read = rd_size;

	while (rd_size > 0) {
		tmp = kmap(page);
		if (!tmp) {
			pr_err("%s-%d\n", __func__, __LINE__);
			return -EINVAL;
		}
		if (copy_to_user(buf + to, tmp, can_read)) {
			pr_err("%s-%d\n", __func__, __LINE__);
			kunmap(page);
			return -EINVAL;
		}
		pr_debug("%s buf:%p, to:%d, canread:%d, rdsize:%d, page:%lx\n",
			 __func__, buf, to, can_read,
			 rd_size, page_to_pfn(page));
		rd_size -= can_read;
		to += can_read;
		kunmap(page);
		if (rd_size >= PAGE_SIZE)
			can_read = PAGE_SIZE;
		else
			can_read = rd_size;
		page = list_next_entry(page, lru);
		if (&page->lru == &code_list)
			break;
	}
	*ppos += size;
	return size;
}

loff_t audio_utils_seek(struct file *file, loff_t offset, int whence)
{
	pr_debug("%s where:%d, off:%lx\n",
		 __func__, whence, (unsigned long)offset);
	return 0;
}

static int audio_utils_release(struct inode *inode, struct file *file)
{
	pr_debug("%s\n", __func__);
	return 0;
}

static const struct file_operations audio_utils_fops = {
	.open		= audio_utils_open,
	.read		= audio_utils_read,
	.llseek		= audio_utils_seek,
	.unlocked_ioctl = audio_utils_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl   = audio_utils_compact_ioctl,
#endif
	.mmap		= audio_utils_mmap,
	.release	= audio_utils_release,
};

static struct class audio_utils_class = {
	.name			= DEVICE_NAME,
	.owner			= THIS_MODULE,
};

static int major;		/* major number we get from the kernel */
static int __init audio_utils_init(void)
{
	int r;
	struct device *cdev;

	r = class_register(&audio_utils_class);
	if (r) {
		pr_err("%s regist class failed\n", __func__);
		return -EINVAL;
	}
	major = register_chrdev(0, DEVICE_NAME, &audio_utils_fops);
	if (major < 0) {
		pr_err("%s register cdev failed\n", __func__);
		return -EINVAL;
	}
	cdev = device_create(&audio_utils_class, NULL,
			     MKDEV(major, 0), NULL, DEVICE_NAME);
	if (IS_ERR_OR_NULL(cdev)) {
		pr_err("%s alloc cdev failed, cdev:%p\n", __func__, cdev);
		return -EINVAL;
	}

	return 0;
}

static void __exit audio_utils_exit(void)
{
	device_destroy(&audio_utils_class, MKDEV(major, 0));
	unregister_chrdev(major, DEVICE_NAME);
	class_unregister(&audio_utils_class);
}

module_init(audio_utils_init);
module_exit(audio_utils_exit);
MODULE_LICENSE("GPL");

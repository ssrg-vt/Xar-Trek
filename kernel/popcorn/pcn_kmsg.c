/*
 * pcn_kmesg.c - Kernel Module for Popcorn Messaging Layer over Socket
 */

#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>

#include <popcorn/pcn_kmsg.h>
#include <popcorn/debug.h>
#include <popcorn/stat.h>
#include <popcorn/bundle.h>

static pcn_kmsg_cbftn pcn_kmsg_cbftns[PCN_KMSG_TYPE_MAX] = { NULL };

static struct pcn_kmsg_transport *transport = NULL;

void pcn_kmsg_set_transport(struct pcn_kmsg_transport *tr)
{
	if (transport && tr) {
		printk(KERN_ERR "Replace hot transport at your own risk.\n");
	}
	transport = tr;
}
EXPORT_SYMBOL(pcn_kmsg_set_transport);

int pcn_kmsg_register_callback(enum pcn_kmsg_type type, pcn_kmsg_cbftn callback)
{
	BUG_ON(type < 0 || type >= PCN_KMSG_TYPE_MAX);

	pcn_kmsg_cbftns[type] = callback;
	return 0;
}
EXPORT_SYMBOL(pcn_kmsg_register_callback);

int pcn_kmsg_unregister_callback(enum pcn_kmsg_type type)
{
	return pcn_kmsg_register_callback(type, (pcn_kmsg_cbftn)NULL);
}
EXPORT_SYMBOL(pcn_kmsg_unregister_callback);


void pcn_kmsg_process(struct pcn_kmsg_message *msg)
{
	pcn_kmsg_cbftn ftn;

#ifdef CONFIG_POPCORN_CHECK_SANITY
	BUG_ON(msg->header.type < 0 || msg->header.type >= PCN_KMSG_TYPE_MAX);
	BUG_ON(msg->header.size < 0 || msg->header.size > PCN_KMSG_MAX_SIZE);
#endif

	ftn = pcn_kmsg_cbftns[msg->header.type];

	if (ftn != NULL) {
		ftn(msg);
	} else {
		printk(KERN_ERR"No callback registered for %d\n", msg->header.type);
		pcn_kmsg_done(msg);
	}

	account_pcn_message_recv(msg);
}
EXPORT_SYMBOL(pcn_kmsg_process);


static inline int __build_and_check_msg(enum pcn_kmsg_type type, int to, void *_msg, size_t size)
{
	struct pcn_kmsg_message *msg = _msg;

#ifdef CONFIG_POPCORN_CHECK_SANITY
	BUG_ON(type < 0 || type >= PCN_KMSG_TYPE_MAX);
	BUG_ON(size > PCN_KMSG_MAX_SIZE);
	BUG_ON(to < 0 || to >= MAX_POPCORN_NODES);
	BUG_ON(to == my_nid);
#endif

	msg->header.type = type;
	msg->header.prio = PCN_KMSG_PRIO_NORMAL;
	msg->header.size = size;
	msg->header.from_nid = my_nid;
	return 0;
}

int pcn_kmsg_send(enum pcn_kmsg_type type, int to, void *msg, size_t size)
{
	int ret;
	if ((ret = __build_and_check_msg(type, to, msg, size))) return ret;

	account_pcn_message_sent(msg);
	return transport->send(to, msg, size);
}
EXPORT_SYMBOL(pcn_kmsg_send);

int pcn_kmsg_post(enum pcn_kmsg_type type, int to, void *msg, size_t size)
{
	int ret;
	if ((ret = __build_and_check_msg(type, to, msg, size))) return ret;

	account_pcn_message_sent(msg);
	return transport->post(to, msg, size);
}
EXPORT_SYMBOL(pcn_kmsg_post);

void *pcn_kmsg_get(size_t size)
{
	if (transport && transport->get)
		return transport->get(size);
	return kmalloc(size, GFP_KERNEL);
}
EXPORT_SYMBOL(pcn_kmsg_get);

void pcn_kmsg_put(void *msg)
{
	if (transport && transport->put) {
		transport->put(msg);
	} else {
		kfree(msg);
	}
}
EXPORT_SYMBOL(pcn_kmsg_put);


void pcn_kmsg_done(void *msg)
{
	if (transport && transport->done) {
		transport->done(msg);
	} else {
		kfree(msg);
	}
}
EXPORT_SYMBOL(pcn_kmsg_done);


ssize_t pcn_kmsg_stat(char *buffer, size_t count)
{
	if (transport && transport->stat) {
		return transport->stat(buffer, count);
	}
	return 0;
}
EXPORT_SYMBOL(pcn_kmsg_stat);

bool pcn_kmsg_has_features(unsigned int features)
{
	if (!transport) return false;

	return (transport->features & features) == features;
}
EXPORT_SYMBOL(pcn_kmsg_has_features);


int pcn_kmsg_rdma_read(int from_nid, void *addr, size_t size, dma_addr_t rdma_addr, u32 rdma_key)
{
	if (!transport || !transport->rdma_read) return -EPERM;

	account_pcn_rdma_read(size);
	return transport->rdma_read(from_nid, addr, size, rdma_addr, rdma_key);
}
EXPORT_SYMBOL(pcn_kmsg_rdma_read);

int pcn_kmsg_rdma_write(int dest_nid, void *addr, size_t size, dma_addr_t rdma_addr, u32 rdma_key)
{
	if (!transport || !transport->rdma_write) return -EPERM;

	account_pcn_rdma_write(size);
    return transport->rdma_write(dest_nid, addr, size, rdma_addr, rdma_key);
}
EXPORT_SYMBOL(pcn_kmsg_rdma_write);


struct pcn_kmsg_rdma_handle *pcn_kmsg_pin_rdma_buffer(void *buffer, size_t size)
{
	if (transport && transport->pin_rdma_buffer) {
		return transport->pin_rdma_buffer(buffer, size);
	}
	return ERR_PTR(-EINVAL);
}
EXPORT_SYMBOL(pcn_kmsg_pin_rdma_buffer);

void pcn_kmsg_unpin_rdma_buffer(struct pcn_kmsg_rdma_handle *handle)
{
	if (transport && transport->unpin_rdma_buffer) {
		transport->unpin_rdma_buffer(handle);
	}
}
EXPORT_SYMBOL(pcn_kmsg_unpin_rdma_buffer);


int __init pcn_kmsg_init(void)
{
	return 0;
}

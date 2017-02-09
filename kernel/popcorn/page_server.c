/**
 * @file page_server.c
 *
 * Popcorn Linux page server implementation
 * This work was an extension of Marina Sadini MS Thesis, but totally revamped
 * for multiple node setup.
 *
 * @author Sang-Hoon Kim, SSRG Virginia Tech 2017
 */

#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/rmap.h>
#include <linux/mmu_notifier.h>
#include <linux/wait.h>
#include <linux/ptrace.h>

#include <asm/tlbflush.h>
#include <asm/cacheflush.h>
#include <asm/mmu_context.h>
#include <asm/kdebug.h>

#include <popcorn/bundle.h>
#include <popcorn/cpuinfo.h>
#include <popcorn/pcn_kmsg.h>
#include <popcorn/debug.h>

#include "types.h"
#include "stat.h"

bool page_is_replicated(struct page *page)
{
	return !!bitmap_weight(page->page_owners, MAX_POPCORN_NODES);
}

bool page_is_mine(struct page *page)
{
	return !page_is_replicated(page) || test_bit(get_nid(), page->page_owners);
}


struct remote_page {
	struct list_head list;
	unsigned long addr;

	bool mapped;
	int ret;
	atomic_t pendings;
	wait_queue_head_t pendings_wait;

	remote_page_response_t *response;
	DECLARE_BITMAP(page_owners, MAX_POPCORN_NODES);

	unsigned char data[PAGE_SIZE];
};

static struct remote_page *__alloc_remote_page_request(struct task_struct *tsk, unsigned long addr, unsigned long fault_flags, remote_page_request_t **preq)
{
	struct remote_page *rp = kzalloc(sizeof(*rp), GFP_KERNEL);
	remote_page_request_t *req = kzalloc(sizeof(*req), GFP_KERNEL);

	BUG_ON(!rp || !req);

	/* rp */
	INIT_LIST_HEAD(&rp->list);
	rp->addr = addr;

	rp->mapped = false;
	rp->ret = 0;
	atomic_set(&rp->pendings, 0);
	init_waitqueue_head(&rp->pendings_wait);

	/* req */
	req->header.type = PCN_KMSG_TYPE_REMOTE_PAGE_REQUEST;
	req->header.prio = PCN_KMSG_PRIO_NORMAL;

	req->tgroup_home_cpu = tsk->tgroup_home_cpu;
	req->tgroup_home_id = tsk->tgroup_home_id;
	req->addr = addr;
	req->remote_pid = tsk->pid;
	req->fault_flags = fault_flags;

	*preq = req;

	return rp;
}


static struct remote_page *__lookup_pending_remote_page_request(memory_t *m, unsigned long addr)
{
	struct remote_page *rp;

	list_for_each_entry(rp, &m->pages, list) {
		if (rp->addr == addr) return rp;
	}
	return NULL;
}


static inline pte_t *__find_pte_lock(struct mm_struct *mm, unsigned long addr, spinlock_t **ptl)
{
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;

	pgd = pgd_offset(mm, addr);
	pud = pud_alloc(mm, pgd, addr);
	if (!pud) return NULL;
	pmd = pmd_alloc(mm, pud, addr);
	if (!pmd) return NULL;

	pte = pte_alloc_map_lock(mm, pmd, addr, ptl);
	return pte;
}


static struct page *__find_page_at(struct mm_struct *mm, unsigned long addr, pte_t **ptep, spinlock_t **ptlp)
{
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte = NULL;
	spinlock_t *ptl = NULL;
	struct page *page;

	pgd = pgd_offset(mm, addr);
	if (!pgd || pgd_none(*pgd)) {
		return ERR_PTR(VM_FAULT_SIGSEGV);
	}
	pud = pud_offset(pgd, addr);
	if (!pud || pud_none(*pud)) {
		return ERR_PTR(VM_FAULT_SIGSEGV);
	}
	pmd = pmd_offset(pud, addr);
	if (!pmd || pmd_none(*pmd)) {
		return ERR_PTR(VM_FAULT_SIGSEGV);
	}

	pte = pte_offset_map_lock(mm, pmd, addr, &ptl);
	if (pte == NULL || pte_none(*pte)) {
		// PTE not exist
		pte_unmap_unlock(pte, ptl);
		page = ERR_PTR(VM_FAULT_SIGSEGV);
		pte = NULL;
		ptl = NULL;
	} else {
		page = pte_page(*pte);
	}

	*ptep = pte;
	*ptlp = ptl;
	return page;
}


/**
 * Response for remote page request and handling the response
 */

struct remote_page_work {
	struct work_struct work;
	void *private;
};


/**************************************************************************
 * Invalidate pages among distributed nodes
 */

static void process_remote_page_invalidate(struct work_struct *_work)
{
	struct pcn_kmsg_work *w = (struct pcn_kmsg_work *)_work;
	remote_page_invalidate_t *req = w->msg;
	memory_t *memory;
	struct mm_struct *mm;

	printk("invalidate_page: %lu\n", req->addr);

	/* Only home issues invalidate requests. Hence, I am a remote */
	memory = find_memory_entry_in(req->tgroup_home_cpu, req->tgroup_home_id);
	if (!memory) {
		printk("%s: no memory for %d %d\n", __func__,
				req->tgroup_home_cpu, req->tgroup_home_id);
		goto out_free;
	}
	mm = memory->helper->mm;
	printk("%s: mm at %p\n", __func__, mm);

out_free:
	pcn_kmsg_free_msg(req);
	kfree(w);
}


static void __invalidate_page(struct task_struct *tsk, struct page *page, unsigned long addr)
{
	int nid;
	remote_page_invalidate_t *req;

	req = kzalloc(sizeof(*req), GFP_KERNEL);

	req->addr = addr;
	req->tgroup_home_cpu = get_nid();
	req->tgroup_home_id = tsk->tgid;

	for_each_set_bit(nid, page->page_owners, MAX_POPCORN_NODES) {
		pcn_kmsg_send_long(nid, req, sizeof(*req));
	}

	bitmap_zero(page->page_owners, MAX_POPCORN_NODES);

	kfree(req);
}



/**************************************************************************
 * Handle remote page fetch response
e*/

static void process_remote_page_response(struct work_struct *_work)
{
	struct pcn_kmsg_work *w = (struct pcn_kmsg_work *)_work;
	remote_page_response_t *res = w->msg;
	struct task_struct *tsk;
	memory_t *m;
	unsigned long flags;
	struct remote_page *rp;

	tsk = find_task_by_vpid(res->remote_pid);
	if (!tsk) {
		__WARN();
		goto out_free;
	}
	get_task_struct(tsk);
	m = tsk->memory;
	BUG_ON(tsk->tgroup_home_id != res->tgroup_home_id);

	spin_lock_irqsave(&m->pages_lock, flags);
	rp = __lookup_pending_remote_page_request(m, res->addr);
	spin_unlock_irqrestore(&m->pages_lock, flags);
	if (!rp) {
		__WARN();
		goto out_unlock;
	}
	WARN_ON(atomic_read(&rp->pendings) <= 0);

	rp->response = res;
	wake_up(&rp->pendings_wait);

out_unlock:
	put_task_struct(tsk);

out_free:
	kfree(w);
}



/**************************************************************************
 * Handle for remote page fetch
 */
static void __forward_remote_page_request(struct page *page)
{
	BUG_ON("Not implemented yet");
}


static void __reply_remote_page(int nid, struct task_struct *tsk, int remote_pid, remote_page_response_t *res)
{
	if (res->result == VM_FAULT_FORWARDED) return;

	res->header.type = PCN_KMSG_TYPE_REMOTE_PAGE_RESPONSE;
	res->header.prio = PCN_KMSG_PRIO_NORMAL;

	res->tgroup_home_cpu = get_nid();
	res->tgroup_home_id = tsk->tgid;
	res->remote_pid = remote_pid;

	pcn_kmsg_send_long(nid, res, sizeof(*res));
}


static int __get_remote_page(struct task_struct *tsk,
		struct vm_area_struct *vma, unsigned long addr,
		int from, remote_page_response_t *res, bool invalidate)
{
	int ret = 0;
	spinlock_t *ptl;
	pte_t *pte;
	struct page *page;
	void *paddr;
	struct mm_struct *mm = tsk->mm;

	page = __find_page_at(mm, addr, &pte, &ptl);
	if (IS_ERR(page)) {
		return PTR_ERR(page);
	}

	if (!page_is_mine(page)) {
		 __forward_remote_page_request(page);
		 return VM_FAULT_FORWARDED;
	}

	flush_cache_page(vma, addr, pte_pfn(*pte));

	paddr = kmap(page);
	copy_page(res->page, paddr);

	/* TODO: adjust page ownership and memory usage statistics */
	if (invalidate) {
		__invalidate_page(tsk, page, addr);
	}
	set_bit(from, page->page_owners);
	kunmap(page);

	pte_unmap_unlock(pte, ptl);
	return ret;
}


static void process_remote_page_request(struct work_struct *work)
{
	struct pcn_kmsg_work *w = (struct pcn_kmsg_work *)work;
	remote_page_request_t *req = w->msg;
	remote_page_response_t *res = NULL;
	struct task_struct *tsk;
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	unsigned long addr = req->addr;
	int from = req->header.from_cpu;

	might_sleep();

	while (!res) {
		res = kzalloc(sizeof(*res), GFP_KERNEL);
	};
	res->addr = addr;

	tsk = find_task_by_vpid(req->tgroup_home_id);
	if (!tsk) {
		res->result = VM_FAULT_SIGBUS;
		goto out;
	}
	get_task_struct(tsk);
	mm = tsk->mm;

	down_read(&mm->mmap_sem);
	vma = find_vma(mm, addr);

	if (!vma || vma->vm_start > addr) {
		res->result = VM_FAULT_SIGBUS;
		goto out_up;
	}

	if (!(req->fault_flags & FAULT_FLAG_WRITE)) {
		printk("remote_page_reqeust: read fault\n");
		res->result = __get_remote_page(tsk, vma, addr, from, res, false);
	} else if (!(vma->vm_flags & VM_SHARED)) {
		printk("remote_page_request: write fault\n");
		res->result = __get_remote_page(tsk, vma, addr, from, res, true);
	} else {
		BUG_ON("remote_page_request: shared fault\n");
	}
	printk("remote_page_request: vma at %lx -- %lx %lx\n",
			vma->vm_start, vma->vm_end, vma->vm_flags);

out_up:
	up_read(&mm->mmap_sem);
	put_task_struct(tsk);

out:
	__reply_remote_page(from, tsk, req->remote_pid, res);
	pcn_kmsg_free_msg(req);
	kfree(w);
	kfree(res);
}


/**************************************************************************
 * Page fault handler
 */

static void __map_remote_page(struct remote_page *rp)
{
	struct mm_struct *mm = current->mm;
	struct vm_area_struct *vma;
	remote_page_response_t *res = rp->response;
	unsigned long addr = res->addr;
	struct page *page;
	void *paddr;
	pte_t pte, *ptep;
	spinlock_t *ptl = NULL;

	down_read(&mm->mmap_sem);
	if ((rp->ret = res->result)) {
		// Something went wrong. I will not map now
		return;
	}

	vma = find_vma(mm, addr);
	BUG_ON(!vma || vma->vm_start > addr);

	printk("%s: vma %lx -- %lx %lx\n", __func__,
			vma->vm_start, vma->vm_end, vma->vm_flags);

	// TODO: accout memory usage

	anon_vma_prepare(vma);
	page = alloc_page_vma(GFP_HIGHUSER_MOVABLE, vma, addr);
	if (!page) {
		rp->ret = VM_FAULT_OOM;
		return;
	}

	paddr = kmap(page);
	copy_page(paddr, rp->response->page);
	kunmap(page);
	flush_icache_page(vma, page);

	// TODO: Update the page ownership
	memcpy(page->page_owners, res->owners, sizeof(page->page_owners));
	set_bit(get_nid(), page->page_owners);

	__SetPageUptodate(page);

	ptep = __find_pte_lock(mm, addr, &ptl);

	pte = mk_pte(page, vma->vm_page_prot);
	pte = maybe_mkwrite(pte_mkdirty(pte), vma);
	pte = pte_set_flags(pte, _PAGE_PRESENT | _PAGE_USER);
	pte = pte_mkyoung(pte);

	if (vma_is_anonymous(vma)) {
		page_add_new_anon_rmap(page, vma, addr);
	} else {
		page_add_file_rmap(page);
	}
	set_pte_at_notify(mm, addr, ptep, pte);
	update_mmu_cache(vma, addr, ptep);
	flush_tlb_page(vma, addr);
	flush_tlb_fix_spurious_fault(vma, addr);

	pte_unmap_unlock(ptep, ptl);
}


int __handle_remote_fault(unsigned long addr, unsigned long fault_flags)
{
	struct task_struct *tsk = current;
	memory_t *m = tsk->memory;
	struct remote_page *rp;
	unsigned long flags;
	DEFINE_WAIT(wait);
	int remaining;
	int ret = 0;
	remote_page_request_t *req = NULL;

	spin_lock_irqsave(&m->pages_lock, flags);
	rp = __lookup_pending_remote_page_request(m, addr);
	if (!rp) {
		struct remote_page *r;
		spin_unlock_irqrestore(&m->pages_lock, flags);

		rp = __alloc_remote_page_request(tsk, addr, fault_flags, &req);

		spin_lock_irqsave(&m->pages_lock, flags);
		r = __lookup_pending_remote_page_request(m, addr);
		if (!r) {
			printk("%s: %lx from %d, %d\n", __func__,
					addr, tsk->tgroup_home_cpu, tsk->tgroup_home_id);
			list_add(&rp->list, &m->pages);
		} else {
			printk("%s: %lx pended\n", __func__, addr);
			kfree(rp);
			rp = r;
			kfree(req);
			req = NULL;
		}
	}
	atomic_inc(&rp->pendings);
	spin_unlock_irqrestore(&m->pages_lock, flags);

	if (req) {
		pcn_kmsg_send_long(tsk->tgroup_home_cpu, req, sizeof(*req));
		kfree(req);
	}

	prepare_to_wait(&rp->pendings_wait, &wait, TASK_UNINTERRUPTIBLE);
	up_read(&tsk->mm->mmap_sem);
	schedule();

	/* Now the remote page would be brought in to rp */

	finish_wait(&rp->pendings_wait, &wait);

	remaining = atomic_dec_return(&rp->pendings);
	if (!rp->mapped) {
		__map_remote_page(rp);	// This function set rp->ret
		rp->mapped = true;
	} else {
		down_read(&tsk->mm->mmap_sem);
	}
	ret = rp->ret;

	printk("%s: %lx resume %d %d\n", __func__, addr, ret, remaining);

	spin_lock_irqsave(&m->pages_lock, flags);
	if (remaining) {
		wake_up(&rp->pendings_wait);
	} else {
		list_del(&rp->list);
		pcn_kmsg_free_msg(rp->response);
		kfree(rp);
	}
	spin_unlock_irqrestore(&m->pages_lock, flags);

	return ret;
}


/**
 * down_read(&mm->mmap_sem) already held getting in
 *
 * return types:
 * VM_FAULT_CONTINUE, page_fault that can handled
 * 0, remotely fetched;
 */
int page_server_handle_pte_fault(struct mm_struct *mm,
		struct vm_area_struct *vma,
		unsigned long address, pte_t *pte, pmd_t *pmd,
		unsigned int flags)
{
	unsigned long addr = address & PAGE_MASK;
	struct pt_regs *regs = current_pt_regs();
	pte_t entry = *pte;
	barrier();

	might_sleep();

	printk(KERN_WARNING"\n");
	printk(KERN_WARNING"## PAGEFAULT: %lx %lx\n", address, regs->ip);

	if (!pte_present(entry)) {
		BUG_ON(!pte_none(entry) && "Is swap enabled?");

		/* Can we handle the fault locally? */
		if (vma->vm_flags & VM_FETCH_LOCAL) {
			WARN_ON("pte_fault: VM_FETCH_LOCAL. continue\n");
			return VM_FAULT_CONTINUE;
		}
		if (!vma_is_anonymous(vma) &&
				((vma->vm_flags & (VM_WRITE | VM_SHARED)) == 0)) {
			printk("pte_fault: Locally file-mapped. continue\n");
			return VM_FAULT_CONTINUE;
		}
		return __handle_remote_fault(addr, flags);
	}

	// Fault occurs whereas PTE exists: CoW?
	if (flags & FAULT_FLAG_WRITE) {
		if (!pte_write(entry)) {
			WARN_ON("pte_fault: lets cow. dont forget to notify of server\n");
			return VM_FAULT_CONTINUE;
		}
	}
	return VM_FAULT_SIGSEGV;
}


/**************************************************************************
 * Routing popcorn messages to worker
 */
static struct workqueue_struct *remote_page_wq;

DEFINE_KMSG_WQ_HANDLER(remote_page_request, remote_page_wq);
DEFINE_KMSG_WQ_HANDLER(remote_page_response, remote_page_wq);
DEFINE_KMSG_WQ_HANDLER(remote_page_invalidate, remote_page_wq);

int __init page_server_init(void)
{
	remote_page_wq = create_workqueue("remote_page_wq");
	if (!remote_page_wq)
		return -ENOMEM;

	REGISTER_KMSG_WQ_HANDLER(
			PCN_KMSG_TYPE_REMOTE_PAGE_REQUEST, remote_page_request);
	REGISTER_KMSG_WQ_HANDLER(
			PCN_KMSG_TYPE_REMOTE_PAGE_RESPONSE, remote_page_response);
	REGISTER_KMSG_WQ_HANDLER(
			PCN_KMSG_TYPE_REMOTE_PAGE_INVALIDATE, remote_page_invalidate);

	return 0;
}

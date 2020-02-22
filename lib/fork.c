// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800
//#define PTEPTR(x)	((0x3BD<<22)|(PDX(x)<<12)|(PTX(x)<<2))
//extern pte_t* uvpd;
//extern pte_t* uvpt;
//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	//if((err&FEC_WR)==0||(*(pte_t *)(PTEPTR((uint32_t)(addr)))&PTE_COW)==0)
	if(!((err&FEC_WR)&&(uvpd[PDX(addr)]&PTE_P)&&(uvpt[PGNUM(addr)]&PTE_P)&&(uvpt[PGNUM(addr)]&PTE_COW)))
		panic("Page fault but not caused by write to a COW page");
	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
	addr=(void *)PTE_ADDR(addr);
	if((r=sys_page_alloc(0,(void *)PFTEMP,PTE_P|PTE_U|PTE_W))<0)
		panic("sys_page_alloc: %e",r);
	memmove(PFTEMP,addr,PGSIZE);
	if((r=sys_page_map(0,PFTEMP,0,addr,PTE_P|PTE_U|PTE_W))<0)
		panic("sys_page_map: %e",r);
	if((r=sys_page_unmap(0,(void *)PFTEMP))<0)
		panic("sys_page_unmap: %e",r);
	//panic("pgfault not implemented");
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;

	// LAB 4: Your code here.
	void *va=(void *)(pn*PGSIZE);
	//if(*(pte_t *)(PTEPTR(va))&(PTE_W|PTE_COW)){
	if(uvpt[pn]&(PTE_W|PTE_COW)){
		if((r=sys_page_map(0,va,envid,va,PTE_P|PTE_U|PTE_COW))<0)
			return r;
		if((r=sys_page_map(0,va,0,va,PTE_P|PTE_U|PTE_COW))<0)
			return r;
	}
	else{
		if((r=sys_page_map(0,va,envid,va,PTE_P|PTE_U))<0)
			return r;
	}
	//panic("duppage not implemented");
	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	//panic("fork not implemented");
	extern void _pgfault_upcall(void);
	envid_t envid;
	int r;
	set_pgfault_handler(pgfault);
	envid=sys_exofork();
	if(envid<0){
		panic("sys_exofork() failed");
		return envid;
	}
	if(envid==0){
		//子进程
		thisenv=envs+ENVX(sys_getenvid());
		cprintf("env %d start\n",thisenv->env_id);
		//set_pgfault_handler(pgfault);
		return 0;
	}
	//父进程
	unsigned pn;
	for(pn=0;pn<PGNUM(USTACKTOP);pn++){
		//开始想偷懒，直接判断uvpt[pn]&PTE_P，会导致后面page fault出问题，必须要判断页目录项是否存在
		if((uvpd[PDX(pn*PGSIZE)]&PTE_P)&&(uvpt[pn]&PTE_P)&&(uvpt[pn]&PTE_U)){
			if((r=duppage(envid,pn))<0)
				return r;
		}
	}
	if((r=sys_page_alloc(envid,(void *)(UXSTACKTOP-PGSIZE),PTE_P|PTE_U|PTE_W))<0)
		return r;
	if((r=sys_env_set_pgfault_upcall(envid,_pgfault_upcall))<0)
		return r;
	if((r=sys_env_set_status(envid,ENV_RUNNABLE))<0)
		return r;
	return envid;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}

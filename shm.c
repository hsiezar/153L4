#include "param.h"
#include "types.h"
#include "defs.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"

const uint tableSize = 64;

struct {
  struct spinlock lock;
  struct shm_page {
    uint id;
    char *frame;
    int refcnt;
  } shm_pages[64];
} shm_table;

void shminit() {
  int i;
  initlock(&(shm_table.lock), "SHM lock");
  acquire(&(shm_table.lock));
  for (i = 0; i< 64; i++) {
    shm_table.shm_pages[i].id =0;
    shm_table.shm_pages[i].frame =0;
    shm_table.shm_pages[i].refcnt =0;
  }
  release(&(shm_table.lock));
}

int shm_open(int id, char **pointer) {

//you write this
    int i;
    acquire(&(shm_table.lock));
    for (i = 0; i < tableSize; i++) {
        if (shm_table.shm_pages[i].id == id) { //check if segment id is found
            if (mappages(myproc()->pgdir, (char *)PGROUNDUP(myproc()->sz), PGSIZE, V2P(shm_table.shm_pages[i].frame), PTE_W|PTE_U) == -1) { //ADD MAPPING FROM VIRTUAL TO PHYSICAL
	        release(&(shm_table.lock));
	        return -1;
	    }

            shm_table.shm_pages[i].refcnt += 1; //increase refcnt
            *pointer = (char *)PGROUNDUP(myproc()->sz);
            myproc()->sz += PGSIZE;
            release(&(shm_table.lock)); 
            return 0;
        }
    }

    for (i = 0; i < tableSize; i++) {
        if (shm_table.shm_pages[i].id == 0) {
            shm_table.shm_pages[i].id = id;
            if ((shm_table.shm_pages[i].frame = kalloc()) == 0) {
                release(&(shm_table.lock));
                return -1;
            }
            memset(shm_table.shm_pages[i].frame, 0, PGSIZE);
            shm_table.shm_pages[i].refcnt = 1;
	    if (mappages(myproc()->pgdir, (char *)PGROUNDUP(myproc()->sz), PGSIZE, V2P(shm_table.shm_pages[i].frame), PTE_W|PTE_U) == -1) {
	        release(&(shm_table.lock));
	        return -1;
	    }
	    //return pointer to virtual address
	    *pointer = (char *)PGROUNDUP(myproc()->sz);
            myproc()->sz += PGSIZE;
            release(&(shm_table.lock));
            return 0;

        }
    }
    

    release(&(shm_table.lock));
    return -1; //added to remove compiler warning -- you should decide what to return

}


int shm_close(int id) {
//you write this too!
    int i;

    acquire(&(shm_table.lock)); //Lock to not lose updates
    for (i = 0; i < tableSize; i++) {
	//CHECK IF SEGMENT ID IS FOUND
        if (shm_table.shm_pages[i].id == id) {
            shm_table.shm_pages[i].refcnt -= 1; //DECREMENT REF
            if (shm_table.shm_pages[i].refcnt <= 0) {
                shm_table.shm_pages[i].id = 0; //clear all values to 0
                shm_table.shm_pages[i].frame = 0;
                shm_table.shm_pages[i].refcnt = 0;
            }
            release(&(shm_table.lock));
            return 0;
        }
    }
    release(&(shm_table.lock));
    return -1;
}

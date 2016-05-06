#ifndef KASERT_H_
#define KASERT_H_

#ifdef HW_KDEBUG
#define kassert(cond) \
    do \
    { \
        if (!(cond)) \
        { \
            printk("KASSERT FAILED [jiffies: %u pid: %d] !!!!!!!!!!!!!!!!!! \n\t[cond: %s] \n\t[file: `%s` line: %d]\n", jiffies, current->pid, #cond, __FILE__, __LINE__); \
        } \
    } while(0)
#else  
#define kassert(cond) \
    do { (void)sizeof(cond); } while(0) 
#endif // HW_KDEBUG

#endif // KASERT_H_

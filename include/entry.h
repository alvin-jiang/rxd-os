/**
 *
 * @file: entry.h
 * @author: Alvin Jiang
 * @mail: celsius.j@gmail.com
 * @created time: 2015-05-24
 *
 */


#ifndef __ENTRY_H__
#define __ENTRY_H__

extern void move_to_user_mode (void);
extern void copy_from_user (char * kaddr, const char * uaddr);
extern void strncpy_from_user (char * kaddr, const char * uaddr, int count);

#define KERNEL_STACK_TOP(task) ((unsigned long *)((char *)(task) + PAGE_SIZE))

#define SS(task) *(KERNEL_STACK_TOP(task) - 1)
#define ESP(task) *(KERNEL_STACK_TOP(task) - 2)
#define EFLAGS(task) *(KERNEL_STACK_TOP(task) - 3)
#define CS(task) *(KERNEL_STACK_TOP(task) - 4)
#define EIP(task) *(KERNEL_STACK_TOP(task) - 5)

#define EAX(task) *(KERNEL_STACK_TOP(task) - 6)
#define ECX(task) *(KERNEL_STACK_TOP(task) - 7)
#define EDX(task) *(KERNEL_STACK_TOP(task) - 8)
#define EBX(task) *(KERNEL_STACK_TOP(task) - 9)
/* placeholder for esp */
#define EBP(task) *(KERNEL_STACK_TOP(task) - 11)
#define ESI(task) *(KERNEL_STACK_TOP(task) - 12)
#define EDI(task) *(KERNEL_STACK_TOP(task) - 13)

#define DS(task) *(KERNEL_STACK_TOP(task) - 14)
#define ES(task) *(KERNEL_STACK_TOP(task) - 15)

#define FORK_STACK(task) (KERNEL_STACK_TOP(task) - 15)

#define SET_TASK_RETVAL(task, retval) { EAX(task) = (retval); }
#define SET_RETVAL(retval) SET_TASK_RETVAL(current_task, retval)

#define RETURN(retval) do { SET_RETVAL(retval); return; } while (0)

#endif


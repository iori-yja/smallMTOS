Task switcher
=============

Trigger
-------
I've made decision to use IRQ for task switcher.

First reason, FIQ is not focused at such a usage.
FIQ is especcially used in very strict time limitation, like as in controlling
DRAMs, framebuffer, etc.

Secondly, in ARMv7 registers, there is "bank register", it is masked when
the processor is in priviledge mode.
More registers are masked in FIQ mode than IRQ mode, and it is very complicated
and rather slow to back up more registers under mask.

Here, as my decision to use IRQ, what I have to back up is only LR and SP(each
of them are Link Register and Stack Pointer).


Task structure
--------------
Tasks are in list now, it is smarter implementation than table.
Lack of MMU, we have to deal with "real" memory address, and we can't alocate
a resizable table eazily.
Task run queue is managed by
int create_task(int*functionpointer);
int kill_task(int pid);
int run_task(int pid);
int suspend_task(int pid);
:
:
:



#include "common.h"
#include "x86.h"
#include "device.h"

#define va_to_pa(va) (va + (current + 1) * 0x100000)
#define pa_to_va(pa) (pa - (current + 1) * 0x100000)

extern SegDesc gdt[NR_SEGMENTS];
extern TSS tss;
extern int displayRow;
extern int displayCol;

extern ProcessTable pcb[MAX_PCB_NUM];
extern int current; // current process


void GProtectFaultHandle(struct StackFrame *sf);

void timerHandle(struct StackFrame *sf);

void syscallHandle(struct StackFrame *sf);

void syscallWrite(struct StackFrame *sf);
void syscallPrint(struct StackFrame *sf);
void syscallFork(struct StackFrame *sf);
void syscallExec(struct StackFrame *sf);
void syscallSleep(struct StackFrame *sf);
void syscallExit(struct StackFrame *sf);


void irqHandle(struct StackFrame *sf) { // pointer sf = esp
	/* Reassign segment register */
	asm volatile("movw %%ax, %%ds"::"a"(KSEL(SEG_KDATA)));
	
	/* Save esp to stackTop */
	//为了中断嵌套
	pcb[current].stackTop=(uint32_t)sf;

	switch(sf->irq) {
		case -1:
			break;
		case 0xd:
			GProtectFaultHandle(sf);
			break;
		case 0x20:
			timerHandle(sf);
			break;
		case 0x80:
			syscallHandle(sf);
			break;
		default:assert(0);
	}
}

void GProtectFaultHandle(struct StackFrame *sf) {
	assert(0);
	return;
}

void timerHandle(struct StackFrame *sf){
	//TODO 完成进程调度，建议使用时间片轮转，按顺序调度
	uint32_t tmpStackTop;
	//遍历pcb，将状态为STATE_BLOCKED的进程的sleepTime减一，如果进程的sleepTime变为0，重 新设为STATE_RUNNABLE
	for (int i = 1; i < MAX_PCB_NUM; i++)
	{	
		if (pcb[i].state == STATE_BLOCKED)
		{
			pcb[i].sleepTime--;
			if (pcb[i].sleepTime == 0)
				pcb[i].state = STATE_RUNNABLE;
		}
	}
	//将当前进程的timeCount加一
	pcb[current].timeCount++;

	//轮转调度
	int pre = current;
	if (pcb[current].timeCount >= MAX_TIME_COUNT)
	{
		//find process able to run
		int i = (current + 1) % MAX_PCB_NUM;
		while (i != current)
		{
			if (pcb[i].state == STATE_RUNNABLE || pcb[i].state == STATE_RUNNING)
				break;
			i = (i + 1) % MAX_PCB_NUM;
		}
		//find another process
		if (i != current)
			current = i;
		//still current process
		else
		{
			if (pcb[current].state == STATE_RUNNABLE || pcb[current].state == STATE_RUNNING)
				pcb[current].timeCount = 0;
			else
				current = 0;
		}
		if(pre!=current&&pcb[pre].state==STATE_RUNNING)
			pcb[pre].state = STATE_RUNNABLE;
		pcb[current].state = STATE_RUNNING;
	}
	
	tmpStackTop = pcb[current].stackTop;
	pcb[current].stackTop = pcb[current].prevStackTop;
	tss.esp0 = (uint32_t)&(pcb[current].stackTop);
	asm volatile("movl %0, %%esp" ::"m"(tmpStackTop));
	asm volatile("popl %gs");
	asm volatile("popl %fs");
	asm volatile("popl %es");
	asm volatile("popl %ds");
	asm volatile("popal");
	asm volatile("addl $8, %esp");
	asm volatile("iret");
	return;
}


void syscallHandle(struct StackFrame *sf) {
	switch(sf->eax) { // syscall number
		case 0:
			syscallWrite(sf);
			break; // for SYS_WRITE
		/* Add Fork,Sleep... */
		case 1:
			syscallFork(sf);
			break;
		case 2:
			syscallExec(sf);
			break;
		case 3:
			syscallSleep(sf);
			break;
		case 4:
			syscallExit(sf);
			break;
		default:break;
	}
}

void syscallWrite(struct StackFrame *sf) {
	switch(sf->ecx) { // file descriptor
		case 0:
			syscallPrint(sf);
			break; // for STD_OUT
		default:break;
	}
}

void syscallPrint(struct StackFrame *sf) {
	int sel = sf->ds; // segment selector for user data, need further modification
	char *str = (char*)sf->edx;
	int size = sf->ebx;
	int i = 0;
	int pos = 0;
	char character = 0;
	uint16_t data = 0;
	asm volatile("movw %0, %%es"::"m"(sel));
	for (i = 0; i < size; i++) {
		asm volatile("movb %%es:(%1), %0":"=r"(character):"r"(str+i));
		if(character == '\n') {
			displayRow++;
			displayCol=0;
			if(displayRow==25){
				displayRow=24;
				displayCol=0;
				scrollScreen();
			}
		}
		else {
			data = character | (0x0c << 8);
			pos = (80*displayRow+displayCol)*2;
			asm volatile("movw %0, (%1)"::"r"(data),"r"(pos+0xb8000));
			displayCol++;
			if(displayCol==80){
				displayRow++;
				displayCol=0;
				if(displayRow==25){
					displayRow=24;
					displayCol=0;
					scrollScreen();
				}
			}
		}

	}
	updateCursor(displayRow, displayCol);
	sf->eax=size;
	return;
}	

void memcpy(void* dst,void* src,size_t size){
	for(uint32_t j=0;j<size;j++){
		*(uint8_t*)(dst+j)=*(uint8_t*)(src+j);
	}
}

void syscallFork(struct StackFrame *sf){
	//TODO 完善它
putChar('F');
	//TODO 查找空闲pcb，如果没有就返回-1
	int i=0;
	for (i = 0; i < MAX_PCB_NUM; i++)
	{
		if (pcb[i].state == STATE_DEAD)
			break;
	}

	if(i==MAX_PCB_NUM)
		{
			pcb[current].regs.eax = -1;
			return;
		}


	//TODO 拷贝地址空间
	enableInterrupt();
	
	memcpy((void*)((i+1)*0x100000),(void*)((current+1)*0x100000),0x100000);
	
	disableInterrupt();

	// 拷贝pcb，这部分代码给出了，请注意理解
	memcpy(&pcb[i],&pcb[current],sizeof(ProcessTable));
	
	pcb[current].regs.eax = i;
	pcb[i].pid = i;

	pcb[i].regs.eax = 0;
	pcb[i].regs.cs = USEL(1 + i * 2);
	pcb[i].regs.ds = USEL(2 + i * 2);
	pcb[i].regs.es = USEL(2 + i * 2);
	pcb[i].regs.fs = USEL(2 + i * 2);
	pcb[i].regs.gs = USEL(2 + i * 2);
	pcb[i].regs.ss = USEL(2 + i * 2);
	pcb[i].stackTop = (uint32_t)&(pcb[i].regs);
	pcb[i].prevStackTop = (uint32_t)&(pcb[i].stackTop);
	pcb[i].state = STATE_RUNNABLE;
	pcb[i].timeCount = 0;
	pcb[i].sleepTime = 0;
}	


void syscallExec(struct StackFrame *sf) {
	// TODO 完成exec
	// hint: 用loadelf，已经封装好了
	uint32_t entry = 0;
	uint32_t secstart = sf->ecx;
	uint32_t secnum =  sf->edx;

	SegDesc seg = gdt[sf->ss >> 3];
	uint32_t base = (seg.base_31_24 << 24) | (seg.base_23_16 << 16) | seg.base_15_0;
	int argc=sf->ebx;
	//point to user stack
	char **argv=(void*)sf->esi+base;
	//compute the maxlen of paremeter
	int lenmax = 0;
	for(int i=0;i<argc;++i)
	{
		int len = 0;
		for (char *s=argv[i]+base; *s; ++s) ++len;
		++len;
		if (len > lenmax) {
			lenmax = len;
		}
	}
	//copy the paremeter from user stack to kernel stack
	char args[argc][lenmax];
	for(int i=0;i<argc;++i)
	{
		char *d=args[i];
		for (char *s=argv[i]+base; *s; ++s, ++d) *d=*s;
		*d='\0';
	}

	loadelf(secstart,secnum, (current + 1) * 0x100000, &entry);
	putChar('E');

	//copy the paremeter str to the user 's esp
	sf->eip = entry;
	sf->esp -= argc * lenmax;
	char (*argd)[lenmax] = (void *)(base + sf->esp);
	for(int i=0;i<argc;++i)
	{
		char *d=argd[i];
		for (char *s=args[i]; *s; ++s, ++d) *d=*s;
		*d='\0';
	}
	//push the pointer pointing to  paremeter str to the user's esp
	sf->esp -= argc * 4;
	char **p = (void *)(base + sf->esp);
	for(int i=0;i<argc;++i)
	{
		p[i]=(void *)&argd[i]-base;
	}
	//esp+8 is valued to argv
	sf->esp -= 12;
	char ***argvd=(void*)sf->esp+8+base;
	*argvd=(void *)p-base;
	//esp+4 is valued to argc
	int *argcd=(void*)sf->esp+4+base;
	*argcd=argc;
	//why after exec,the esp is at where we expect?
	return;


}


void syscallSleep(struct StackFrame *sf){
	//TODO:实现它
	pcb[current].state = STATE_BLOCKED;
	if ((int)sf->ecx < 0)
		return;
	pcb[current].sleepTime = sf->ecx;
	pcb[current].timeCount = MAX_TIME_COUNT;
	asm volatile("int $0x20");
}	

void syscallExit(struct StackFrame *sf){
	//TODO 先设置成dead，然后用int 0x20进入调度
	disableInterrupt();
	pcb[current].state = STATE_DEAD;
	pcb[current].timeCount = MAX_TIME_COUNT;
	enableInterrupt();
	asm volatile("int $0x20");
}

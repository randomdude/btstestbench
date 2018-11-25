extern void unknowninterrupt(void);

extern void int0(void);
extern void int1(void);
extern void int2(void);
extern void int3(void);
extern void int4(void);
extern void int5(void);
extern void int6(void);
extern void int7(void);
extern void int8(void);
extern void int9(void);
extern void int10(void);
extern void int11(void);
extern void int12(void);
extern void int13(void);
extern void int14(void);
extern void int15(void);
extern void int16(void);
extern void int17(void);
extern void int18(void);
extern void int19(void);
extern void int20(void);

struct interruptRecord {
	int interruptNum;
}; typedef struct interruptRecord interruptRecord;
#define MAX_INTERRUPT_RECORDS 0x10

extern interruptRecord* interrupts;
extern int interruptsSoFar;
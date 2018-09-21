#ifndef DEFINE_H_
#define DEFINE_H_

#define	uchar 	unsigned char
#define uint	unsigned int
#define INT8U   unsigned char
#define INT16U  unsigned int
#define ulong   unsigned long

//#define DEBUG_SUPPORT  1

#define CPU_F            ((double)1000000) 
#define Delay_nms(x)      __delay_cycles((long)(CPU_F *(double)(x)/1000))

#endif /* DEFINE_H_ */

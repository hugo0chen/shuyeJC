#include "msp430f2272.h"
#include "define.h"

void write_Seg(INT16U addr, INT8U* buf, INT8U len)
{
  char *Flash_ptr;                          // Flash pointer
  unsigned int i;

  Flash_ptr = (char *)addr;               // Initialize Flash pointer
  FCTL3 = FWKEY;                            // Clear Lock bit
  FCTL1 = FWKEY + ERASE;                    // Set Erase bit
  *Flash_ptr = 0;                           // Dummy write to erase Flash seg

  FCTL1 = FWKEY + WRT;                      // Set WRT bit for write operation

  for (i = 0; i < len; i++)
  {
    *Flash_ptr++ = buf[i];                   // Write value to flash
  }

  FCTL1 = FWKEY;                            // Clear WRT bit
  FCTL3 = FWKEY + LOCK;                     // Set LOCK bit
}
void read_flash(INT16U addr, INT8U *buf, INT8U len)
{
	//_DINT();  // diable interrupts
	for(INT8U i = 0; i < len; i++){
    	buf[i] = *(INT8U*)addr++;
	}
	//_EINT();  // enable interrupts
	return;
}
 

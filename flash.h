#ifndef FLASH_H_
#define FLASH_H_

void write_Seg(INT16U addr, INT8U* buf, INT8U len);
int read_flash(INT16U addr, INT8U *buf, INT8U len);

#endif /*FLASH_H_*/


#include <inttypes.h>

#define safe_insn(out, insn) (insn)

#define BITS_IN_BYTE 8

uint32_t popcnt_x86(uint32_t ebx)
{
  uint32_t eax;
 
/* mov eax, ebx       */
  eax = ebx;
/* and eax, 55555555h */
  eax = eax & 0x55555555;
/* shr ebx, 1         */
  ebx = ebx >> 1;
/* and ebx, 55555555h */
  ebx = ebx & 0x55555555;
/* add ebx, eax       */
  ebx = ebx + eax;
/* mov eax, ebx       */
  eax = ebx;
/* and eax, 33333333h */
  eax = eax & 0x33333333;
/* shr ebx, 2         */
  ebx = ebx >> 2;
/* and ebx, 33333333h */
  ebx = ebx & 0x33333333;
/* add ebx, eax       */
  ebx = ebx + eax;
/* mov eax, ebx       */
  eax = ebx;
/* and eax, 0F0F0F0Fh */
  eax = eax & 0x0F0F0F0F;
/* shr ebx, 4         */
  ebx = ebx >> 4;
/* and ebx, 0F0F0F0Fh */
  ebx = ebx & 0x0F0F0F0F;
/* add ebx, eax       */
  ebx = ebx + eax;
/* mov eax, ebx       */
  eax = ebx;
/* and eax, 00FF00FFh */
  eax = eax & 0x00FF00FF;
/* shr ebx, 8         */
  ebx = ebx >> 8;
/* and ebx, 00FF00FFh */
  ebx = ebx & 0x00FF00FF;
/* add ebx, eax       */
  ebx = ebx + eax;
/* mov eax, ebx       */
  eax = ebx;
/* and eax, 0000FFFFh */
  eax = eax & 0x0000FFFF;
/* shr ebx, 16        */
  ebx = ebx >> 16;
/* and ebx, 0000FFFFh */
  ebx = ebx & 0x0000FFFF;
/* add ebx, eax       */
  ebx = eax + ebx;
/* mov eax, ebx       */
  eax = ebx;
 
  return eax;
}

uint32_t popcnt_loop(uint32_t x)
{
  uint32_t popcnt = 0;
  uint32_t mask = 1;
  for (int i = 0; i < sizeof(uint32_t)*BITS_IN_BYTE; i++) {
    if (mask & x) popcnt++;
    mask <<= 1;
  }

  return popcnt;
}

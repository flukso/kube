MEMORY
{
  FLASH (rx) : ORIGIN = 0x00000000+0x1000, LENGTH = 16k-0x1000
  RAM (rwx) : ORIGIN = 0x10000000+0x200, LENGTH = 4k-0x200
}

INCLUDE rules.ld

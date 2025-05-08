#include "fs.h"


uint32_t data[4] = { 1234, 2345, 9999, 9999};
uint32_t buf[4];
void test_flash()
{

  //cfi_write_word((uintptr_t)0xfffff00000400000, 'a');
  //cfi_write_word((uintptr_t)0xfffff00000400004, 'b');
  CfiFlashWrite((uint32_t *)data, 0x400000, sizeof(data));
  CfiFlashRead(buf, 0x400000, sizeof(buf));
  assert(buf[0] == data[0]);
  assert(buf[1] == data[1]);
  assert(buf[2] == data[2]);
  assert(buf[3] == data[3]);
  cprintf("testflash write and read is ok\n");
}

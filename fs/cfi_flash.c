// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2002-2004
 * Brad Kemp, Seranoa Networks, Brad.Kemp@seranoa.com
 *
 * Copyright (C) 2003 Arabella Software Ltd.
 * Yuli Barcohen <yuli@arabellasw.com>
 *
 * Copyright (C) 2004
 * Ed Okerson
 *
 * Copyright (C) 2006
 * Tolunay Orkun <listmember@orkun.us>
 */

/* The DEBUG define must be before common to enable debugging */
/* #define DEBUG	*/

#include "fs.h"

#define NOR_FLASH_BASE                  FLASH_MAP_ADDR

#include <inc/riscv/riscv.h>
#include <inc/riscv/stdio.h>
#include <inc/riscv/string.h>

#define CFIFLASH_MAX_NUM            2
#define CFIFLASH_CAPACITY           (32 * 1024 * 1024)
#define CFIFLASH_ERASEBLK_SIZE      (128 * 1024 * 2)    /* fit QEMU of 2 banks  */
#define CFIFLASH_ERASEBLK_WORDS     (CFIFLASH_ERASEBLK_SIZE / sizeof(uint32_t))
#define CFIFLASH_ERASEBLK_WORDMASK  (~(CFIFLASH_ERASEBLK_WORDS - 1))

#define CFIFLASH_ONE_BANK_BITS          24

#define CFIFLASH_SEC_SIZE               512
#define CFIFLASH_SEC_SIZE_BITS          9
#define CFIFLASH_SECTORS                (CFIFLASH_CAPACITY / CFIFLASH_SEC_SIZE)

#define CFIFLASH_PAGE_SIZE              (2048 * 2)          /* fit QEMU of 2 banks */
#define CFIFLASH_PAGE_WORDS             (CFIFLASH_PAGE_SIZE / sizeof(uint32_t))
#define CFIFLASH_PAGE_WORDS_MASK        (CFIFLASH_PAGE_WORDS - 1)

#define CFIFLASH_QUERY_CMD              0x98
#define CFIFLASH_QUERY_BASE             0x55
#define CFIFLASH_QUERY_QRY              0x10
#define CFIFLASH_QUERY_VENDOR           0x13
#define CFIFLASH_QUERY_SIZE             0x27
#define CFIFLASH_QUERY_PAGE_BITS        0x2A
#define CFIFLASH_QUERY_ERASE_REGION     0x2C
#define CFIFLASH_QUERY_BLOCKS           0x2D
#define CFIFLASH_QUERY_BLOCK_SIZE       0x2F
#define CFIFLASH_EXPECT_VENDOR          1       /* Intel command set */
#define CFIFLASH_EXPECT_PAGE_BITS       11
#define CFIFLASH_EXPECT_BLOCKS          127     /* plus 1: # of blocks, arm_virt is 255, riscv32 is 127 */
#define CFIFLASH_EXPECT_BLOCK_SIZE      512     /* times 128: block size */
#define CFIFLASH_EXPECT_ERASE_REGION    1

#define CFIFLASH_CMD_ERASE              0x20
#define CFIFLASH_CMD_CLEAR_STATUS       0x50
#define CFIFLASH_CMD_READ_STATUS        0x70
#define CFIFLASH_CMD_CONFIRM            0xD0
#define CFIFLASH_CMD_BUFWRITE           0xE8
#define CFIFLASH_CMD_RESET              0xFF

#define CFIFLASH_STATUS_READY_MASK      0x80

#define BIT_SHIFT8      8
#define BYTE_WORD_SHIFT 2

#define FLASH_ERROR -1
#define FLASH_OK  0

uint32_t CfiFlashSec2Bytes(uint32_t sector)
{
    return sector << CFIFLASH_SEC_SIZE_BITS;
}

static inline uint32_t CfiFlashPageWordOffset(uint32_t wordOffset)
{
    return wordOffset & CFIFLASH_PAGE_WORDS_MASK;
}

static inline uint32_t CfiFlashEraseBlkWordAddr(uint32_t wordOffset)
{
    return wordOffset & CFIFLASH_ERASEBLK_WORDMASK;
}

static inline uint32_t W2B(uint32_t words)
{
  // 0x11 << 2
    return words << BYTE_WORD_SHIFT;
}

static inline uint32_t B2W(uint32_t bytes)
{
    return bytes >> BYTE_WORD_SHIFT;
}

static inline int CfiFlashQueryQRY(uint8_t *p)
{
    uint32_t wordOffset = CFIFLASH_QUERY_QRY;

    if (p[W2B(wordOffset++)] == 'Q') {
        if (p[W2B(wordOffset++)] == 'R') {
            if (p[W2B(wordOffset)] == 'Y') {
                return FLASH_OK;
            }
        }
    }
    return FLASH_ERROR;
}

int CfiFlashQuery()
{
    uint8_t *p = (uint8_t *)NOR_FLASH_BASE;
    if (p == NULL) {
        return FLASH_ERROR;
    }
    uint32_t *base = (uint32_t *)p;
    base[CFIFLASH_QUERY_BASE] = CFIFLASH_QUERY_CMD;

    if (CfiFlashQueryQRY(p)) {
        return FLASH_ERROR;
    }
    
    base[0] = CFIFLASH_CMD_RESET;

    return FLASH_OK;
}

static inline int CfiFlashIsReady(uint32_t wordOffset, volatile uint32_t *p)
{
    //dsb();
    p[wordOffset] = CFIFLASH_CMD_READ_STATUS;
    //dsb();
    return p[wordOffset] & CFIFLASH_STATUS_READY_MASK;
}

/* all in word(4 bytes) measure */
static void CfiFlashWriteBuf(uint32_t wordOffset, const uint32_t *buffer, size_t words, volatile uint32_t *p)
{
    uint32_t blkAddr = 0;

    /* first write might not be Page aligned */
    uint32_t i = CFIFLASH_PAGE_WORDS - CfiFlashPageWordOffset(wordOffset);
    uint32_t wordCount = (i > words) ? words : i;

    while (words) {
        /* command buffer-write begin to Erase Block address */
        blkAddr = CfiFlashEraseBlkWordAddr(wordOffset);
        p[blkAddr] = CFIFLASH_CMD_BUFWRITE;

        /* write words count, 0-based */
        //dsb();
        p[blkAddr] = wordCount - 1;

        /* program word data to actual address */
        for (i = 0; i < wordCount; i++, wordOffset++, buffer++) {
            p[wordOffset] = *buffer;
        }

        /* command buffer-write end to Erase Block address */
        p[blkAddr] = CFIFLASH_CMD_CONFIRM;
        while (!CfiFlashIsReady(blkAddr, p)) { }

        words -= wordCount;
        wordCount = (words >= CFIFLASH_PAGE_WORDS) ? CFIFLASH_PAGE_WORDS : words;
    }

    p[0] = CFIFLASH_CMD_CLEAR_STATUS;
}

int CfiFlashWrite(const uint32_t *buffer, uint32_t offset, uint32_t nbytes)
{
    if ((offset + nbytes) > CFIFLASH_CAPACITY) {
        return FLASH_ERROR;
    }

    volatile uint8_t *pbase = (uint8_t *)NOR_FLASH_BASE;

    if (pbase == NULL) {
        return FLASH_ERROR;
    }

    volatile uint32_t *base = (uint32_t *)pbase;

    uint32_t words = B2W(nbytes);
    uint32_t wordOffset = B2W(offset);

    //disable_irq();
    CfiFlashWriteBuf(wordOffset, buffer, words, base);
    //enable_irq();

    return FLASH_OK;
}

int CfiFlashRead(uint32_t *buffer, uint32_t offset, uint32_t nbytes)
{
    uint32_t i = 0;

    if ((offset + nbytes) > CFIFLASH_CAPACITY) {
        return FLASH_ERROR;
    }

    volatile uint8_t *pbase = (uint8_t *)NOR_FLASH_BASE;
    if (pbase == NULL) {
        return FLASH_ERROR;
    }
    volatile uint32_t *base = (uint32_t *)pbase;

    uint32_t words = B2W(nbytes);
    uint32_t wordOffset = B2W(offset);

    //disable_irq();
    for (i = 0; i < words; i++) {
        buffer[i] = base[wordOffset + i];
    }
    //enable_irq();
    return FLASH_OK;
}

int CfiFlashErase(uint32_t offset)
{
    if (offset > CFIFLASH_CAPACITY) {
        return FLASH_ERROR;
    }

    uint8_t *pbase = (uint8_t *)NOR_FLASH_BASE;
    if (pbase == NULL) {
        return FLASH_ERROR;
    }
    uint32_t *base = (uint32_t *)pbase;

    uint32_t blkAddr = CfiFlashEraseBlkWordAddr(B2W(offset));

    //disable_irq();
    base[blkAddr] = CFIFLASH_CMD_ERASE;
    //dsb();
    base[blkAddr] = CFIFLASH_CMD_CONFIRM;
    while (!CfiFlashIsReady(blkAddr, base)) { }
    base[0] = CFIFLASH_CMD_CLEAR_STATUS;
    //enable_irq();

    return FLASH_OK;
}

#define FS_NOR_OFFSET 0x800000

int cfi_flash_read(uint32_t secno, void *dst, size_t nsecs){
  int i = 0;
  for( i = 0 ; nsecs > 0; i++, nsecs--, dst += SECTSIZE){
    CfiFlashRead(dst, FS_NOR_OFFSET + (secno + i) * SECTSIZE, SECTSIZE);
  }
  return 0;
}
int cfi_flash_write(uint32_t secno, const void *src, size_t nsecs){
  int i = 0;
  for( i = 0 ; nsecs > 0; i++, nsecs--, src += SECTSIZE){
    CfiFlashWrite(src, FS_NOR_OFFSET + (secno + i) * SECTSIZE, SECTSIZE);
  }
  return 0;
}

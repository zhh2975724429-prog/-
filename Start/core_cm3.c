/**************************************************************************//**
 * @file     core_cm3.c
 * @brief    CMSIS Cortex-M3 Core Peripheral Access Layer Source File
 * @version  V1.30
 * @date     30. October 2009
 *
 * @note
 * Copyright (C) 2009 ARM Limited. All rights reserved.
 *
 * @par
 * ARM Limited (ARM) is supplying this software for use with Cortex-M 
 * processor based microcontrollers.  This file can be freely distributed 
 * within development tools that are supporting such ARM based processors. 
 *
 * @par
 * THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 * OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 * ARM SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR
 * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 ******************************************************************************/

#include <stdint.h>

/* define compiler specific symbols */
#if defined ( __CC_ARM   )
  #if (__ARMCC_VERSION >= 6000000)
    /* ARM Compiler 6 */
    #define __ASM            __asm
    #define __INLINE         __inline
    #define __STATIC_INLINE  static __inline
  #else
    /* ARM Compiler 5 */
    #define __ASM            __asm
    #define __INLINE         __inline
    #define __STATIC_INLINE  static __inline
  #endif

#elif defined ( __ICCARM__ )
  #define __ASM           __asm
  #define __INLINE        inline

#elif defined   (  __GNUC__  )
  #define __ASM            __asm
  #define __INLINE         inline

#elif defined   (  __TASKING__  )
  #define __ASM            __asm
  #define __INLINE         inline

#endif


/* ###################  Compiler specific Intrinsics  ########################### */

#if defined ( __CC_ARM   ) /*------------------RealView Compiler -----------------*/
/* ARM armcc specific functions */

#if (__ARMCC_VERSION >= 6000000)
/* ARM Compiler 6 - use embedded assembly */

__attribute__((naked)) uint32_t __get_PSP(void)
{
    __asm volatile (
        "MRS r0, psp\n"
        "BX lr\n"
    );
}

__attribute__((naked)) void __set_PSP(uint32_t topOfProcStack)
{
    __asm volatile (
        "MSR psp, r0\n"
        "BX lr\n"
    );
}

__attribute__((naked)) uint32_t __get_MSP(void)
{
    __asm volatile (
        "MRS r0, msp\n"
        "BX lr\n"
    );
}

__attribute__((naked)) void __set_MSP(uint32_t topOfMainStack)
{
    __asm volatile (
        "MSR msp, r0\n"
        "BX lr\n"
    );
}

__attribute__((naked)) uint32_t __REV16(uint16_t value)
{
    __asm volatile (
        "REV16 r0, r0\n"
        "BX lr\n"
    );
}

__attribute__((naked)) int32_t __REVSH(int16_t value)
{
    __asm volatile (
        "REVSH r0, r0\n"
        "BX lr\n"
    );
}

__attribute__((naked)) uint32_t __RBIT(uint32_t value)
{
    __asm volatile (
        "RBIT r0, r0\n"
        "BX lr\n"
    );
}

__attribute__((naked)) uint8_t __LDREXB(uint8_t *addr)
{
    __asm volatile (
        "LDREXB r0, [r0]\n"
        "BX lr\n"
    );
}

__attribute__((naked)) uint16_t __LDREXH(uint16_t *addr)
{
    __asm volatile (
        "LDREXH r0, [r0]\n"
        "BX lr\n"
    );
}

__attribute__((naked)) uint32_t __LDREXW(uint32_t *addr)
{
    __asm volatile (
        "LDREX r0, [r0]\n"
        "BX lr\n"
    );
}

__attribute__((naked)) uint32_t __STREXB(uint8_t value, uint8_t *addr)
{
    __asm volatile (
        "STREXB r0, r0, [r1]\n"
        "BX lr\n"
    );
}

__attribute__((naked)) uint32_t __STREXH(uint16_t value, uint16_t *addr)
{
    __asm volatile (
        "STREXH r0, r0, [r1]\n"
        "BX lr\n"
    );
}

__attribute__((naked)) uint32_t __STREXW(uint32_t value, uint32_t *addr)
{
    __asm volatile (
        "STREX r0, r0, [r1]\n"
        "BX lr\n"
    );
}

__attribute__((naked)) void __CLREX(void)
{
    __asm volatile (
        "CLREX\n"
        "BX lr\n"
    );
}

__attribute__((naked)) uint32_t __get_BASEPRI(void)
{
    __asm volatile (
        "MRS r0, basepri\n"
        "BX lr\n"
    );
}

__attribute__((naked)) void __set_BASEPRI(uint32_t basePri)
{
    __asm volatile (
        "MSR basepri, r0\n"
        "BX lr\n"
    );
}

__attribute__((naked)) uint32_t __get_PRIMASK(void)
{
    __asm volatile (
        "MRS r0, primask\n"
        "BX lr\n"
    );
}

__attribute__((naked)) void __set_PRIMASK(uint32_t priMask)
{
    __asm volatile (
        "MSR primask, r0\n"
        "BX lr\n"
    );
}

__attribute__((naked)) uint32_t __get_FAULTMASK(void)
{
    __asm volatile (
        "MRS r0, faultmask\n"
        "BX lr\n"
    );
}

__attribute__((naked)) void __set_FAULTMASK(uint32_t faultMask)
{
    __asm volatile (
        "MSR faultmask, r0\n"
        "BX lr\n"
    );
}

__attribute__((naked)) uint32_t __get_CONTROL(void)
{
    __asm volatile (
        "MRS r0, control\n"
        "BX lr\n"
    );
}

__attribute__((naked)) void __set_CONTROL(uint32_t control)
{
    __asm volatile (
        "MSR control, r0\n"
        "BX lr\n"
    );
}

__attribute__((naked)) uint32_t __REV(uint32_t value)
{
    __asm volatile (
        "REV r0, r0\n"
        "BX lr\n"
    );
}

#else
/* ARM Compiler 5 - use old style assembly */

__ASM uint32_t __get_PSP(void)
{
  mrs r0, psp
  bx lr
}

__ASM void __set_PSP(uint32_t topOfProcStack)
{
  msr psp, r0
  bx lr
}

__ASM uint32_t __get_MSP(void)
{
  mrs r0, msp
  bx lr
}

__ASM void __set_MSP(uint32_t mainStackPointer)
{
  msr msp, r0
  bx lr
}

__ASM uint32_t __REV16(uint16_t value)
{
  rev16 r0, r0
  bx lr
}

__ASM int32_t __REVSH(int16_t value)
{
  revsh r0, r0
  bx lr
}

__ASM uint32_t __RBIT(uint32_t value)
{
  rbit r0, r0
  bx lr
}

__ASM uint8_t __LDREXB(uint8_t *addr)
{
  ldrexb r0, [r0]
  bx lr
}

__ASM uint16_t __LDREXH(uint16_t *addr)
{
  ldrexh r0, [r0]
  bx lr
}

__ASM uint32_t __LDREXW(uint32_t *addr)
{
  ldrex r0, [r0]
  bx lr
}

__ASM uint32_t __STREXB(uint8_t value, uint8_t *addr)
{
  strexb r0, r0, [r1]
  bx lr
}

__ASM uint32_t __STREXH(uint16_t value, uint16_t *addr)
{
  strexh r0, r0, [r1]
  bx lr
}

__ASM uint32_t __STREXW(uint32_t value, uint32_t *addr)
{
  strex r0, r0, [r1]
  bx lr
}

__ASM void __CLREX(void)
{
  clrex
  bx lr
}

__ASM uint32_t __get_BASEPRI(void)
{
  mrs r0, basepri
  bx lr
}

__ASM void __set_BASEPRI(uint32_t basePri)
{
  msr basepri, r0
  bx lr
}

__ASM uint32_t __get_PRIMASK(void)
{
  mrs r0, primask
  bx lr
}

__ASM void __set_PRIMASK(uint32_t priMask)
{
  msr primask, r0
  bx lr
}

__ASM uint32_t __get_FAULTMASK(void)
{
  mrs r0, faultmask
  bx lr
}

__ASM void __set_FAULTMASK(uint32_t faultMask)
{
  msr faultmask, r0
  bx lr
}

__ASM uint32_t __get_CONTROL(void)
{
  mrs r0, control
  bx lr
}

__ASM void __set_CONTROL(uint32_t control)
{
  msr control, r0
  bx lr
}

__ASM uint32_t __REV(uint32_t value)
{
  rev r0, r0
  bx lr
}

#endif /* __ARMCC_VERSION */


#elif (defined (__ICCARM__)) /*------------------ ICC Compiler -------------------*/
/* IAR iccarm specific functions */
#pragma diag_suppress=Pe940

uint32_t __get_PSP(void)
{
  __ASM("mrs r0, psp");
  __ASM("bx lr");
}

void __set_PSP(uint32_t topOfProcStack)
{
  __ASM("msr psp, r0");
  __ASM("bx lr");
}

uint32_t __get_MSP(void)
{
  __ASM("mrs r0, msp");
  __ASM("bx lr");
}

void __set_MSP(uint32_t topOfMainStack)
{
  __ASM("msr msp, r0");
  __ASM("bx lr");
}

uint32_t __REV16(uint16_t value)
{
  __ASM("rev16 r0, r0");
  __ASM("bx lr");
}

int32_t __REVSH(int16_t value)
{
  __ASM("revsh r0, r0");
  __ASM("bx lr");
}

uint32_t __RBIT(uint32_t value)
{
  __ASM("rbit r0, r0");
  __ASM("bx lr");
}

uint8_t __LDREXB(uint8_t *addr)
{
  __ASM("ldrexb r0, [r0]");
  __ASM("bx lr");
}

uint16_t __LDREXH(uint16_t *addr)
{
  __ASM("ldrexh r0, [r0]");
  __ASM("bx lr");
}

uint32_t __LDREXW(uint32_t *addr)
{
  __ASM("ldrex r0, [r0]");
  __ASM("bx lr");
}

uint32_t __STREXB(uint8_t value, uint8_t *addr)
{
  __ASM("strexb r0, r0, [r1]");
  __ASM("bx lr");
}

uint32_t __STREXH(uint16_t value, uint16_t *addr)
{
  __ASM("strexh r0, r0, [r1]");
  __ASM("bx lr");
}

uint32_t __STREXW(uint32_t value, uint32_t *addr)
{
  __ASM("strex r0, r0, [r1]");
  __ASM("bx lr");
}

void __CLREX(void)
{
  __ASM("clrex");
}

uint32_t __get_BASEPRI(void)
{
  __ASM("mrs r0, basepri");
  __ASM("bx lr");
}

void __set_BASEPRI(uint32_t basePri)
{
  __ASM("msr basepri, r0");
  __ASM("bx lr");
}

uint32_t __get_PRIMASK(void)
{
  __ASM("mrs r0, primask");
  __ASM("bx lr");
}

void __set_PRIMASK(uint32_t priMask)
{
  __ASM("msr primask, r0");
  __ASM("bx lr");
}

uint32_t __get_FAULTMASK(void)
{
  __ASM("mrs r0, faultmask");
  __ASM("bx lr");
}

void __set_FAULTMASK(uint32_t faultMask)
{
  __ASM("msr faultmask, r0");
  __ASM("bx lr");
}

uint32_t __get_CONTROL(void)
{
  __ASM("mrs r0, control");
  __ASM("bx lr");
}

void __set_CONTROL(uint32_t control)
{
  __ASM("msr control, r0");
  __ASM("bx lr");
}

uint32_t __REV(uint32_t value)
{
  __ASM("rev r0, r0");
  __ASM("bx lr");
}

#pragma diag_default=Pe940


#elif (defined (__GNUC__)) /*------------------ GNU Compiler ---------------------*/
/* GNU gcc specific functions */

__attribute__((naked)) uint32_t __get_PSP(void)
{
    __asm volatile (
        "MRS r0, psp\n"
        "BX lr\n"
    );
}

__attribute__((naked)) void __set_PSP(uint32_t topOfProcStack)
{
    __asm volatile (
        "MSR psp, r0\n"
        "BX lr\n"
    );
}

__attribute__((naked)) uint32_t __get_MSP(void)
{
    __asm volatile (
        "MRS r0, msp\n"
        "BX lr\n"
    );
}

__attribute__((naked)) void __set_MSP(uint32_t topOfMainStack)
{
    __asm volatile (
        "MSR msp, r0\n"
        "BX lr\n"
    );
}

__attribute__((naked)) uint32_t __REV16(uint16_t value)
{
    __asm volatile (
        "REV16 r0, r0\n"
        "BX lr\n"
    );
}

__attribute__((naked)) int32_t __REVSH(int16_t value)
{
    __asm volatile (
        "REVSH r0, r0\n"
        "BX lr\n"
    );
}

__attribute__((naked)) uint32_t __RBIT(uint32_t value)
{
    __asm volatile (
        "RBIT r0, r0\n"
        "BX lr\n"
    );
}

__attribute__((naked)) uint8_t __LDREXB(uint8_t *addr)
{
    __asm volatile (
        "LDREXB r0, [r0]\n"
        "BX lr\n"
    );
}

__attribute__((naked)) uint16_t __LDREXH(uint16_t *addr)
{
    __asm volatile (
        "LDREXH r0, [r0]\n"
        "BX lr\n"
    );
}

__attribute__((naked)) uint32_t __LDREXW(uint32_t *addr)
{
    __asm volatile (
        "LDREX r0, [r0]\n"
        "BX lr\n"
    );
}

__attribute__((naked)) uint32_t __STREXB(uint8_t value, uint8_t *addr)
{
    __asm volatile (
        "STREXB r0, r0, [r1]\n"
        "BX lr\n"
    );
}

__attribute__((naked)) uint32_t __STREXH(uint16_t value, uint16_t *addr)
{
    __asm volatile (
        "STREXH r0, r0, [r1]\n"
        "BX lr\n"
    );
}

__attribute__((naked)) uint32_t __STREXW(uint32_t value, uint32_t *addr)
{
    __asm volatile (
        "STREX r0, r0, [r1]\n"
        "BX lr\n"
    );
}

__attribute__((naked)) void __CLREX(void)
{
    __asm volatile (
        "CLREX\n"
        "BX lr\n"
    );
}

__attribute__((naked)) uint32_t __get_BASEPRI(void)
{
    __asm volatile (
        "MRS r0, basepri\n"
        "BX lr\n"
    );
}

__attribute__((naked)) void __set_BASEPRI(uint32_t basePri)
{
    __asm volatile (
        "MSR basepri, r0\n"
        "BX lr\n"
    );
}

__attribute__((naked)) uint32_t __get_PRIMASK(void)
{
    __asm volatile (
        "MRS r0, primask\n"
        "BX lr\n"
    );
}

__attribute__((naked)) void __set_PRIMASK(uint32_t priMask)
{
    __asm volatile (
        "MSR primask, r0\n"
        "BX lr\n"
    );
}

__attribute__((naked)) uint32_t __get_FAULTMASK(void)
{
    __asm volatile (
        "MRS r0, faultmask\n"
        "BX lr\n"
    );
}

__attribute__((naked)) void __set_FAULTMASK(uint32_t faultMask)
{
    __asm volatile (
        "MSR faultmask, r0\n"
        "BX lr\n"
    );
}

__attribute__((naked)) uint32_t __get_CONTROL(void)
{
    __asm volatile (
        "MRS r0, control\n"
        "BX lr\n"
    );
}

__attribute__((naked)) void __set_CONTROL(uint32_t control)
{
    __asm volatile (
        "MSR control, r0\n"
        "BX lr\n"
    );
}

__attribute__((naked)) uint32_t __REV(uint32_t value)
{
    __asm volatile (
        "REV r0, r0\n"
        "BX lr\n"
    );
}

#endif

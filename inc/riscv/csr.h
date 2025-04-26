#ifndef CSR_H
#define CSR_H

#define CSR_MSTATUS		0x300
#define CSR_MISA		0x301
#define CSR_MIE			0x304
#define CSR_MTVEC		0x305
#define CSR_MSCRATCH		0x340
#define CSR_MEPC		0x341
#define CSR_MCAUSE		0x342
#define CSR_MTVAL		0x343
#define CSR_MIP			0x344

#define CSR_SSTATUS		0x100
#define CSR_SIE			0x104
#define CSR_STVEC		0x105
#define CSR_SCOUNTEREN		0x106
#define CSR_SSCRATCH		0x140
#define CSR_SEPC		0x141
#define CSR_SCAUSE		0x142
#define CSR_STVAL		0x143
#define CSR_SIP			0x144
#define CSR_SATP		0x180

#define SSTATUS_SIE             (0x00000002)
#define SSTATUS_SPIE            (0x00000020)
#define SSTATUS_SPP             (0x00000100)
#define SPP_SHIFT               (0x8)

#define SIE_SSIE                (0x00000002)
#define SIE_STIE                (0x00000020)
#define SIE_SEIE                (0x00000200)

#define IRQ_S_SOFT			1
#define IRQ_VS_SOFT			2
#define IRQ_M_SOFT			3
#define IRQ_S_TIMER			5
#define IRQ_VS_TIMER			6
#define IRQ_M_TIMER			7
#define IRQ_S_EXT			9
#define IRQ_VS_EXT			10
#define IRQ_M_EXT			11
#define IRQ_S_GEXT			12

#define MIP_SSIP			((1) << IRQ_S_SOFT)
#define MIP_VSSIP			((1) << IRQ_VS_SOFT)
#define MIP_MSIP			((1) << IRQ_M_SOFT)
#define MIP_STIP			((1) << IRQ_S_TIMER)
#define MIP_VSTIP			((1) << IRQ_VS_TIMER)
#define MIP_MTIP			((1) << IRQ_M_TIMER)
#define MIP_SEIP			((1) << IRQ_S_EXT)
#define MIP_VSEIP			((1) << IRQ_VS_EXT)
#define MIP_MEIP			((1) << IRQ_M_EXT)
#define MIP_SGEIP			((1) << IRQ_S_GEXT)

#endif

/*
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Alex Zuepke <azu@sysgo.de>
 *
 * (C) Copyright 2002
 * Gary Jennejohn, DENX Software Engineering, <gj@denx.de>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>

#include <asm/arch/s3c24x0_cpu.h>
#include <asm/proc-armv/ptrace.h>
#include <asm/io.h>

/*    15625 * 200 = 3125000   */
#define TimerCycle  15625
#define TimerOverLoop	200

#define TIMER_MASK (1 <<14)
static void LED_Timer_Init(void);

int	arch_interrupt_init	()
{
	LED_Timer_Init();
	return 0;
}

void do_irq (struct pt_regs *pt_regs)
{
	struct s3c24x0_interrupt *irq = s3c24x0_get_base_interrupt();
	struct s3c24x0_gpio *IO_Base = s3c24x0_get_base_gpio();
	u_int32_t ClearSourceTemp = 0; 
	u_int32_t intpnd = readl(&irq->intpnd);
	static u_int8_t TimerCount = 0;
	static u_int8_t LED_Flag = 0;
	ulong GPF_DAT_Temp;

	if(intpnd & TIMER_MASK)
	{
		TimerCount++;
		if(TimerCount >= TimerOverLoop)
		{
			TimerCount = 0;
			if(LED_Flag)
			{
				LED_Flag = 0;
				GPF_DAT_Temp = readl(&IO_Base->gpfdat) | (0x02 << 4);
				writel(GPF_DAT_Temp, &IO_Base->gpfdat);
			}
			else
			{
				LED_Flag = 1;
				GPF_DAT_Temp = readl(&IO_Base->gpfdat) & (~(0x02 << 4));
				writel(GPF_DAT_Temp, &IO_Base->gpfdat);
			}
		}
		ClearSourceTemp = readl(&irq->srcpnd) | TIMER_MASK;
		writel(ClearSourceTemp, &irq->srcpnd);
		ClearSourceTemp = readl(&irq->intpnd) | TIMER_MASK;
		writel(ClearSourceTemp, &irq->intpnd);
	}
}

void LED_Timer_Init(void)
{
	printf("%s\n", __func__);
	struct s3c24x0_timers *timers = s3c24x0_get_base_timers();
	struct s3c24x0_interrupt *S3C_Intr = s3c24x0_get_base_interrupt();
	struct s3c24x0_gpio *IO_Base = s3c24x0_get_base_gpio();
	ulong tmr;
	ulong IntrTemp = 0;
	ulong GPF_DAT_Temp;

	//turn off all LED
	GPF_DAT_Temp = readl(&IO_Base->gpfdat) | (0x07 << 4);
	writel(GPF_DAT_Temp, &IO_Base->gpfdat);

	/* use PWM Timer 4 because it has no output */
	/* prescaler for Timer 4 is 16 */
	/* PCLK = 50MHz, 50/16 = 3.125MHz */
	writel(0x0f00, &timers->tcfg0);

	/* load value for 10 ms timeout */
	writel(TimerCycle, &timers->tcntb4);
	/* auto load, manual update of timer 4 */
	tmr = (readl(&timers->tcon) & ~0x0700000) | 0x0600000;
	writel(tmr, &timers->tcon);

	printf("enable timer4 interrupt\n");
	//IRQ
	IntrTemp = readl(&S3C_Intr->intmod) & (~TIMER_MASK);
	writel(IntrTemp, &S3C_Intr->intmod);
	
	//enable interrupt
	IntrTemp = readl(&S3C_Intr->intmsk) & (~TIMER_MASK);
	writel(IntrTemp, &S3C_Intr->intmsk);

	printf("start timer4\n");
	/* auto load, start timer 4 */
	tmr = (tmr & ~0x0700000) | 0x0500000;
	writel(tmr, &timers->tcon);
}

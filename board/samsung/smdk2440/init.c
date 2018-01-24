/********************************************************
ï¿½Ë²ï¿½ï¿½Ö³ï¿½Ê¼ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Îªï¿½Ë´ï¿½NANDï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½NANDÊ¹ï¿½ï¿½ubootï¿½ÐµÄ´ï¿½ï¿½ï¿½
********************************************************/

/* NAND FLASHï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ */
#define NFCONF (*((volatile unsigned long *)0x4E000000))
#define NFCONT (*((volatile unsigned long *)0x4E000004))
#define NFCMMD (*((volatile unsigned char *)0x4E000008))
#define NFADDR (*((volatile unsigned char *)0x4E00000C))
#define NFDATA (*((volatile unsigned char *)0x4E000010))
#define NFSTAT (*((volatile unsigned char *)0x4E000020))

/* GPIO */
#define GPHCON              (*(volatile unsigned long *)0x56000070)
#define GPHUP               (*(volatile unsigned long *)0x56000078)


void nand_read_for_start(unsigned int addr, unsigned char *buf, unsigned int len);


int isBootFromNorFlash(void)
{
	volatile int *p = (volatile int *)0;
	int val;

	val = *p;
	*p = 0x12345678;
	if (*p == 0x12345678)
	{
		/* Ð´ï¿½É¹ï¿½, ï¿½ï¿½nandï¿½ï¿½ï¿½ï¿½ */
		*p = val;
		return 0;
	}
	else
	{
		/* NORï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ú´ï¿½Ò»ï¿½ï¿½Ð´ */
		return 1;
	}
}

void copy_code_to_sdram_for_start(unsigned char *src, unsigned char *dest, unsigned int len)
{
	nand_read_for_start((unsigned int)src, dest, len);
}

void clear_bss_for_start(void)
{
	extern int __bss_start, __bss_end;
	int *p = &__bss_start;

	for (; p < &__bss_end; p++)
		*p = 0;
}

void nand_init_for_start(void)
{
#define TACLS   0
#define TWRPH0  1
#define TWRPH1  0
	/* ï¿½ï¿½ï¿½ï¿½Ê±ï¿½ï¿½ */
	NFCONF = (TACLS<<12)|(TWRPH0<<8)|(TWRPH1<<4);
	/* Ê¹ï¿½ï¿½NAND Flashï¿½ï¿½ï¿½ï¿½ï¿½ï¿½, ï¿½ï¿½Ê¼ï¿½ï¿½ECC, ï¿½ï¿½Ö¹Æ¬Ñ¡ */
	NFCONT = (1<<4)|(1<<1)|(1<<0);
}

void nand_select_for_start(void)
{
	NFCONT &= ~(1<<1);
}

void nand_deselect_for_start(void)
{
	NFCONT |= (1<<1);
}

void nand_cmd_for_start(unsigned char cmd)
{
	volatile int i;
	NFCMMD = cmd;
	for (i = 0; i < 10; i++);
}

void nand_addr_for_start(unsigned int addr)
{
	unsigned int col  = addr % 2048;
	unsigned int page = addr / 2048;
	volatile int i;

	NFADDR = col & 0xff;
	for (i = 0; i < 10; i++);
	NFADDR = (col >> 8) & 0xff;
	for (i = 0; i < 10; i++);

	NFADDR  = page & 0xff;
	for (i = 0; i < 10; i++);
	NFADDR  = (page >> 8) & 0xff;
	for (i = 0; i < 10; i++);
	NFADDR  = (page >> 16) & 0xff;
	for (i = 0; i < 10; i++);
}

void nand_wait_ready_for_start(void)
{
	while (!(NFSTAT & 1));
}

unsigned char nand_data_for_start(void)
{
	return NFDATA;
}

void nand_read_for_start(unsigned int addr, unsigned char *buf, unsigned int len)
{
	int col = addr % 2048;
	int i = 0;

	/* 1. Ñ¡ï¿½ï¿½ */
	nand_select_for_start();

	while (i < len)
	{
		/* 2. ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½00h */
		nand_cmd_for_start(0x00);

		/* 3. ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ö·(ï¿½ï¿½5ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½) */
		nand_addr_for_start(addr);

		/* 4. ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½30h */
		nand_cmd_for_start(0x30);

		/* 5. ï¿½Ð¶ï¿½×´Ì¬ */
		nand_wait_ready_for_start();

		/* 6. ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ */
		for (; (col < 2048) && (i < len); col++)
		{
			buf[i] = nand_data_for_start();
			i++;
			addr++;
		}

		col = 0;
	}

	/* 7. È¡ï¿½ï¿½Ñ¡ï¿½ï¿½ */
	nand_deselect_for_start();
}

#define PCLK            50000000    // init.cÖÐµÄclock_initº¯ÊýÉèÖÃPCLKÎª50MHz
#define UART_CLK        PCLK        //  UART0µÄÊ±ÖÓÔ´ÉèÎªPCLK
#define UART_BAUD_RATE  115200      // ²¨ÌØÂÊ
#define UART_BRD        ((UART_CLK  / (UART_BAUD_RATE * 16)) - 1)

/* UART registers*/
#define ULCON0              (*(volatile unsigned long *)0x50000000)
#define UCON0               (*(volatile unsigned long *)0x50000004)
#define UFCON0              (*(volatile unsigned long *)0x50000008)
#define UMCON0              (*(volatile unsigned long *)0x5000000c)
#define UTRSTAT0            (*(volatile unsigned long *)0x50000010)
#define UTXH0               (*(volatile unsigned char *)0x50000020)
#define URXH0               (*(volatile unsigned char *)0x50000024)
#define UBRDIV0             (*(volatile unsigned long *)0x50000028)

#define TXD0READY   (1<<2)

/*
 * ³õÊ¼»¯UART0
 * 115200,8N1,ÎÞÁ÷¿Ø
 */
void uart0_init(void)
{
    GPHCON  |= 0xa0;    // GPH2,GPH3ÓÃ×÷TXD0,RXD0
    GPHUP   = 0x0c;     // GPH2,GPH3ÄÚ²¿ÉÏÀ­

    ULCON0  = 0x03;     // 8N1(8¸öÊý¾ÝÎ»£¬ÎÞ½ÏÑé£¬1¸öÍ£Ö¹Î»)
    UCON0   = 0x05;     // ²éÑ¯·½Ê½£¬UARTÊ±ÖÓÔ´ÎªPCLK
    UFCON0  = 0x00;     // ²»Ê¹ÓÃFIFO
    UMCON0  = 0x00;     // ²»Ê¹ÓÃÁ÷¿Ø
    UBRDIV0 = UART_BRD; // ²¨ÌØÂÊÎª115200
}

/*
 * ·¢ËÍÒ»¸ö×Ö·û
 */
void putc_for_test(unsigned char c)
{
    /* µÈ´ý£¬Ö±µ½·¢ËÍ»º³åÇøÖÐµÄÊý¾ÝÒÑ¾­È«²¿·¢ËÍ³öÈ¥ */
    while (!(UTRSTAT0 & TXD0READY));
    
    /* ÏòUTXH0¼Ä´æÆ÷ÖÐÐ´ÈëÊý¾Ý£¬UART¼´×Ô¶¯½«Ëü·¢ËÍ³öÈ¥ */
    UTXH0 = c;
}

void puts_for_test(char *str)
{
	int i = 0;
	while (str[i])
	{
		putc_for_test(str[i]);
		i++;
	}
}

void puthex(unsigned int val)
{
	/* 0x1234abcd */
	int i;
	int j;
	
	puts_for_test("0x");

	for (i = 0; i < 8; i++)
	{
		j = (val >> ((7-i)*4)) & 0xf;
		if ((j >= 0) && (j <= 9))
			putc_for_test('0' + j);
		else
			putc_for_test('A' + j - 0xa);
		
	}
	
}
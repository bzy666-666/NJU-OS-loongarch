// formatted console output -- printf, panic.

#include <stdarg.h>

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "loongarch.h"
#include "defs.h"
#include "proc.h"
#define UART_BASE_PA  0x1FE001E0UL  /* 物理地址 */
#define WIN_BASE      0x9000000000000000UL  /* 窗口基址 */
#define UART_ADDR     (WIN_BASE | UART_BASE_PA)  /* 完整地址 */
#define CUSTOM_VA     0x70000000UL  /* 自定义虚拟地址 */
/* LSR寄存器偏移（状态寄存器） */
#define UART_LSR      0x05
volatile int panicked = 0;

// lock to avoid interleaving concurrent printf's.
static struct {
  struct spinlock lock;
  int locking;
} pr;

static char digits[] = "0123456789abcdef";

static void
printint(int xx, int base, int sign)
{
  char buf[16];
  int i;
  uint x;

  if(sign && (sign = xx < 0))
    x = -xx;
  else
    x = xx;

  i = 0;
  do {
    buf[i++] = digits[x % base];
  } while((x /= base) != 0);

  if(sign)
    buf[i++] = '-';

  while(--i >= 0)
    consputc(buf[i]);
}

static void
printptr(uint64 x)
{
  int i;
  consputc('0');
  consputc('x');
  for (i = 0; i < (sizeof(uint64) * 2); i++, x <<= 4)
    consputc(digits[x >> (sizeof(uint64) * 8 - 4)]);
}
void uart_putc(char c) {
    volatile char *uart = (volatile char *)UART_ADDR;
    
    /* 等待发送就绪 */
    volatile char *lsr = (volatile char *)(UART_ADDR + UART_LSR);
    while ((*lsr & 0x20) == 0);  /* 等待THRE位（发送保持寄存器空） */
    
    *uart = c;
}
void uart_puts(const char *str) {
    while (*str) {
        uart_putc(*str++);
    }
}
void printfa(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    
    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt) {
                case '%':
                    uart_putc('%');
                    break;
                    
                case 'c': {
                    char c = (char)va_arg(args, int);
                    uart_putc(c);
                    break;
                }
                    
                case 's': {
                    char *s = va_arg(args, char*);
                    uart_puts(s);
                    break;
                }
                    
                case 'd': {
                    int val = va_arg(args, int);
                    if (val < 0) {
                        uart_putc('-');
                        val = -val;
                    }
                    char buf[32];
                    int i = 0;
                    
                    do {
                        buf[i++] = '0' + (val % 10);
                        val /= 10;
                    } while (val > 0);
                    
                    while (--i >= 0) {
                        uart_putc(buf[i]);
                    }
                    break;
                }
                    
                case 'x': {
                    unsigned int val = va_arg(args, unsigned int);
                    const char hex[] = "0123456789abcdef";
                    
                    int started = 0;
                    for (int i = 7; i >= 0; i--) {
                        char digit = hex[(val >> (i * 4)) & 0xF];
                        if (digit != '0' || i == 0) {
                            started = 1;
                        }
                        if (started) {
                            uart_putc(digit);
                        }
                    }
                    break;
                }
                    
                case 'l':
                    fmt++;
                    if (*fmt == 'x') {
                        uint64 val = va_arg(args, uint64);
                        const char hex[] = "0123456789abcdef";
                        
                        int started = 0;
                        for (int i = 15; i >= 0; i--) {
                            char digit = hex[(val >> (i * 4)) & 0xF];
                            if (digit != '0' || i == 0) {
                                started = 1;
                            }
                            if (started) {
                                uart_putc(digit);
                            }
                        }
                    }
                    break;
                    
                default:
                    uart_putc('?');
                    break;
            }
        } else {
            uart_putc(*fmt);
        }
        fmt++;
    }
    
    va_end(args);
}

// Print to the console. only understands %d, %x, %p, %s.
void
printf(char *fmt, ...)
{
  va_list ap;
  int i, c, locking;
  char *s;

  locking = pr.locking;
  if(locking)
    acquire(&pr.lock);

  if (fmt == 0)
    panic("null fmt");

  va_start(ap, fmt);
  for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
    if(c != '%'){
      consputc(c);
      continue;
    }
    c = fmt[++i] & 0xff;
    if(c == 0)
      break;
    switch(c){
    case 'd':
      printint(va_arg(ap, int), 10, 1);
      break;
    case 'x':
      printint(va_arg(ap, int), 16, 1);
      break;
    case 'p':
      printptr(va_arg(ap, uint64));
      break;
    case 's':
      if((s = va_arg(ap, char*)) == 0)
        s = "(null)";
      for(; *s; s++)
        consputc(*s);
      break;
    case '%':
      consputc('%');
      break;
    default:
      // Print unknown % sequence to draw attention.
      consputc('%');
      consputc(c);
      break;
    }
  }

  if(locking)
    release(&pr.lock);
}

void
panic(char *s)
{
  pr.locking = 0;
  printf("panic: ");
  printf(s);
  printf("\n");
  panicked = 1; // freeze uart output from other CPUs
  for(;;)
    ;
}

void
printfinit(void)
{
  initlock(&pr.lock, "pr");
  pr.locking = 1;
}

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#include <stdarg.h>

static char digits[] = "0123456789ABCDEF";

static void
putc(int fd, char c)
{
  write(fd, &c, 1);
}

static void
printint(int fd, int xx, int base, int sgn)
{
  char buf[16];
  int i, neg;
  uint x;

  neg = 0;
  if(sgn && xx < 0){
    neg = 1;
    x = -xx;
  } else {
    x = xx;
  }

  i = 0;
  do{
    buf[i++] = digits[x % base];
  }while((x /= base) != 0);
  if(neg)
    buf[i++] = '-';

  while(--i >= 0)
    putc(fd, buf[i]);
}

static void
printptr(int fd, uint64 x) {
  int i;
  putc(fd, '0');
  putc(fd, 'x');
  for (i = 0; i < (sizeof(uint64) * 2); i++, x <<= 4)
    putc(fd, digits[x >> (sizeof(uint64) * 8 - 4)]);
}

// Print to the given fd. Only understands %d, %x, %p, %s.
void
vprintf(int fd, const char *fmt, va_list ap)
{
  char *s;
  int c, i, state;

  state = 0;
  for(i = 0; fmt[i]; i++){
    c = fmt[i] & 0xff;
    if(state == 0){
      if(c == '%'){
        state = '%';
      } else {
        putc(fd, c);
      }
    } else if(state == '%'){
      if(c == 'd'){
        printint(fd, va_arg(ap, int), 10, 1);
      } else if(c == 'l') {
        printint(fd, va_arg(ap, uint64), 10, 0);
      } else if(c == 'x') {
        printint(fd, va_arg(ap, int), 16, 0);
      } else if(c == 'p') {
        printptr(fd, va_arg(ap, uint64));
      } else if(c == 's'){
        s = va_arg(ap, char*);
        if(s == 0)
          s = "(null)";
        while(*s != 0){
          putc(fd, *s);
          s++;
        }
      } else if(c == 'c'){
        putc(fd, va_arg(ap, uint));
      } else if(c == '%'){
        putc(fd, c);
      } else {
        // Unknown % sequence.  Print it to draw attention.
        putc(fd, '%');
        putc(fd, c);
      }
      state = 0;
    }
  }
}

void
fprintf(int fd, const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  vprintf(fd, fmt, ap);
}

void
printf(const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  vprintf(1, fmt, ap);
}



#define BUFSIZE 1024
static char inbuf[BUFSIZE];
static int bufpos = 0;
static int buflen = 0;

// 从缓冲区读取一个字符
static int
_getc(int fd)
{
  // 如果缓冲区为空，填充缓冲区
  if(bufpos >= buflen) {
    bufpos = 0;
    buflen = read(fd, inbuf, sizeof(inbuf));
    if(buflen <= 0) return -1;
  }
  return inbuf[bufpos++] & 0xff;
}

// 跳过一个字符（简单的回退）
static void
_ungetc(void)
{
  if(bufpos > 0) bufpos--;
}

// 跳过空白字符
static void
_skipspace(int fd)
{
  int c;
  while(1) {
    c = _getc(fd);
    if(c == -1) break;
    if(c != ' ' && c != '\t' && c != '\n' && c != '\r') {
      // 把字符放回缓冲区
      _ungetc();
      break;
    }
  }
}

// 读取整数
static int
_readint(int fd, int base, int *sign)
{
  int n = 0;
  int c;
  int got = 0;
  
  *sign = 1;
  _skipspace(fd);
  
  c = _getc(fd);
  if(c == '-') {
    *sign = -1;
    c = _getc(fd);
  } else if(c == '+') {
    c = _getc(fd);
  }
  
  while(c != -1) {
    int d = -1;
    if(c >= '0' && c <= '9')
      d = c - '0';
    else if(base == 16) {
      if(c >= 'a' && c <= 'f')
        d = c - 'a' + 10;
      else if(c >= 'A' && c <= 'F')
        d = c - 'A' + 10;
    }
    
    if(d >= 0 && d < base) {
      n = n * base + d;
      got = 1;
    } else {
      // 不是数字，放回缓冲区
      _ungetc();
      break;
    }
    c = _getc(fd);
  }
  
  return got ? n * (*sign) : 0;
}

// 读取字符串
static int
_readstr(int fd, char *s, int max)
{
  int c;
  int i = 0;
  
  _skipspace(fd);
  
  while(i < max - 1) {
    c = _getc(fd);
    if(c == -1 || c == ' ' || c == '\t' || c == '\n' || c == '\r') {
      if(c != -1) _ungetc();
      break;
    }
    s[i++] = c;
  }
  s[i] = 0;
  return i > 0;
}

// 读取字符
static int
_readchar(int fd, char *ch)
{
  int c = _getc(fd);
  if(c == -1) return 0;
  *ch = c;
  return 1;
}

// 核心vscanf函数
int
vscanf(int fd, const char *fmt, va_list ap)
{
  int count = 0;
  int i, state;
  int c;
  
  state = 0;
  for(i = 0; fmt[i]; i++) {
    c = fmt[i];
    
    if(state == 0) {
      if(c == '%') {
        state = '%';
      } else if(c == ' ') {
        _skipspace(fd);
      } else {
        int ch = _getc(fd);
        if(ch != c) {
          if(ch != -1) _ungetc();
          return count;
        }
      }
    } else if(state == '%') {
      switch(c) {
      case 'd': {
        int *ip = va_arg(ap, int*);
        int sign;
        int val = _readint(fd, 10, &sign);
        if(ip) *ip = val;
        count++;
        break;
      }
      case 'x': {
        int *ip = va_arg(ap, int*);
        int sign;
        int val = _readint(fd, 16, &sign);
        if(ip) *ip = val;
        count++;
        break;
      }
      case 's': {
        char *sp = va_arg(ap, char*);
        if(_readstr(fd, sp, 100))
          count++;
        break;
      }
      case 'c': {
        char *cp = va_arg(ap, char*);
        if(_readchar(fd, cp))
          count++;
        break;
      }
      case '%': {
        int ch = _getc(fd);
        if(ch != '%') {
          if(ch != -1) _ungetc();
          return count;
        }
        break;
      }
      default:
        _ungetc();  // 把%放回
        state = 0;
        i--;  // 重新处理这个字符
        continue;
      }
      state = 0;
    }
  }
  return count;
}

// scanf函数
int
scanf(const char *fmt, ...)
{
  va_list ap;
  int n;
  
  va_start(ap, fmt);
  n = vscanf(0, fmt, ap);  // 0 = 标准输入
  va_end(ap);
  
  return n;
}

// fscanf函数
int
fscanf(int fd, const char *fmt, ...)
{
  va_list ap;
  int n;
  
  va_start(ap, fmt);
  n = vscanf(fd, fmt, ap);
  va_end(ap);
  
  return n;
}

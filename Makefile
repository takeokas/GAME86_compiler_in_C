
all: gc86 rt.com rt-raw.com # gc86-raw

gc86: gc86.c
	cc  -DDOS -o gc86  gc86.c

gc86-raw: gc86.c
	cc -o gc86-raw gc86.c

rt.com: rt.asm
	 nasm -f bin -l rt.lst -o rt.com -dDOS  $<  # bin

rt-raw.com: rt.asm
	 nasm  -f bin -l rt-raw.lst -o rt-raw.com -dRAW $<  # bin

clean: rt.com rt-raw.com gc86 gc86-raw
	rm -f rt.com rt-raw.com gc86 gc86-raw


DATE :=$(shell date +'%Y%m%d-%H%M%S')
tar:
	(cd ..; tar cvJf gc86-${DATE}.tgz gc86 )


all: gc86 rt.com

gc86: gc86.c
	cc -o gc86  gc86.c

rt.com: rt.asm
	 nasm -f bin -l rt.lst -o rt.com  $<  # bin


DATE :=$(shell date +'%Y%m%d-%H%M%S')
tar:
	(cd ..; tar cvJf gc86-${DATE}.tgz gc86 )

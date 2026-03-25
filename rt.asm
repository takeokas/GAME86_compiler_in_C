;--------------------------------------------------------------------
;    RUN TIME ROUTINE for GAME86
;              in NASM
;                                           
;    original by  Jun Mizutani, March.27 1989
;--------------------------------------------------------------------

	CPU 8086
%ifdef DOS
COMOFFSET equ 100h
%endif
%ifdef RAW
COMOFFSET equ 0
%endif

CODE SEGMENT  BYTE
;          ASSUME   DS:data
RNDSEED    EQU  01F0H+COMOFFSET
OBJEND     EQU  01F4H+COMOFFSET
REMAINDER  equ  01F8H+COMOFFSET
ESEG       equ  01FCH+COMOFFSET
INBUFFER   EQU  0234H+COMOFFSET ;'[' ;01FEH
;;RNDSEED    EQU  01C0H
;;OBJEND     EQU  01C2H
;;REMAINDER  equ  01C4H
;;ESEG       equ  01C6H
;;INBUFFER   EQU  01C8H
;RNDSEED    EQU  03FAH
;OBJEND     EQU  03F8H
;INBUFFER   EQU  03F0H
;
;DATAAREA   EQU  1000H
;
; SS: 02000h
;STACKSPACE EQU  0000H
; code
;;;USERSTART  EQU  0200H+COMOFFSET
USERSTART  EQU  0300H+COMOFFSET
;


;          ASSUME   CS:CODE
	;org 000h, 0400h:0000h
start_off equ 0000h+COMOFFSET
;        TIMES start_off-($-$$) DB 00h   ;0FFh
	org start_off

	section .text

entrypoints:	
     JMP near  STARTUP
     JMP near putc
     JMP near PRINT
     JMP near CRLF
     JMP near TAB
     JMP near HEX2
     JMP near HEX4
     JMP near PRINT_RIGHT
     JMP near PRINT_LEFT
     JMP near getc
     JMP near INPUT
     ;JMP near ABSS		;obsolete
     JMP near RND
     JMP near IRETX
     STI ; EI
      RET
      NOP
     CLI ; DI
      RET
      NOP
     JMP near intEntry
     JMP near NMIEntry
     JMP near div0Entry
     HALT:
	HLT
	NOP
	NOP
	
	;; 
intEntry:
;	push di
;	push si
;	push ax
;	push bx
;	push cx
;	push dx
	jmp intTest	; jump to Int routine
NMIEntry:
	push di
	push si
	push ax
	push bx
	push cx
	push dx
	jmp 0	; jump to NMI routine
;	jmp nmiTest	; jump to NMI routine

div0Entry:
	push di
	push si
	push ax
	push bx
	push cx
	push dx
	jmp 0	; jump to 0div routine
	;; 
IRETX:
	pop dx ;drop PC
IRETX1:
	pop dx
	pop cx
	pop bx
	pop ax
	pop si
	pop di
	sti
	iret
	;; 
CRLF:                ; CR/LF
     MOV  AL,0DH
     CALL putc
     MOV  AL,0AH
     CALL putc
     RET
;
TAB:              ; out Spaces, AX has # of spaces 
     MOV  CX,AX
     MOV  AL,' '
   TAB1:
     CALL putc
     LOOP TAB1
     RET
;
HEX4:                ; output 4 column hexadecimal no.
	PUSH AX
	MOV  AL,AH
	CALL HEX2
	POP  AX
	CALL HEX2
	RET

HEX2:             ; output 2 column hexadecimal no.
	MOV  CH,AL
	MOV  CL,4
	SHR  AL,CL
	CALL HEX1
	MOV  AL,CH
HEX1:
        and al,0Fh
        add al, 90h
        daa
        adc al, 40h
        daa
	jmp putc

RND:   ; AX returns random #
     PUSH BX
     PUSH DI
     PUSH AX
     MOV  DI,RNDSEED            ; work for seed
     MOV  AX,3D09H
     MOV  BX,CS:[DI]
     MUL  BX
     INC  AX
     MOV  CS:[DI],AX
     POP  BX
     MOV  AL,AH
     XOR  AH,AH
     MUL  BX
     MOV  AL,AH
     XOR  AH,AH
     INC  AX
     POP  DI
     POP  BX
     RET


PRINT:     ; DI : Address  CX : No of Char
     TEST CX,CX
     JZ   PR2
  PR1:
     MOV  AL,CS:[DI]
     CALL putc
     INC  DI
     LOOP PR1
  PR2:
     RET

PRINT_LEFT:
     TEST AX,AX
     JNS  PL1
     NEG  AX
     PUSH AX
     MOV  AL,'-'
     CALL putc
     POP  AX
   PL1:
     XOR  CX,CX
     MOV  BX,10000
     CALL PLSUB
     MOV  BX,1000
     CALL PLSUB
     MOV  BX,100
     CALL PLSUB
     MOV  BX,10
     CALL PLSUB
     ADD  AL,'0'
     CALL putc
     RET
   PLSUB:
     XOR  DX,DX
     DIV  BX
     PUSH DX
     TEST AX,AX
     JZ   PS1
     ADD  AL,'0'
     CALL putc
     INC  CX
     JMP  PS2
   PS1:
     TEST CX,CX
     JZ   PS2
     MOV  AL,'0'
     CALL putc
   PS2:
     POP  AX
     RET
   ;ENDP

PRINT_RIGHT:     ; CX:column  AX:number
     TEST AX,AX
     PUSH AX
     JNS  PRR1
     NEG  AX
     DEC  CX
   PRR1:
     MOV  BX,10
   PRR2:
     DEC  CX
     JLE  PRR4
     CMP  AX,BX
     JB   PRR3
     PUSH AX
     MOV  AX,10
     MUL  BX
     MOV  BX,AX
     POP  AX
     JMP  PRR2
   PRR3:
     MOV  AL,' '
     CALL putc
     LOOP PRR3
   PRR4:
     POP  AX
     CALL PRINT_LEFT
     RET
     ;ENDP

BS:                ; Back Space
     MOV  AL,08H
     CALL putc
     MOV  AL,' '
     CALL putc
     MOV  AL,08H
     CALL putc
     RET


INPUT:              ; INPUT NUMBER
     PUSH BX
     PUSH DI
     PUSH SI
     MOV  DI,INBUFFER         ; BUFFER
     XOR  CX,CX
     XOR  SI,SI
   IN1:
     CALL getc
     CMP  AL,'-'              ; NEGATIVE #
     JNZ  IN2
     TEST SI,SI
     JNZ  IN11
     CALL putc
     MOV  SI,1
   IN11:
     JMP  IN1
   IN2:
     CMP  AL,08H              ; BS
     JNZ  IN3
     TEST CX,CX
     JZ   IN3
     CALL BS
     DEC  CX
     DEC  DI
     JMP  IN1
   IN3:
     CMP  AL,0DH              ; CR
     JZ   IN4
     CMP  AL,'0'
     JB   IN31
     CMP  AL,'9'
     JA   IN31
     CALL putc
     MOV  CS:[DI],AL
     INC  DI
     INC  CX
     CMP  CX,6                ; 6 characters
     JAE  IN32
     JMP  IN1
   IN32:
     DEC  CX
     DEC  DI
     CALL BS
   IN31:
     JMP  IN1
   IN4:
     CALL CRLF
     MOV  DI,INBUFFER
     XOR  AX,AX
   IN5:
     TEST CX,CX
     JZ   IN6
     XOR  BH,BH
     MOV  BL,CS:[DI]
     SUB  BL,'0'
     MOV  DX,10
     MUL  DX
     ADD  AX,BX
     INC  DI
     LOOP IN5
     TEST SI,SI
     JZ   IN6
     NEG  AX
   IN6:
     POP  SI
     POP  DI
     POP  BX
     RET


ABSS:  ; abs(AX)
;     TEST AX,AX
;     JNS  ABSS1
;     NEG  AX
;  ABSS1:
;     RET
	hlt

;;; ;;;;;;;;;
; takep8088
USARTD  equ     00h     ;8251 data register
USARTC  equ     01h     ;8251 control register
;
%ifdef DOS
;--------------------------
; DOS I/O
;--------------------------
putc:
     push ax
     push bx
     push cx
     push dx
     MOV  AH,02H
     MOV  DL,AL
     INT  21H
     pop dx
     pop cx
     pop bx
     pop ax
     RET
getc:   
     push bx
     push cx
     push dx
     MOV  AH,08H
     INT  21H
     XOR  AH,AH
     pop dx
     pop cx
     pop bx
     RET
%endif
%ifdef RAW	
;--------------------------
; Serial I/O
;--------------------------
putc:   push ax
putc1:  in      al,USARTC       ;Get status
        and     al,01h          ;check TxBUF empty
        jz      putc1           ;wait for empty
        pop     ax              ;Restore char
        out     USARTD,al       ;Out it
        ret
        ;; end of putc

getc:   
getc1:
        ;;; himatubusi SW->LED
        in al,04h               ;read SW1
        add al,al
        add al,al
        out 0Ch,al              ;out LED
        ;;; 
        in      al,USARTC       ;Get status
        and     al,02h          ;check RxBUF full
        jz      getc1           ;wait for empty
        in      al,USARTD       ;Get Char
        ret
        ;; end of getc
%endif
;;; ;;;;;;;;;;;;;

STARTUP:
     MOV  BX,CS
     MOV  ES,BX
     MOV  DS,BX
     ;MOV  DI,OBJEND	; &"&"
     ;MOV  AX,CS:[DI]	;AX = &

     ;ADD  AX,STACKSPACE	;new SP = &+StackSpace
     ;MOV  SP,AX
     ;MOV  CL,4
     ;SHR  AX,CL			;AX=sp/16
     ;ADD  AX,BX			;AX=AX+CS
     ;INC  AX
     ;MOV  DS,AX
     ;ADD  AX,DATAAREA
     ;MOV  BX,AX
     ;MOV  AH,4AH
     ;INT  21H
     MOV  BX,USERSTART
     JMP  BX
	;;
intTest:
	push ax
	;;; 
	mov ax,[0x202]
	inc ax
	mov [0x202],ax
	in ax,4 ;sysreg
	and ax,2 ;time int?
	;jz inttst0
	out 08h,ax ;time int clr
	add ax,ax
	add ax,ax
inttst0:	
	or ax,3
	out 0Ch,ax ;sysreg
	;;; 
	pop ax
	sti
	iret
	;jmp IRETX1
;nmiTest:	
;	inc word [0x21a]
;	iret

;;; ;
;start_end equ 2ffh+COMOFFSET
;        TIMES start_end-($-$$) DB 41h   ;0FFh
;	db 41h

;END
	;mov di,ax
	;mov al,ES:[di]


	;mov bx,[0c0ah]
	;mov ES,bx
	;pop di
	;mov ES:[di],al

	;or ax,ax
	;jz xxx
	;;;mov ax,0FFFFh
	;xor ax,ax
	;dec ax
;xxx:	inc ax

	;and bx,ax
	;or bx,ax
	;xor bx,ax
	;mov dx,ax
	;in al,dx
	;mov dx,bx
	;out dx,al
	;push ax
	;pop dx

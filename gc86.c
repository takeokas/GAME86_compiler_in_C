/*---------------------------------------------------------------*
  GAME 86 Compiler in C by take@takeoka.net

   Jun Mizutani (mizutani.jun@nifty.or.jp) 氏 作のPascal版を、Cに書き直した。
   オリジナル:  https://www.mztn.org/game86/

  たけおか版の
  独自拡張文法
  - IO port access 「^] 。 ^:expIOadr) で、IOポートを1Byte アクセス
  - 単項演算子: ~ (バイナリ インバート)
  - 二項演算子:  &(and)  |(or)  ^(xor)
  - 行の先頭アドレスを参照。単行演算子「@」,  @行番号(十進定数)で、行の先頭アドレスが得られる
  - セグメント指定付き 1Byte アクセス。
    特殊変数「_」を使用する。
    先んじて、SEG variable '_'に  _=exp で、セグメントの値を設定する。
     値のセット   _:expoff)=exp でセグメントとoffsetを指定してexpの値をセット。
     参照、式中の  _:expoff) でセグメントとoffsetを指定してアクセス。
  
  生成バイナリの、メモリ・マップ

  RAW
   CS,DS:40h |--------------------| 400h:0000h
             |   RUNTIME          |
             |--------------------| 100h
             |   RUNTIME Work     |
             |--------------------| 200h
             |     VARIABLE       |
             |--------------------| 300h
             |  user OBJECT CODE  |
             |                    |
   SS:2000h  |--------------------| 020000h:0
             |   STACK            |            | MAX 64 KB 
             |--------------------| FFFF

  DOS
   CS,DS:40h |--------------------| 400h:0100h
             |   RUNTIME          |
             |--------------------| 200h
             |   RUNTIME Work     |
             |--------------------| 300h
             |     VARIABLE       |
             |--------------------| 400h
             |  user OBJECT CODE  |
             |                    |
   SS:2000h  |--------------------| 020000h:0
             |   STACK            |            | MAX 64 KB 
             |--------------------| FFFF



        === オリジナル MS-DOS, Pascal 版 ===
  *    GAME 86 Compiler   for 8086      ver. 1.00B
   Jun Mizutani (mizutani.jun@nifty.or.jp) 氏 作
    https://www.mztn.org/game86/
  *
  *    CS,SS  |--------------------| 0000       -                 *
  *           |   PSP              |            | MIN  2 KB       *
  *           |--------------------| 0100       |                 *
  *           |   RUNTIME          |            | MAX 64 KB       *
  *           |--------------------| 03F0       |                 *
  *           |   RUNTIME Work     |            |                 *
  *           |--------------------| 0400       |                 *
  *           |   OBJECT CODE      |            |                 *
  *           |--------------------|            |                 *
  *           |   STACK            | 1024 bytes |                 *
  *           |--------------------|            -                 *
  *      DS   |     VARIABLE       | 0000       |                 *
  *           |                    |            | 64 KB           *
  *           |--------------------| FFFF       -                 *
  *                                                               *
  *  by Jun Mizutani                                              *
  *  from March 2, 1989  to  March 27, 1989                       *
  *  modified for MS-Windows  April 4, 1999                       *
  *  0100..03EF : RunTime     03F0..03FF : RunTime Work           *
  *---------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h> 

#define  hi(x) (((x)>>8)& 0xFF)
#define  lo(x) ((x)& 0xFF)
#define  True 1
#define  False 0

int Dos=1;
int ComOffset =0x100;      /*Offset for DOS *.COM file*/
int  RTAdr; // may be ComOffset. Runtime routines Top address, Top of Object code
char *RTName ="rt.com";  // for DOS

  /* Run-time Routine Address for MS-DOS GAME86 compiler     */
int OUTCH;
int PRINT;
int CRLF;
int TAB;
int HEX2;
int HEX4;
int PRRIGHT;
int PRLEFT;
int INCH;
int INPUT;
/*int ABSS    = 0x0021+RTAdr; /* obsolete*/
int RND;
int IRET;
int STI;
int CLI;
int INTEntry;


void
setup(int dos)
{
  if(dos){
    ComOffset= 0x100;      /*Offset for .COM file*/
    RTName= "rt.com";
  }else{ //RAW
    ComOffset= 0x0;      /*Offset for  RAW .COM file*/
    RTName= "rt-raw.com";
  }

  RTAdr= ComOffset;
  /* Run-time Routine Address for MS-DOS GAME86 compiler     */
  OUTCH   = 0x0003+RTAdr;
  PRINT   = 0x0006+RTAdr;
  CRLF    = 0x0009+RTAdr;
  TAB     = 0x000C+RTAdr;
  HEX2    = 0x000F+RTAdr;
  HEX4    = 0x0012+RTAdr;
  PRRIGHT = 0x0015+RTAdr;
  PRLEFT  = 0x0018+RTAdr;
  INCH    = 0x001B+RTAdr;
  INPUT   = 0x001E +RTAdr;
  /*ABSS    = 0x0021+RTAdr; /* obsolete*/
  RND     = 0x0021+RTAdr;//0x0024;
  IRET    = 0x0024+RTAdr;
  STI     = 0x0027+RTAdr;
  CLI     = 0x002A+RTAdr;
  INTEntry = 0x002D+RTAdr;
}
//
#define RT_WORK (ComOffset+0x100)
#define VARTop (0x200+ComOffset)        /* Top of Variable Area   */
#define ObjOffset (0x300+ComOffset) /* Offset for Object  */

#define RNDSEED     (RT_WORK+0xF0)    /*//0x1C0*/
#define ObjEnd      (RT_WORK+0xF4)    //0x1C2
#define REMAINDER   (RT_WORK+0xF8) //0x1C4
#define ESEG        (RT_WORK+0xFC)//0x1C6
//const int RNDSEED   = 0x2FA;
//const int ObjEnd    = 0x2F8;

//const int MAXSOURCE = 30000;               /* Source size in bytes */
//const int MAXOBJ    = 20000;               /* Object size in bytes */
//const int MAXLINES  = 2000;                /* Maximum source lines */
#define  MAXSOURCE  30000               /* Source size in bytes */
#define  MAXOBJ  20000               /* Object size in bytes */
#define  MAXLINES   2000                /* Maximum source lines */

#define ERROR_CLOSE_BRACKET     1
#define ERROR_DQ                2
#define ERROR_reserved          3
#define ERROR_INVALID_TERM      4
#define ERROR_INVALID_ARRAY     5
#define ERROR_INVALID_VARIABLE  6
#define ERROR_IF                7
#define ERROR_OUTPUT_COMMAND    8
#define ERROR_LINENO            9
#define ERROR_INVALID_COMMAND   10
#define ERROR_CR_FORMAT         11

extern void Expression();    /* Code Generation , BX returns the value */
extern int linetopAdr(int K);

char FileName[128];                /* Source file            */
char *SaveFileName=NULL;
char out_fn[128];

unsigned char SourceCode[MAXSOURCE];  /* Source Text        */
unsigned char ObjCode[MAXOBJ]; /* Store Object Code      */

int Verb=0;
int List=0;

//int  SrcPtr;                     /* Ptr to currrent post'n */
int  S;                     /* Ptr to currrent post'n */

int  Adr;                     /* Ptr to currrent post'n */
//int  DataBegin;                     /* Top of Variable Area   */
int    K,H,Z,Q;                     /* Work Variable          */
int    SourceLine;                  /* Pointer for Line Table */
int    LineNo;                  /* Current Line No.       */


struct lnoTable{
  int ln;  /* Line Number     */
  int adr;  /* Object address  */
};

struct lnoTable  LineTable[MAXLINES];

int    CtrlPtr;                   /* Pointer for CtrlTable */

struct forvar{
  int typ; /* FOR:1 or DO:2   */
  int vari; /* variable address*/
  int adr; /* Object address  */
};

struct forvar  CtrlTable[100]; /* Jump Address for Control Statement*/

int    SourceBegin, SourceEnd, ObjectEnd;
int    Pass;                 /* This is 2 pass compiler */
int    p;                 /* file extension handling */

//procedure RunTimeCode; external;
 /*$L RT2.OBJ */

void
SetRunTime()
{
  FILE *r;
  int i,x;

  r=fopen(RTName, "r");
  if(r == NULL){
    printf("%s can not open\n",RTName);
    exit(0);
  }
  for(i=RTAdr;;i++){
    x=getc(r);
    if(x == EOF) break;
    ObjCode[i]=x;
  }
  fclose(r);
  printf("%s:start=0x%x,RT-end=0x%x,", SaveFileName, RTAdr,i);
}


#if 0
void
AdvPtr()                        /* Advance Text pointer */
{
  S++;
}
#endif

void
ChangePtr(int i)        /* Change Text Pointer */
{
  S += i;
}

unsigned char
ReadChar(int i)  /* Get a char. from Source Text */
{
    return SourceCode[S + i];
}

void
SkipSpaces()
{
  while( ReadChar(0) == ' '){
    S++; //AdvPtr();
  }
}

void
PutObj(int i,unsigned int op) /* Put Object Code into Array */
{
  if(i < 0 ){ Adr--; return;}
  if(i == 1){
        ObjCode[Adr] = lo(op);
        Adr++;
  }else if(i == 2){
        ObjCode[Adr] = lo(op);
        ObjCode[Adr+1] = hi(op);
        Adr += 2;
  }
}

void
PutAdr(unsigned int op)       /* Put Relative Address into Array */
{
  Adr += 2;
  ObjCode[Adr-2] = lo(op-Adr);
  ObjCode[Adr-1] = hi(op-Adr);
}

void
Error(int en)            /* Compile Error */
{
  int i;
  char *estr;

  switch(en){
  case ERROR_CLOSE_BRACKET    : estr = " ) reqired."; break;
  case ERROR_DQ               : estr = " \" required."; break;
  case ERROR_reserved         : estr = " "; break;
  case ERROR_INVALID_TERM     : estr = " term "; break;
  case ERROR_INVALID_ARRAY    : estr = " array "; break;
  case ERROR_INVALID_VARIABLE : estr = " variable"; break;
  case ERROR_IF               : estr = " IF"; break;
  case ERROR_OUTPUT_COMMAND   : estr = " Output Statement"; break;
  case ERROR_LINENO           : estr = " Line no"; break;
  case ERROR_INVALID_COMMAND  : estr = " Command "; break;
  case ERROR_CR_FORMAT        : estr = " CRLF "; break;
  }
  printf("\n Error at %x,  Line#:%d,  Error  :%s\n",
	 S,LineTable[SourceLine-1].ln, estr);
  /*printf("\n Error at ",S,"  Line#:",LineTable[SourceLine-1].ln,
    "   Error  :", estr);*/
  for(int i= -10;i<=10;i++){
    if(((S + i) > 0) && ((S + i) <= SourceEnd)){
      if(i < 10){
	if( SourceCode[S+i] >= ' '){
	  printf("%c",SourceCode[S+i]);
	}else{
	  printf(" ");
	}
      }else if( SourceCode[S+i] >= 0x80){
	printf(" ");
      }
    }
    else printf(" ");
  }
  printf("\n");
  for(int i= -10;i<=10;i++){
    if( i == 0)printf("^");
    else printf("-");
  }
  exit(0);
}

void
PushBX()      /* Code Gen.     'PUSH BX', with Optimization */
{
    PutObj(1,0x53);                                       /* PUSH  BX */
    if( ObjCode[Adr-2] == 0x5B) Adr = Adr-2;           /* POP   BX */
}

void
XchgAXBX()    /* Code Gen.  'XCHG AX,BX', with Optimization */
{
    PutObj(1,0x93);                                    /* XCHG AX,BX  */
    if( ObjCode[Adr-2] == 0x93) Adr -= 2;
}

int
GetNo()
{
/* Get No. from Source, No. is returned by gloval var.: K */
  int E = -1;
  int c;
  //printf(" GetNo() ");
  K = 0;
  c= ReadChar(0);
  //printf("===%c===",c);
  if( ReadChar(0)=='$'){
    S++; //AdvPtr();
#if 0
    if(' '==c || c=='\n'){
      //printf(" inchar ");
      PutObj(1,0xE8);
      PutAdr(INCH);         /* CALL INCHAR */
      return -1;
    }
#endif
    for(;;){
      c= ReadChar(0);
      if('0'<=c && c<='9'){
	E = c-'0'; K = K*16+E;
	S++; //AdvPtr();
      }else if('A'<=c && c<='F'){
	E = c-'A' + 10;	K = K*16+E;
	S++; //AdvPtr();
      }else
	break;
    }
    //printf(" Hex=%x ",K);
    return K;
  }else{  // decimal No.
    for(;;){
      E = ReadChar(0)-'0';
      if(!((E>=0) && (E<=9))){
	//printf(" Dec=%d ",K);
	return K;
      }
      K = K*10+E;
      S++; //AdvPtr();
    }    //until (E < 0) or (E > 9);
  }
} /* GetNo */

void
ArrayIndex(int n,int Z) /* Code Gen. BX returns Address */
{
  S++; //AdvPtr();
  Expression();
  if( ReadChar(0) != ')'){
    Error(ERROR_INVALID_ARRAY);
  }
  XchgAXBX();
  if(Z >=0){ // variable
    PutObj(2,0x1E8B);                               /* MOV BX,[z] */
    PutObj(2,Z);
    PutObj(2,0xC301);                               /* ADD BX,AX  */
  }else{
    // IO port, no code required
  }
  if( n != 1) PutObj(2,0xC301);                /* ADD BX,AX  */
  S++; //AdvPtr();
}

int
SignedNum()
{int c;
 int minus=0;
 c = ReadChar(0);
 //printf("---==%c==----",c);
 if((c=='$') && !isxdigit(ReadChar(1))){
   S++;
   //printf("---$INCHAR----");
   return -1; // no value && '$' then INCHAR
 }
 if(c=='-'){
   if(!isdigit(ReadChar(1))){
     //printf("---NEGfunc----");
     return 0;
   }else{
     //printf("---MINUS_DecNum----");
     S++;
     c=ReadChar(0);
     minus=1;
   }
 }
 if((c!='$') &&(!isdigit(c))){
   //printf("---NOTNum----");
   return 0;
 }
 GetNo();
 if(minus) K= -K;
 PutObj(1,0xB8);                           /* MOV  AX,k */
 PutObj(2,K);
 return 1;
}

void
Term()        /* Code Generation,  AX returns the value */
{
  char cch;
  int ln;
  //printf(" Term() ");

  //SkipSpaces();
  if(SignedNum()) return;
  cch = ReadChar(0);
  S++; //AdvPtr();
  switch(cch){
  case '"' :
    K = ReadChar(0);
    ChangePtr(2);
    PutObj(1,0xB8);                         /* MOV  AX,k */
    PutObj(2,K);
    break;
  case '&' :
    K = ObjectEnd;                        /* MOV  AX,k */
    PutObj(1,0xB8);
    PutObj(2,K);
    break;
  case '(' :
    PushBX();
    Expression();
    XchgAXBX();
    PutObj(1,0x5B);                         /* POP BX */
    if( ReadChar(0) != ')') Error(ERROR_CLOSE_BRACKET);
    S++; //AdvPtr();
    break;
  case '\'':                    /* (0 < RND < AX) -> AX */
    Term();
    PutObj(1,0xE8);  PutAdr(RND);           /* CALL RANDOM */
    break;
  case '%' :
    Term();
    PutObj(1,0xA1);                         /* MOV AX,[REMAINDER] */
    PutObj(2,REMAINDER);
    //PutObj(2,0xF202);
    break;
  case '-' :
    Term();
    PutObj(2,0xD8F7);                       /* NEG AX */
    break;
  case'+' : //abs(AX)
    Term();
    //PutObj(1,0xE8);   PutAdr(ABSS);         /* CALL ABS */
    PutObj(1,0x85);    PutObj(1,0xC0);//TEST AX,AX
    PutObj(1,0x79);    PutObj(1,0x02);//JNS  +2
    PutObj(1,0xF7);    PutObj(1,0xD8);//NEG  AX

    /*270 00000184 85C0                         TEST AX,AX
      271 00000186 7902                         JNS  ABSS1
      272 00000188 F7D8                         NEG  AX
      273                                    ABSS1: */
    break;
  case '#' :
    Term();
    ///////PutObj(2,0xD0F7);                       /* NOT AX */
    PutObj(1,0x09); PutObj(1,0xC0); /* or ax,aX */
    PutObj(1,0x74); PutObj(1,0x03); /* jz +03 */
    ///PutObj(1,0xB8);PutObj(1,0xFF); PutObj(1,0xFF); /* mov ax,0FFFFh */
    PutObj(1,0x31); PutObj(1,0xC0); /* xor ax,ax */
    PutObj(1,0x48);		    /* dec ax */
    //jz here
    PutObj(1,0x40);		    /* inc ax */
    break;
  case '~' : // bits invert
    Term();
    PutObj(2,0xD0F7);                       /* NOT AX */
    break;
  case '?' :
    PutObj(1,0xE8);   PutAdr(INPUT);        /* CALL INPUT */
    break;
  case '@' : // get line top address
    GetNo();
    PutObj(1,0xB8);                     /* mov ax,ln */
    //PutObj(1,0xBB );/* MOV bx,ln */
    int la=linetopAdr(K);
    PutObj(2,la );/*  line address */
    break;

  default:
    if(cch == '_' && ReadChar(0)==':'){ // read  1Byte from ES
      ArrayIndex(1,-1); //offset

      PutObj(1,0x8B);PutObj(1,0x1E);PutObj(2,ESEG);/* MOV bx,[ESEG] */
      PutObj(1,0x8E);PutObj(1,0xC3);/* mov ES,bx */

      PutObj(1,0x89);PutObj(1,0xC7);/* mov di,ax */
      PutObj(1,0x26);PutObj(1,0x8A);PutObj(1,0x05);/* mov al,[ES:di] */

      PutObj(2,0xE430);                 /* XOR AH,AH */
      break;
    }
    if(cch == '^' && ReadChar(0)==':'){ // In from input Port 1Byte
      ArrayIndex(1,-1); //IN port
      //PutObj(2,0x078A);                   /* MOV AL,[BX] */
      //PutObj(2,0xE430);                   /* XOR AH,AH */
      PutObj(2,0xC289);                 /* mov dx,ax */
      PutObj(1,0xEC);                   /*  in al,dx */
      PutObj(2,0xE430);                 /* XOR AH,AH */
      break;
    }
    if(!( 'A'<= cch && cch <='Z')){
     Error(ERROR_INVALID_TERM);
    }
    //  'A'..'Z' :
    /* skip following A..Z */
    while((ReadChar(0)>='A') && (ReadChar(0) <= 'Z')){
      S++; //AdvPtr();
    }
    H = (cch - 'A') * 2 + VARTop; // DataBegin;
    if( ReadChar(0)==':'){              /* Array 1 byte */
      ArrayIndex(1,H);
      PutObj(2,0x078A);                   /* MOV AL,[BX] */
      PutObj(2,0xE430);                   /* XOR AH,AH */
    }else if( ReadChar(0)=='('){          /* Array 2 byte */
      ArrayIndex(2,H);
      PutObj(2,0x078B);                   /* MOV AX,[BX] */
    }else{
      PutObj(1,0xA1);                     /* Simple Var */
      PutObj(2,H);                       /* MOV AX,[h] */
    }
    break; /* 'A'..'Z' */
  } /* case cch of */
} /* Term */

void
RelativeOp(int H) /* Code Gen. for :  >, >=, <, <=, <>, = */
{
    PutObj(2,0xC339);                               /* CMP BX,AX */
    PutObj(1,0xBB);                                 /* MOV BX,0001 */
    PutObj(2,0x0001);
    PutObj(1,H);                                   /* Jcc 02 */
    PutObj(1,0x02);
    PutObj(2,0xDB31);                               /* XOR BX,BX */
}

void
Expression()    /* Code Generation , BX returns the value */
{
  char cch;
  int  flg;
  //printf(" Expression() ");

  Term();                /* 1st term --> BX */
  XchgAXBX();
  flg = False;
  do{
    cch = ReadChar(0);
    S++; //AdvPtr();
    switch(cch){
    case '&' :
      Term();     /* 2nd term --> AX */
      PutObj(2,0xC321);                    /* and BX,AX */
      break;
    case '|' :
      Term();     /* 2nd term --> AX */
      PutObj(2,0xC309);                    /* or BX,AX */
      break;
    case '^' :
      Term();     /* 2nd term --> AX */
      PutObj(2,0xC331);                    /* xor BX,AX */
      break;

    case '+' :
      Term();     /* 2nd term --> AX */
      PutObj(2,0xC301);                    /* ADD BX,AX */
      break;
    case  '-' :
      Term();     /* 2nd term --> AX */
      PutObj(2,0xC329);                    /* SUB BX,AX */
      break;
    case  '*' :
      Term();     /* 2nd term --> AX */
      XchgAXBX();
      PutObj(2,0xEBF7);                    /* IMUL BX */
      XchgAXBX();
      break;
    case  '/' :
      Term();     /* 2nd term --> AX */
      XchgAXBX();

      //332 000001B2 89D9                            mov cx,bx
      //333 000001B4 F7F9                            idiv cx

      ////PutObj(2,0xD989);                    /* mov cX,bx */
      ////PutObj(2,0xF9F7);                    /* IDIV cx   */
      ///PutObj(2,0xDA89);                    /* mov DX,bx */
      ///PutObj(2,0xFAF7);                    /* IDIV DX   */
      PutObj(2,0xD231);                    /* XOR DX,DX */
      PutObj(2,0xFBF7);                    /* IDIV BX   */

      PutObj(2,0x1689); PutObj(2,REMAINDER);  /* MOV [REMAINDER],DX*/
      //PutObj(2,0x168B); PutObj(2,0xF202);   /* MOV [REMAINDER],DX*/
      XchgAXBX();
      break;
    case  '=' :
      Term();                               /* JZ  xx */
      H = 0x74;
      RelativeOp(H);
      break;
    case  '<' :
      if( ReadChar(0) == '>'){
	S++; //AdvPtr();
	Term();
	H = 0x75;                       /* JNZ  xx */
      }else if( ReadChar(0) == '='){
	S++; //AdvPtr();
	Term();
	H = 0x7E;                       /* JLE xx */
      }else{
	Term();
	H = 0x7C;                       /* JL  xx */
      }
      RelativeOp(H);
      break;
    case '>' :
      if( ReadChar(0) == '='){
	S++; //AdvPtr();
	Term();
	H = 0x7D;                       /* JGE xx */
      }else{
	Term();
	H = 0x7F;                       /* JG  xx */
      }
      RelativeOp(H);
      break;
    default:
      ChangePtr(-1);
      SkipSpaces();
      flg = True;
    }
  }while(!flg);  //until flg;
}

/*-----------------------------------------------------------------------*/

int
linetopAdr(int K)
{
  int j;
  if(Pass == 1){ return 0; }

  for(j=0;;j++){
    if(LineTable[j].ln >= K) return LineTable[j].adr;
  }
}

void
PutJumpAdr(int Q )  /* write line-top address */
{
  int la;

  PutObj(1,Q); // instruction
  if( Pass == 1){
    PutObj(2,0);
    return;
  }
  la=linetopAdr(K);
  //printf(" **PutJumpAdr:K=%d,Q=%x,adr=%x** ",K,Q,la);
  PutAdr(la);
}

void
JumpCode(int Q)      /* Goto Statement */
{
  if( ReadChar(0) == '-'){
    S++; //AdvPtr();
    GetNo();
    K = 0x7FFF;
    PutJumpAdr(Q);
  }
  GetNo();
  PutJumpAdr(Q);
}

void
ReturnCmd()
{
  S++; //AdvPtr();
  PutObj(1,0xC3);                                 /* RET  */
}

void
GotoCmd(int Q)
{
  if( ReadChar(1) == '='){        /* Goto or Gosub */
        ChangePtr(2);
        JumpCode(Q);
  }else{     /* On XX goto */
    S++; //AdvPtr();
    Term();
    S++; //AdvPtr();
    Q = 0xE9;                                  /* JMP xxxx */
    do{
      PutObj(1,0x48);                           /* DEC AX */
      PutObj(2,0x0375);                         /* JNZ 03 */
      JumpCode(Q);
      S++; //AdvPtr();
    }while(ReadChar(-1) == ',');   //   until ReadChar(-1) != ',';
    ChangePtr(-1);
  }
} /* GotoCom */

void
VariableCmd()     /* A= , A:x)= , A(x)=  or  FOR */
{
    if(ReadChar(0)=='_'){
      Z= -2; //S++; //AdvPtr();
    }else if(ReadChar(0)=='^'){
      Z= -1; //S++; //AdvPtr();
    }else{
      Z = 2*(ReadChar(0)-'A')+VARTop; //DataBegin;
      while( (ReadChar(1)>='A') && (ReadChar(1)<='Z')){
	S++; //AdvPtr();
      }
    }
    if( ReadChar(1) == '='){  /*Simple Var  or FOR */
        ChangePtr(2);
        Expression();
        PutObj(2,0x1E89);                           /* MOV [z],BX */
        PutObj(2,Z);
        if( ReadChar(0) == ','){                  /* FOR statement */
	  S++; //AdvPtr();
	  CtrlPtr = CtrlPtr + 1;
	  CtrlTable[CtrlPtr].typ  = 1; /* for */
	  CtrlTable[CtrlPtr].vari = Z;
	  Expression();
	  PushBX();                                /* PUSH BX */
	  CtrlTable[CtrlPtr].adr = Adr;
	}
    }else if( Z == -2 && ReadChar(1)==':'){//  ES:off 1Byte
      S++; //AdvPtr();
      ArrayIndex(1,-1); // offset
      //PutObj(1,0x53);                             /* PUSH BX */
      PutObj(1,0x50);                             /* PUSH ax */
      S++; //AdvPtr();              /* skip '=' */
      Expression();
      XchgAXBX();
      PutObj(1,0x8B);PutObj(1,0x1E);PutObj(2,ESEG);/* MOV bx,[ESEG] */
      PutObj(1,0x8E);PutObj(1,0xC3);/* mov ES,bx */

      PutObj(1,0x5F);                             /* POP  di */
      PutObj(1,0x26);PutObj(1,0x88);PutObj(1,0x05); /* mov ES:[di],al */
    }else if( Z == -1 && ReadChar(1)==':'){// out to Port 1Byte
      S++; //AdvPtr();
      ArrayIndex(1,-1); //out port
      //PutObj(1,0x53);                             /* PUSH BX */
      PutObj(1,0x50);                             /* PUSH ax */
      S++; //AdvPtr();              /* skip '=' */
      Expression();
      XchgAXBX();
      //PutObj(1,0x5B);                             /* POP  BX */
      //PutObj(2,0xDA89);                           /* MOV dx,BX */
      /////PutObj(2,0x0788);                           /* MOV [BX],AL */
      PutObj(1,0x5A);                             /* POP  dx */
      PutObj(1,0xEE);                             /* out dx,al */
    }else if( ReadChar(1) == ':'){  /*Array 1 byte */
      S++; //AdvPtr();
      ArrayIndex(1,Z);
      PutObj(1,0x53);                             /* PUSH BX */
      S++; //AdvPtr();              /* skip '=' */
      Expression();
      XchgAXBX();
      PutObj(1,0x5B);                             /* POP  BX */
      PutObj(2,0x0788);                           /* MOV [BX],AL */
    }else if(ReadChar(1) == '('){  /*Array 2 byte */
      S++; //AdvPtr();
      ArrayIndex(2,Z);
      PutObj(1,0x53);                             /* PUSH BX  */
      S++; //AdvPtr();              /* skip '=' */
      Expression();
      XchgAXBX();
      PutObj(1,0x5B);                             /* POP  BX  */
      PutObj(2,0x0789);                           /* MOV [BX],AX */
    }else
      Error(ERROR_INVALID_VARIABLE);
}

void
IfCmd()
{
  if( ReadChar(1) != '='){ Error(ERROR_IF);}
  ChangePtr(2);
  Expression();
  PutObj(2,0xDB85);                               /* TEST BX,BX */
  PutObj(2,0x0375);                               /* JNZ  03    */
  Q = 0xE9;                                      /* JMP  xxxx  */
  K = LineNo+1;     /* False : Goto Next Line */
  PutJumpAdr(Q);
}

void
DoCmd()
{
  //printf(" DoCmd() ");
  S++; //AdvPtr();
  if( ReadChar(0) != '='){                 /* Do */
        CtrlPtr = CtrlPtr + 1;
        CtrlTable[CtrlPtr].adr  = Adr;
        CtrlTable[CtrlPtr].typ  = 2;          /* 2 : do */
  }else{                                       /* Until or  Next */
    if( CtrlTable[CtrlPtr].typ  == 1){    /* NEXT */
      S++; //AdvPtr();
      Expression();
      Z = CtrlTable[CtrlPtr].vari;
      PutObj(2,0x1E89); PutObj(2,Z);      /* MOV [Z],BX */
      XchgAXBX();
      PutObj(1,0x5B);                     /* POP   BX   */
      PutObj(2,0xD839);                   /* CMP AX,BX  */
      PutObj(2,0x047F);                   /* JGE   04   */
      PutObj(1,0x53);                     /* PUSH BX    */
      PutObj(1,0xE9);
      PutAdr(CtrlTable[CtrlPtr].adr);    /* JMP xxxxx  */
      CtrlPtr = CtrlPtr - 1;
    }else{                                   /* UNTIL */
      S++; //AdvPtr();
      Expression();
      PutObj(2,0xDB85);                   /* TEST BX,BX */
      PutObj(2,0x0375);                   /* JNZ  03    */
      PutObj(1,0xE9);
      PutAdr(CtrlTable[CtrlPtr].adr);    /* JMP xxxx   */
      CtrlPtr = CtrlPtr - 1;
    }
  }
} /* DoCmd */

void
OutString()
{
  int  i,c;
  //printf(" OutString() ");

  PutObj(1,0xE9);                                 /* JP xxxx */
  Q = Adr+2;
  i = 0;
  S++;//  AdvPtr();
  while( (c=ReadChar(0)) != '"'){
    //printf(" %c ",c);
    if(c == '\n'){ break;/*Error(ERROR_DQ);*/}
    ObjCode[Q+i] = ReadChar(0);
    S++; //AdvPtr();
    i++;
  }
  S++; //AdvPtr();
  PutAdr(Q+i);                                   /* jump here */
  Adr = Adr+i;                      /* CX <- No of Ch,  DI <- Address */
  PutObj(1,0xB9);  PutObj(2,i);                   /* MOV CX,i   */
  PutObj(1,0xBF);  PutObj(2,Q);/*PutObj(2,Q+ComOffset);/* MOV DI,q   */
  PutObj(1,0xE8);  PutAdr(PRINT);                 /* CALL OUTCHAR  */
}

void
OutNum()
{
  //printf(" OutNum() ");

  if( ReadChar(1) == '='){
    ChangePtr(2);
    Expression();
    XchgAXBX();
    PutObj(1,0xE8);   PutAdr(PRLEFT);          /* CALL PR-LEFT */
  }else if( ReadChar(1) == '?'){
    ChangePtr(3);
    Expression();
    XchgAXBX();
    PutObj(1,0xE8);   PutAdr(HEX4);             /* CALL HEX4 */
  }else if( ReadChar(1) == '$'){
    ChangePtr(3);
    Expression();
    XchgAXBX();
    PutObj(1,0xE8);   PutAdr(HEX2);             /* CALL HEX2 */
  }else if( ReadChar(1) == '('){      /* ?(5)= : CX <- 5 , AX <- No. */
    S++; //AdvPtr();
    Term();
    PutObj(2,0xC189);                           /* MOV  CX,AX */
    S++; //AdvPtr();
    Expression();
    XchgAXBX();
    PutObj(1,0xE8);   PutAdr(PRRIGHT);          /* CALL PR-RIGHT */
  }
  else{ Error(ERROR_OUTPUT_COMMAND);}
}

/*-----------------------------------------------------------------------*/
void
Statement()
{
  char N;
  //printf(" Statement() ");

  //SkipSpaces();
  N = ReadChar(0);
  //printf(" N=%c",N);
  switch(N){
  case '"'  :  OutString();return;
  case '#'  :  GotoCmd(0xE9);return;                      /*  JP  xxxx */
  case '!'  :  GotoCmd(0xE8);return;                      /* CALL xxxx */
  case ']'  :  ReturnCmd();return;
  case '/'  :             /* CALL CR/LF */
    S++; //AdvPtr();             
    PutObj(1,0xE8);
    PutAdr(CRLF);
    return;
  case '?'  :  OutNum();return;
  case ';'  :  IfCmd();return;
  case '@'  :  DoCmd();return;
  case '.'  :      /* Out Spaces */
    ChangePtr(2);
    Expression();
    XchgAXBX();                        /* XCHG AX,BX  */
    PutObj(1,0xE8);  PutAdr(TAB);     /* CALL TABOUT */
    return;
  case '>'  :                /* Call User Routine */
      ChangePtr(2);
      GetNo();
      PutObj(1,0xE8);  PutAdr(K);       /* CALL k */
      return;
  case '$'  :                 /* Out 1 Char */
    ChangePtr(2);
    Expression();
    XchgAXBX();                        /* XCHG AX,BX   */
    PutObj(1,0xE8);  PutAdr(OUTCH);   /* CALL OUTCHAR */
    return;
  case '\'' :                  /* SET RANDOM SEED */
    ChangePtr(2);
    Expression();
    PutObj(2,0x1E89);
    PutObj(2,RNDSEED);/*MOV [RNDSEED],BX */
    return;
  case '_' :     /* SET ESEG */
    if(ReadChar(1)!='='){ break; /*not simple then fall to variable*/ }
    ChangePtr(2);
    Expression();
    PutObj(2,0x1E89);
    PutObj(2,ESEG);/*MOV [ESEG],BX */
    return;
  case ' '  :   SkipSpaces();return; ////S++;return;/* AdvPtr();return;*/
  default: ; // fall to variable
  }
  // variable
  if(!(( 'A'<= N && N <='Z')|| '^' == N || '_' == N)){
    Error(ERROR_INVALID_COMMAND);
  }
  //  'A'..'Z', '^', '_'
  VariableCmd();return;
}

void
Rem() /*skip line*/
{
   while(ReadChar(0) != '\n'){
     S++; //AdvPtr();
   }
   S++; //AdvPtr();
}

void
Fin()
{
  LineTable[SourceLine].ln = 0x7FFF;
  LineTable[SourceLine].adr = Adr;
  PutObj(1,0xF4);          /* HLT  */
  //PutObj(1,0xB8);   PutObj(2,0x004C);          /* MOV AX,4C00    */
  //PutObj(1,0xCD);   PutObj(1,0x21);            /* INT MSDOS      */
}


void
Gen()           /*** 1-2 PASS ROUTINE ***/
{
  //printf(" Gen() ");
  //Pass=pas;

l99: /* ** TOP OF LINE ** */
 if((ReadChar(0) == 0x1A) /*|| (S > SourceEnd)*/){  /* ^Z, End of File */
   Fin();
   return;
 }
 if((ReadChar(0) == '\n')){  /* null line */
   S++;
   goto l99;
 }

 if(('0' <=ReadChar(0)) && (ReadChar(0) <= '9'))  GetNo(); /* Get Line No.   */
 else Error(ERROR_LINENO);
 LineNo = K;
 if( Pass == 1){
   //printf(" Line No: %4d,  Address =%04x ",LineNo,Adr);
   //write(^M" Line No:",LineNo:4,"  Address. = ",Adr:4,"  ");
   LineTable[SourceLine].ln = LineNo;
   LineTable[SourceLine].adr = Adr;
   SourceLine++;
 }
 if( ReadChar(0) != ' '){                 /*** REM ***/
   Rem();
   goto l99; /*Top of Line*/
 }

 S++; //AdvPtr();
 do{                               /*** STATEMENT ***/
   SkipSpaces();
   Statement();
 }while( ReadChar(0) != '\n');// until ReadChar(0) = ^M; /* until End of Line */
 S++; //AdvPtr();
 //if( ReadChar(0) != '\n'){ Error(ERROR_CR_FORMAT);}
 //S++; //AdvPtr();
 goto l99; /*TopLine*/
}

void
Load(char *SourceName)
{
  FILE *Source;
  int i,x;

  Source=fopen(SourceName, "r");
  if(Source == NULL){
    //printf("%s can not open\n",SourceName);
    exit(0);
  }
  for(i=0;;i++){
    x=getc(Source);
    if(x == EOF) break;
    SourceCode[i]=x;
  }
  fclose(Source);
  SourceEnd = i - 1;
  SourceCode[i]= 0x1A; // put ^Z, EOF
  if(Verb){
    printf("  Loaded %d bytes\n",i);
  }
}

void
Save(char *DestName, int ObjectEnd)
{
  FILE *Dest;//file of byte;
  int i;

  Dest=fopen(DestName, "w");
  if(Dest == NULL){
    printf("%s can not open\n",DestName);
    exit(0);
  }
  //for(i=0;i <= ObjectEnd; i++){
  for(i=ComOffset;i <= ObjectEnd; i++){
    putc(ObjCode[i], Dest);
  }
  fclose(Dest);
}

/*-----------------------------------------------------------------------*/
void
title()
{
  printf("GAME86 Compiler  under Linux  v.1.00 by S.Takeoka\n");
  printf("original GAME86 Compiler  MS-DOS  v.1.00C by Jun 4/4/99\n");
}

char *cmdname;
void
usage()
{
  title();
  printf("%s [-r] [-o OutFileName]  Source_filename\n", cmdname);
}

int
do_args(int argc, char *argv[])
{
  int opt;
  while ((opt = getopt(argc, argv, "rlvo:")) != -1) {
        switch (opt) {
	case 'r':
	  Dos = 0;
	  break;
	case 'v':
	  Verb = 1;
	  break;
	case 'l':
	  List = 1;
	  break;
	case 'o':
	  SaveFileName = optarg; // optarg は引数の値
	  break;
	case '?':
	case 'h':
	default:
	  usage();
	  printf("-r : raw binary(start address=0000h)\n-o FileNmae :output file");
	  exit(1);
	  //return -1;
        }
    }
  return optind;
}

void
main(int argc, char *argv[])
{
  cmdname= argv[0];

  int optind=do_args(argc, argv);

  if(Verb){
    title();
  }

  if( (argc-optind)==1){
    strcpy(FileName,argv[optind]);  //source file name
  }else{
    usage();
    exit(1);
  }

  if(SaveFileName == NULL){
    SaveFileName= out_fn;
    strncpy(SaveFileName, FileName,sizeof(out_fn));
    char *p= rindex(SaveFileName, '.');
    if(p ==NULL){
      strncat(SaveFileName, ".com",sizeof(out_fn));
    }else{
      strcpy(p, ".com");
    }
  }
  //printf("!!FileName=%s!!dos=%d,SaveFileName=%s\n",FileName,Dos,SaveFileName);

  setup(Dos);

  Load(FileName );
  //Load(FileName  + ".GM");


  SetRunTime();
  //printf("\n");
  Adr = ObjOffset;
  S = 0;
  CtrlPtr = 0;
  SourceLine  =  0;

  Pass = 1;
  Gen();
  ObjectEnd = Adr;
  ObjCode[ObjEnd]  =lo(ObjectEnd);     /* 0x03F8 */
  ObjCode[ObjEnd+1]=hi(ObjectEnd);     /* 0x03F9 */

  Adr = ObjOffset;
  //DataBegin = 0;
  S = 0;
  Pass = 2;
  Gen();

  printf("end-adr=0x%x\n",Adr-1);
  Save(SaveFileName,ObjectEnd);

  if(List){
    printf("line#: adr\n");
    for(int i=0;;i++){
       printf(" %4d: %04x\n",
	      LineTable[i].ln,   LineTable[i].adr );
       if(LineTable[i].ln ==  0x7FFF) break;
    }
  }
  //printf("\nEND\n");
}

/* EOF */

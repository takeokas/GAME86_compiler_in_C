10 A=&+1
20 B=@210
30 C=@1000
100--
110 "game test"
120 "B="??=B " C="??=C " &="??=& /
200-
240 !=1000
250 " returned"/
255 "Byte access "
260 I=1,10 A:I)=I @=I+1
270 I=1,10 ?(5)=A:I) @=I+1 /
255 "Word access "
280 I=1,10 A(I)=10-I @=I+1
290 I=1,10 ?(5)=A(I) @=I+1 /
300 "Inter Segment Access (ROM) "
310 " set Segment 0FFFFh" _=$FFFF
320 /"dump ROM data:"I=0,15 ?$=_:I) " " @=I+1 
500 /"char input & Hex const"/
505 @
510 B=$ ??=B " "
520 ??=$3210+B /
550 @=(B="q")
999 #=-1
1000 " subroutine "
1010 I=1,5 /"I="?(5)=I @=I+1
1050 ]
2000--
2010 A=1+A B=A+B
2020 B=A/B+1=3
3000 #=-1

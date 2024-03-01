copy /y ext2fsd_dummy.inf Release\

stampinf -d * -v * -f Release\ext2fsd_dummy.inf -a NTx86,NTamd64,NTia64,NTARM,NTARM64 || goto :eof

inf2cat /driver:Release /os:XP_X86,2000,XP_X64,Vista_X86,Vista_X64,7_X86,7_X64,Server2003_X86,Server2003_X64,Server2003_IA64,Server2008_X86,Server2008_X64,Server2008_IA64,Server2008R2_X64,Server2008R2_IA64,8_X86,8_X64,8_ARM,10_RS3_X86,10_RS3_X64,10_RS3_ARM64 || goto :eof

if exist cab\ext2fsd.cab del cab\ext2fsd.cab

if not exist cab mkdir cab

cabarc -p -r n cab\ext2fsd.cab Release\ext2fsd_dummy.inf Release\ext2fsd*.sys Release\ext2fsd*.pdb || goto :eof

if not exist cab\ext2fsd.cab goto :eof

signtool sign /a /as /v /sha1 a065b19d9f5a23bc67bb1cf6872283f504ba04b1 /fd sha256 /tr "http://sha256timestamp.ws.symantec.com/sha256/timestamp" cab\ext2fsd.cab || goto :eof

# .gdbinit  -*- sh -*-

file ccparse
set args -tr error,overload,dependent,template,templateXfer,strict,topform,templateParams,disamb,prettyPrint,printTypedAST in/t0167.cc

# for XML stuff
#set args -tr no-elaborate,xmlPrintAST,xmlPrintAST-types in/t0029.cc
#set args -tr parseXml,no-typecheck,no-elaborate,printAST outdir/t0029.cc.C1.xml_filtered

#set args -tr c_lang,nohashline tmp.c
#set args -tr nohashline tmp.i
#set args in/../../cppstdex/14.5p1.error2.cc
#set args -tr printTypedAST tmp2.cc

#set args in/big/nonport.i

#set args -tr c_lang,topform,error,sizeof tmp.c

#set args tmp.i

#file smin
#set args bar.c bar.c.str ./test-program

#set args -tr checktree astxml_lexer.yy.cc astxml_lexer.yy.cc.str

#set args -tr parseAstXml,stopAfterParse outdir/t0003.cc.A1.xml
#set args -tr xmlPrintAST,xmlPrintAST-indent in/t0003.cc

set print static-members off
break main
break breaker
#break resolveOverload
run

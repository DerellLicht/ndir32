//**********************************************************
//  This file is NOT compiled into the code,
//  it is only used for LINT exceptions.
//**********************************************************

// For some reason, I cannot load this with pclint ??
// #include "lintdefs.refs.h"

//lint -esym(1055, __builtin_offsetof)
//lint -esym(530, SCSI_PASS_THROUGH_WITH_BUFFERS)
//lint -esym(526, __builtin_offsetof)
//lint -esym(628, __builtin_offsetof)

//  wierd new lint9 stuff
//lint -e844  Pointer variable could be declared as pointing to const
//lint -e845  The right argument to operator '|' is certain to be 0
//lint -e705  Argument nominally inconsistent with format (int vs. unsigned int)
//lint -e1786 Implicit conversion to Boolean (return) (int to bool)

//  wierd new lint9 stuff
//lint -e801   Use of goto is deprecated
//lint -e818   Pointer parameter could be declared as pointing to const
//lint -e830   Location cited in prior message
//lint -e831   Reference cited in prior message
//lint -e834   Operator '-' followed by operator '-' is confusing.  Use parentheses. (duh)
//lint -e1776  Converting a string literal to char * is not const safe (arg. no. 1)

//lint -e835   A zero has been given as right argument to operator '|'
//lint -e737  Loss of sign in promotion from int to unsigned long long

// Error 96: Unmatched left brace for linkage specification on line 8, file c:\mingw\include\rpcdce.h
// Error 96: Unmatched left brace for linkage specification on line 12, file c:\mingw\include\rpc.h
//lint -e96    

//  What the heck is this??
//lint -e10    Expecting '}'  ???

//lint -e438   Last value assigned to variable not used
//lint -e525   Negative indentation from line ...
//lint -e539   Did not expect positive indentation from line ...
//lint -e725   Expected positive indentation from line ... 

//lint -e534   Ignoring return value of function
//lint -e641   Converting enum to int
//lint -e713   Loss of precision (unsigned to signed) (assignment)
//lint -e716   while(1) ... 
//lint -e732   Loss of sign (initialization) (size N to size K)
//lint -e734   Loss of precision (assignment) (N bits to K bits)
//lint -e754   local structure member not referenced
//lint -e755   global macro not referenced (reqd for scsi_defs.h)
//lint -e762   Redundantly declared symbol 'sort()' previously declared at line 563
//lint -e768   global struct member not referenced
//lint -e778   Constant expression evaluates to 0 in operation '*'
//lint -e864   Expression involving variable 'brothers' possibly depends on order of evaluation
//lint -e1746  parameter in function could be made const reference


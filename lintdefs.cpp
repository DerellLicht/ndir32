//**********************************************************
//  This file is NOT compiled into the code,
//  it is only used for LINT exceptions.
//**********************************************************

//  wierd new lint9 stuff
//lint -e438  Last value assigned to variable not used
//lint -e844  Pointer variable could be declared as pointing to const
//lint -e845  The right argument to operator '|' is certain to be 0
//lint -e705  Argument nominally inconsistent with format (int vs. unsigned int)
//lint -e1786 Implicit conversion to Boolean (return) (int to bool)

//lint -e87   expression too complicated for #ifdef or #ifndef

//lint -esym(526, __offsetof__)
//lint -esym(628, __offsetof__)
//lint -esym(746, __offsetof__)
//lint -esym(1055, __offsetof__)

//lint -esym(1055, __builtin_offsetof)
//lint -esym(530, SCSI_PASS_THROUGH_WITH_BUFFERS)
//lint -esym(526, __builtin_offsetof)
//lint -esym(628, __builtin_offsetof)

//  new rules for PC-Lint 9.00f
//lint -elib(14) 
//lint -elib(19) 
//lint -elib(40) 
//lint -elib(49) 
//lint -elib(64) 
//lint -elib(85)
//lint -elib(114) 
//lint -elib(129)
//lint -esym(1065, _iob)

//  wierd new lint8 stuff
//lint -e801   Use of goto is deprecated
//lint -e818   Pointer parameter could be declared as pointing to const
//lint -e830   Location cited in prior message
//lint -e831   Reference cited in prior message
//lint -e834   Operator '-' followed by operator '-' is confusing.  Use parentheses. (duh)
//lint -e1776  Converting a string literal to char * is not const safe (arg. no. 1)

//lint -e835   A zero has been given as right argument to operator '|'

// Error 96: Unmatched left brace for linkage specification on line 8, file c:\mingw\include\rpcdce.h
// Error 96: Unmatched left brace for linkage specification on line 12, file c:\mingw\include\rpc.h
//lint -e96    

//  What the heck is this??
//lint -e10    Expecting '}'  ???

//lint -e525   Negative indentation from line ...
//lint -e539   Did not expect positive indentation from line ...
//lint -e725   Expected positive indentation from line ... 

//lint -e534   Ignoring return value of function
//lint -e641   Converting enum to int
//lint -e713   Loss of precision (unsigned to signed) (assignment)
//lint -e716   while(1) ... 
//lint -e732   Loss of sign (initialization) (size N to size K)
//lint -e734   Loss of precision (assignment) (N bits to K bits)
//lint -e737   Loss of sign in promotion from int to unsigned int
//lint -e750   local macro not referenced
//lint -e754   local structure member not referenced
//lint -e755  global macro not referenced
//lint -e768   global struct member not referenced
//lint -e769   global enumeration constant not referenced
//lint -e778   Constant expression evaluates to 0 in operation '*'

//lint -esym(759,hStdIn) -esym(765,hStdIn)
//lint -esym(759,drandom) -esym(765,drandom) -esym(714,drandom) 
//lint -esym(759,dclreos) -esym(765,dclreos) -esym(714,dclreos) 
//lint -esym(759,ngotoxy) -esym(765,ngotoxy)
//lint -esym(759,clear_visible_rows) -esym(765,clear_visible_rows) -esym(714,clear_visible_rows) 
//lint -esym(759,control_handler) -esym(765, control_handler)
//lint -esym(759, perr) -esym(765, perr) 
//lint -esym(759,hide_cursor) -esym(765,hide_cursor) -esym(714,hide_cursor) 
//lint -esym(759,dprints) -esym(765,dprints) -esym(714,dprints) 

//lint -esym(765, dputsi) 
//lint -esym(765, is_CRLF_present) 
//lint -esym(759,dprintc) -esym(765,dprintc) -esym(714,dprintc) 
//lint -esym(759,get_char) -esym(765,get_char) -esym(714,get_char) 
//lint -esym(715,lfn_off) ;
//lint -esym(759,set_text_attr) -esym(765,set_text_attr) -esym(714,set_text_attr)
//lint -esym(759,get_system_message) -esym(765,get_system_message) -esym(714,get_system_message)


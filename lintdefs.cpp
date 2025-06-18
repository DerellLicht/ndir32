//**********************************************************
//  This file is NOT compiled into the code,
//  it is only used for LINT exceptions.
//**********************************************************

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
//lint -e768   global struct member not referenced
//lint -e778   Constant expression evaluates to 0 in operation '*'

//lint -esym(401, __created)   Symbol not referenced
//lint -esym(528, __created)   Symbol not referenced
//lint -esym(843, __created)   Variable could be declared as const

//  warnings/errors caused by PcLint not supporting STL, C++11 standard, TCHAR
//lint -e2     Unclosed Quote
//lint -e18    Symbol redeclared (basic) 
//lint -e19    Useless Declaration
//lint -e26    Expected an expression, found ')'
//lint -e30    Expected an integer constant
//lint -e32    Field size (member 'std::_Vector_base<int,int>::_Vector_impl::_Tp_alloc_type')
//lint -e36    redefining the storage class of symbol 
//lint -e38    Offset of symbol 'std::_Vector_base<int,int>::_Vector_impl_data::pointer'
//lint -e40    Undeclared identifier 'make_unique'
//lint -e42    Expected a statement (TCHAR)
//lint -e46    field type should be an integral or enumeration type
//lint -e48    Bad type
//lint -e52    Expected an lvalue
//lint -e53    Expected a scalar
//lint -e55    Bad type
//lint -e58    Bad type
//lint -e61    Bad type
//lint -e63    Expected an lvalue
//lint -e64    Type mismatch (initialization) (struct ffdata * = int)
//lint -e78    Symbol ... typedef'ed at line ... used in expression
//lint -e110   Attempt to assign to void
//lint -e121   Attempting to initialize an object of undefined type 'void'
//lint -e129   declaration expected, identifier ignored
//lint -e151   Token 'flist' inconsistent with abstract type
//lint -e155   Ignoring { }'ed sequence within an expression, 0 assumed
//lint -e503   Boolean argument to relational
//lint -e515   Symbol has arg. count conflict 
//lint -e516   Symbol has arg. type conflict 
//lint -e521   Highest operation, a 'constant', lacks side-effects
//lint -e522   Highest operation, operator '!=', lacks side-effects
//lint -e526   Symbol not defined
//lint -e530   Symbol not initialized
//lint -e550   Symbol not accessed
//lint -e559   Size of argument no. 2 inconsistent with format
//lint -e560   argument no. 4 should be a pointer
//lint -e592   Non-literal format specifier used without arguments
//lint -e628   no argument information provided for function
//lint -e681   Loop is not entered
//lint -e707   Mixing narrow and wide string literals in concatenation
//lint -e719   Too many arguments for format (1 too many)
//lint -e746   call to function not made in the presence of a prototype
//lint -e808   No explicit type given symbol 'file', int assumed
//lint -e1013  Symbol not a member of class ''
//lint -e1015  Symbol '_M_allocate' not found in class
//lint -e1039  Symbol is not a member of
//lint -e1040  Symbol is not a legal
//lint -e1054  template variable declaration expects a type, int assumed
//lint -e1055  Symbol undeclared, assumed to return int
//lint -e1057  Member '__gnu_cxx::__is_integer_nonstrict<<1>>::__value' cannot be used
//lint -e1062  template must be either a class or a function
//lint -e1070  No scope in which to find symbol 'pointer'
//lint -e1072  Reference variable 'file' must be initialized
//lint -e1077  Could not evaluate default template parameter '=typename _Alloc::value_type'
//lint -e1087  Previous declaration of '__gnu_cxx::__is_signed' (line 88) is incompatible
//lint -e1098  Function template specialization does not match any function template
//lint -e1514  Creating temporary to copy 'int' to 'struct ffdata &' (context: arg. no. 1)
//lint -e1712  default constructor not defined for class 'ffdata'
//lint -e1025  No function matches invocation 
//lint -e1066  Symbol declared as "C" conflicts ...
//lint -e1702  operator 'operator+' is both an ordinary function and something else??
//lint -e1776  Converting a string literal is not const safe (initialization)


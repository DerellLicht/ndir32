//*****************************************************************
//                                                                 
//  NSORT.CPP -   NDIR data-sorting routines                       
//                                                                 
//  Written by:   Daniel D. Miller  (the Derelict)                 
//                                                                 
//*****************************************************************

// #undef __STRICT_ANSI__
#include <windows.h>
// #include <string.h>
#ifdef _lint
#include <malloc.h>
#endif

#include "common.h"
#include "ndir32.h"

//lint -e1013  Symbol 'LowPart' not a member of class '_LARGE_INTEGER'
//lint -e40    Undeclared identifier 'LowPart'
//lint -e63    Expected an lvalue

//***************  function prototypes  ***************
static int sort_name(struct ffdata *a, struct ffdata *b);
static int sort_lfn_name(struct ffdata *a, struct ffdata *b);
static int sort_ext(struct ffdata *a, struct ffdata *b);
static int sort_size(struct ffdata *a, struct ffdata *b);
// static int sort_date(struct ffdata *a, struct ffdata *b);
// static int sort_time(struct ffdata *a, struct ffdata *b);
static int sort_name_rev(struct ffdata *a, struct ffdata *b);
static int sort_lfn_name_rev(struct ffdata *a, struct ffdata *b);
static int sort_ext_rev(struct ffdata *a, struct ffdata *b);
static int sort_size_rev(struct ffdata *a, struct ffdata *b);
// static int sort_date_rev(struct ffdata *a, struct ffdata *b);
// static int sort_time_rev(struct ffdata *a, struct ffdata *b);
static struct ffdata *merge_sort(struct ffdata *c);
static struct ffdata *merge(struct ffdata *a, struct ffdata *b);

//************************************************************
//  the following object is a dummy point structure
//  which is used by merge_sort.  The main code must
//  allocate a strucure for this to point to.
//  
//  A global function pointer is also required by the
//  sort routine.  This will point to a function which
//  accepts two structure pointers as arguments, and
//  returns:
//  
//     >0 if a > b
//    ==0 if a == b
//     <0 if a < b
//  
//************************************************************
static struct ffdata *z = NULL ;
static int (*sort_fcn) (struct ffdata *a, struct ffdata *b) ;

//****************************************************
//  allocate a dummy structure for merge_sort()
//****************************************************
static int init_sort(void) 
{
   // z = new ffdata ;
   z = (struct ffdata *) malloc(sizeof(ffdata)) ;
   if (z == NULL)
      error_exit(OUT_OF_MEMORY, NULL) ;
   z->next = NULL ;
   return DATA_OKAY ;
}

//****************************************************
// void free_file_structs(void)
//    {
//    if (z != NULL)  delete z ;
//    }

//*********************************************************
static int sort_name(struct ffdata *a, struct ffdata *b)
   {
   return(_tcsicmp(a->name, b->name)) ;
   }

//*********************************************************
static int sort_lfn_name(struct ffdata *a, struct ffdata *b)
   {
   return(_tcsicmp(a->filename, b->filename)) ;
   }

//*********************************************************
static int sort_ext(struct ffdata *a, struct ffdata *b)
   {
   return(_tcsicmp(a->ext, b->ext)) ;
   }

//*********************************************************
static int sort_size(struct ffdata *a, struct ffdata *b)
   {
   if (a->fsize > b->fsize)  return(1) ;
   else if (b->fsize > a->fsize)  return(-1) ;
   else return(0) ;
   }

//*********************************************************
// static int sort_date(struct ffdata *a, struct ffdata *b)
//    {
//    if (a->ft > b->ft)  return(1) ;
//    else if (b->ft > a->ft)  return(-1) ;
//    else 
//       return(0) ;
//    }

//*********************************************************
static int sort_date_time(struct ffdata *a, struct ffdata *b)
   {
   LARGE_INTEGER a64, b64 ;
   a64.LowPart  = a->ft.dwLowDateTime ;
   a64.HighPart = a->ft.dwHighDateTime ;
   b64.LowPart  = b->ft.dwLowDateTime ;
   b64.HighPart = b->ft.dwHighDateTime ;
   if (a64.QuadPart > b64.QuadPart)  return(1) ;   //lint !e530
   else if (a64.QuadPart < b64.QuadPart)  return(-1) ;   //lint !e530
   else 
      return(0) ;
   }  //lint !e550

//*********************************************************
static int sort_name_rev(struct ffdata *a, struct ffdata *b)
   {
   return(_tcsicmp(b->name, a->name)) ;
   }

//*********************************************************
static int sort_lfn_name_rev(struct ffdata *a, struct ffdata *b)
   {
   return(_tcsicmp(b->filename, a->filename)) ;
   }

//*********************************************************
static int sort_ext_rev(struct ffdata *a, struct ffdata *b)
   {
   return(_tcsicmp(b->ext, a->ext)) ;
   }

//*********************************************************
static int sort_size_rev(struct ffdata *a, struct ffdata *b)
   {
   if (b->fsize > a->fsize)  return(1) ;
   else if (a->fsize > b->fsize)  return(-1) ;
   else return(0) ;
   }

//*********************************************************
// static int sort_date_rev(struct ffdata *a, struct ffdata *b)
//    {
//    if (b->ft > a->ft)  return(1) ;
//    else if (a->ft > b->ft)  return(-1) ;
//    else 
//       return(0) ;
//    }

//*********************************************************
static int sort_date_time_rev(struct ffdata *a, struct ffdata *b)
   {
   // if (b->ft > a->ft)  return(1) ;
   // else if (a->ft > b->ft)  return(-1) ;
   // else 
   //    return(0) ;
   LARGE_INTEGER a64, b64 ;
   a64.LowPart  = a->ft.dwLowDateTime ;
   a64.HighPart = a->ft.dwHighDateTime ;
   b64.LowPart  = b->ft.dwLowDateTime ;
   b64.HighPart = b->ft.dwHighDateTime ;
   if (a64.QuadPart < b64.QuadPart)  return(1) ;   //lint !e530
   else if (a64.QuadPart > b64.QuadPart)  return(-1) ;   //lint !e530
   else 
      return(0) ;
   }  //lint !e550

//*********************************************************
//  this function returns
//       <0 if (a) is DIR and (b) is not
//       >0 if (b) is DIR and (a) is not
//      ==0 if (a) and (b) are same type (DIR or not)
//*********************************************************
static int sort_dir(struct ffdata *a, struct ffdata *b)
   {
   if (a->dirflag && !(b->dirflag))  return(-1) ;
   else if (b->dirflag && !(a->dirflag))  return(1) ;
   else return(0) ;
   }

//*********************************************************
//  This routine recursively splits linked lists
//  into two parts, passing the divided lists to
//  merge() to merge the two sorted lists.
//*********************************************************
static struct ffdata *merge_sort(struct ffdata *c)
   {
   struct ffdata *a, *b, *prev ;
   int pcount = 0 ;
   int j = 0 ;

   if ((c != NULL) && (c->next != NULL))
      {
      a = c ;
      while (a != NULL)
         {
         pcount++ ;
         a = a->next  ;
         }
      a = c ;
      b = c ;
      prev = b ;
      while (j <  pcount/2)
         {
         j++ ;
         prev = b ;
         b = b->next ;
         }
      prev->next = NULL ;  //lint !e771

      return merge(merge_sort(a), merge_sort(b)) ;
      }
   return c ;
   }

//*********************************************************
//  This routine merges two sorted linked lists.
//*********************************************************
static struct ffdata *merge(struct ffdata *a, struct ffdata *b)
   {
   struct ffdata *c ;
   c = z ;

   do
      {
      int x = sort_fcn(a, b) ;
      if (x <= 0)
         {
         c->next = a ;
         c = a ;
         a = a->next ;
         }
      else
         {
         c->next = b ;
         c = b ;
         b = b->next ;  //lint !e613
         }
      }
   while ((a != NULL) && (b != NULL));

   if (a == NULL)  c->next = b ;  //lint !e613
             else  c->next = a ;  //lint !e613
   return z->next ;
   }

//*********************************************************
//  This intermediate function is used because I want
//  merge_sort() to accept a passed parameter,
//  but in this particular application the initial
//  list is global.  This function sets up the global
//  comparison-function pointer and passes the global
//  list pointer to merge_sort().
//*********************************************************
static void sort_files(int (*current_sort)(struct ffdata *a, struct ffdata *b))
{
   sort_fcn = current_sort ;
   ftop = merge_sort(ftop) ;
}

//*****************************************************************
void sort_filelist(void)
{
   if (z == 0)
      init_sort() ;
   
   //  reverse sort
   if (n.reverse) {
      switch (n.sort) {
      case SORT_EXT:
         if (lfn_supported) 
            sort_files(sort_lfn_name_rev) ;
         else 
            sort_files(sort_name_rev) ;
         sort_files(sort_ext_rev) ;
         break;
      case SORT_NAME:
         sort_files(sort_ext_rev) ;
         if (lfn_supported) 
            sort_files(sort_lfn_name_rev) ;
         else 
            sort_files(sort_name_rev) ;
         break;
      case SORT_SIZE:
         sort_files(sort_size_rev) ;
         break;
      case SORT_DATE:
         sort_files(sort_date_time_rev) ;
         break;
      default:  break ; // make lint happy
      }
   }

   //  normal sort
   else {
      switch (n.sort) {
      case SORT_EXT:
         if (lfn_supported) 
            sort_files(sort_lfn_name) ;
         else 
            sort_files(sort_name) ;
         sort_files(sort_ext) ;
         break;
      case SORT_NAME:
         sort_files(sort_ext) ;
         if (lfn_supported) 
            sort_files(sort_lfn_name) ;
         else 
            sort_files(sort_name) ;
         break;
      case SORT_SIZE:
         sort_files(sort_size) ;
         break;
      case SORT_DATE:
         sort_files(sort_date_time) ;
         break;
      default:  break ; // make lint happy
      }
   }

   if (n.dir_first)
      sort_files(sort_dir) ;
}

//lint +e1013  Symbol 'LowPart' not a member of class '_LARGE_INTEGER'
//lint +e40    Undeclared identifier 'LowPart'
//lint +e63    Expected an lvalue



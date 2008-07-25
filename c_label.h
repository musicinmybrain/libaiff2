/*      $Id$    */

#ifdef __ELF__
# define _C_LABEL(x)    x
#else
# ifdef __STDC__
#  define _C_LABEL(x)   _ ## x
# else
#  define _C_LABEL(x)   _/**/x
# endif
#endif


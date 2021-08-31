#ifndef LIFETABLES
#define LIFETABLES
#ifdef __GNUC__
/* This is used to define an attribute so that the actuarial functions are only calculated
   once, rather than recalculating them each time, because the value only depends on the input. 
   My own created hashtable (axntable) seems better for this, but I added it anyway. */
#define PURE __attribute__ ((__pure__))
#else
#define PURE
#endif

enum lifetables {LXMR, LXFR, LXMK, LXFK, LXFKP, LXNIHIL, LT_AMOUNT};
int lx(register unsigned ltindex, register unsigned age) PURE ;
#endif

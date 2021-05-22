#ifdef __GNUC__
/* This is used to define an attribute so that the actuarial functions are only calculated
   once, rather than recalculating them each time, because the value only depends on the input */
#define CONST __attribute__ ((const))
#else
#define CONST
#endif

enum lifetables {LXMR, LXFR, LXMK, LXFK, LXFKP, LXNIHIL};
int lx(register unsigned int, register int) CONST ;

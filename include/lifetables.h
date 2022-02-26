#ifndef LIFETABLES
#define LIFETABLES
#ifdef __GNUC__
/*
 * This is used to define an attribute so that the actuarial functions are only
 * calculated once, rather than recalculating them each time, because the value
 * only depends on the input. 
 */
#define CONST __attribute__ ((__const__))
#define PURE __attribute__ ((__pure__))
#else
#define CONST
#define PURE
#endif

enum {MAXAGE = 130, LENGTHLINE = 64};
enum {LXMR, LXFR, LXMK, LXFK, LXFKP, LXNIHIL, LT_AMOUNT};
void makeLifeTables(void);
#endif

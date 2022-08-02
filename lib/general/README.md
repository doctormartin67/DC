This library defines quite a few useful functions that are defined in the
header files include/common.h, include/errorexit.h and helperfunctions.h.
By far the most useful will be common.h, which defines:
	stretchy buffers
	an arena allocator
	a hash map
Stretchy buffers are very useful if you want an array of unkwown size for any
type of element. For example if you have a struct S defined and would like to
start an array of these structs, you would simply write something like
struct S *s = 0;
buf_push(s, an_S_struct);
s[0] is now equal to an_S_struct
An arena allocator is just a way to confine memory allocation so that you
don't have mallocs and frees all over the place.

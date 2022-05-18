#include <assert.h>
#include "common.h"
#include "errorjump.h"

void reset_error(void)
{
	if (!error.is_error) {
		assert(!error.str);
		return;
	}
	buf_free(error.str);
	error.is_error = 0;	
}

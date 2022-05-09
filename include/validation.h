#ifndef VALIDATION
#define VALIDATION

void validate_emit_error(const char *fmt, ...);
void validate_reset(void);
unsigned validate_passed(void);
const char *validate_error(void);

#endif

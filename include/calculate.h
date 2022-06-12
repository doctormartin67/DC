#ifndef CALCULATE_H
#define CALCULATE_H
#include "DCProgram.h"

enum {DBO, NC, IC, ASSETS};
enum {DEF, IMM}; // deferred or immediate payment

void update_res_lump_sums(CurrentMember *cm, int k);
void update_death_lump_sums(CurrentMember *cm, int k);
void update_premiums(CurrentMember *cm, int k);
void update_art24(CurrentMember *restrict cm, int k);
void set_dbo_ret(struct retirement *dbo,
		const double art24[const static METHOD_AMOUNT],
		double res, double red_cap, struct factor f);
void set_nc_ret(struct retirement *nc, const double art24[const static
		METHOD_AMOUNT], struct factor f);
void set_ic_nc_ret(struct retirement *ic_nc, const double art24[const static
		METHOD_AMOUNT], struct factor f);
void set_assets(struct assets *assets, double res, double red_cap,
		struct factor f);
void update_EBP(CurrentMember *cm, int k,
		const double art24[const static METHOD_AMOUNT],
		double res, double red_cap);
unsigned in_prolongation(const CurrentMember *cm, int k);

#endif

path = "/home/doctormartin67/Coding/tables/tables/"

EPS = 0.0000000001

def create_table(table):
	f = open(path + table, "r")
	new_table = [int(s.split(",")[1].rstrip()) for s in f]
	return new_table

def lx(table, age):
	if not table in lx.tables:
		lx.tables[table] = create_table(table)
		print("table " + table + " created")

	if age > len(lx.tables[table]) - 1:
		return 0
	elif age < 0:
		return 0
	else:
		return lx.tables[table][age]
lx.tables = {}

def npx(table, ageX, ageXn, corr):
	ageX += corr
	ageXn += corr
	if ageX == ageXn:
		return 1.0
	elif ageX > ageXn:
		return 0.0
	elif ageX < 0 or ageXn < 0:
		return 0.0

	ip1 = 0.0
	ip2 = 0.0
	lxX = 0
	lxX1 = 0
	lxXn = 0
	lxXn1 = 0
	lxX = lx(table, int(ageX))
	lxX1 = lx(table, int(ageX + 1))
	lxXn = lx(table, int(ageXn))
	lxXn1 = lx(table, int(ageXn + 1))

	ip1 = lxX - (ageX - int(ageX)) * (lxX - lxX1)
	ip2 = lxXn - (ageXn - int(ageXn)) * (lxXn - lxXn1)

	if 0 == ip1:
		assert(0 == ip2)
		return 0.0
	else:
		return ip2/ip1

def nEx(table, i, charge, ageX, ageXn, corr):
	im = (1 + i)/(1 + charge) - 1
	n = ageXn - ageX
	vn = 1 / (1 + im) ** n
	nPx = npx(table, ageX, ageXn, corr)
	return vn * nPx
	
def axn(table, i, charge, prepost, term, ageX, ageXn, corr):
	payments = 0
	ageXk = 0.0
	value = 0.0
	termfrac = 0.0

	if ageX > ageXn + EPS:
		value = 0
	elif 12 % term:
		return -1.0
	else:
		termfrac = 1.0 / term
		ageXk = ageX + prepost/term
		payments = int((ageXn - ageX) * term + EPS)
		while (payments):
			value += nEx(table, i, charge, ageX, ageXk, corr)
			ageXk += termfrac
			payments -= 1

	return value

def Ax1n(table, i, charge, ageX, ageXn, corr):
	k = 0
	payments = 0
	im = 0.0
	v = 0.0
	value = 0.0
	kPx = 0.0
	qx = 0.0

	if ageX > ageXn + EPS:
		return 0
	else:
		im = (1 + i)/(1 + charge) - 1
		v = 1/(1 + im)
		payments = int(ageXn - ageX + EPS)

		while (payments):
			kPx = npx(table, ageX, ageX + k, corr)
			qx = (1 - npx(table, ageX + k, ageX + k + 1, corr))
			value += v ** (k + 1.0/2) * kPx * qx
			k += 1
			payments -= 1
		kPx = npx(table, ageX, ageX + k, corr)
		qx = (1 - npx(table, ageX + k, ageXn, corr))
		value += v (k + 1.0/2) * kPx * qx

		return value

def IAx1n(table, i, charge, ageX, ageXn, corr):
	k = 1
	payments = 0
	value = 0.0

	if ageX > ageXn + EPS:
		return 0
	else:
		payments = int(ageXn - ageX + EPS)
		while payments:
			Ax1 = Ax1n(table, i, charge, ageX + k-1, ageX+k, corr)
			Ex = nEx(table, i, charge, ageX, ageX + k - 1, corr)
			value += k * Ax1 * Ex
			k += 1
			payments -= 1

		Ax1 = Ax1n(table, i, charge, ageX + k - 1, ageXn, corr)
		value += k * Ax1 * nEx(table, i, charge, ageX, ageXn, corr)

		return value

# This function hasn't been completed because I don't think I need it.
# At the moment it only works for term = 1
def Iaxn(table, i, charge, prepost, term, ageX, ageXn, corr):
	k = 1
	payments = 0
	ageXk = 0.0
	value = 0.0

	if ageX > ageXn + EPS:
		return 0
	else:
		ageXk = ageX + prepost/term
		payments = int((ageXn - ageX) * term + EPS)
		while payments: 
			value += k * nEx(table, i, charge, ageX, ageXk, corr)
			ageXk += 1.0/term
			k += 1
			payments -= 1

		return value
	
def lump_sum(combination, res, prem, pre_post, term, age, NRA, admin_cost,
	delta_cap, dth_cap, ps_is_mixed, X10, b2):

	if age == NRA:
		return 0.0
	
	if 0 == res and 0 == prem:
		return 0.0

	ax = axn(table, pre_post, term, age, NRA, 0)
	Ex = nEx(table, age, NRA, 0)

	if "UKMS" == combination or "UKZT" == combination:
		lump_sum = (res + prem (1 - admin_cost) * ax) / Ex
	elif "MIXED" == combination:
		Ax1 = Ax1n(table, age, NRA, 0)
		ax_cost = axn(table, 0, 1, age, NRA, 0)
		lump_sum = (res + prem * (1 - admin_cost) * ax)
		if ps_is_mixed:
			lump_sum /= (Ex + 1/X10 * Ax1 + 1/X10 * b2 * ax_cost)
		else:
			lump_sum /= Ex
	elif "UKMT" == combination:
		Ax1 = Ax1n(table, age, NRA, 0)
		ax_cost = axn(table, 0, 1, age, NRA, 0)
		IAx1 = IAx1n(table, age, NRA, 0)
		Iax = Iaxn(table, 0, 1, age, NRA, 0)
		prem *= (1 - admin_cost)
		lump_sum = (res + prem * ax - dth_cap * (Ax1 + b2 * ax_cost) 
			- prem * (IAx1 + b2 * Iax)) / Ex
	else:
		assert(0)

	return lump_sum + delta_cap * (NRA - age) * 12

##############################
# below are all the unit tests
##############################

tables = ["LXMR", "LXFR", "LXMK", "LXFK'", "Lxnihil"]
def test_lx():
	assert(lx("LXFR", 40) == 982954)
	assert(lx("LXFR", 0) == 1000000)
	assert(lx("LXFR", 120) == 0)
	assert(lx("Lxnihil", 0) == 1000000)
	assert(lx("Lxnihil", 1) == 1000000)

def test_npx(i):
	for table in tables:
		nPx = npx(table, i%110, (i+1)%110, 0)
		nPx = npx(table, i%107, (i+1)%107, 3)
		nPx = npx(table, i%110, (i+1)%110, -3)
		assert(1 == npx(table, 40, 40, 0))
		assert(0 == npx(table, 0, 130, 0))
		if 0 == lx(table, i%110 - 3):
			assert(0 == nPx)
		else:
			assert(abs(nPx - lx(table, (i+1)%110 - 3)
				/ lx(table, i%110 - 3)) < EPS)

def test_nEx(i):
	for table in tables:
		ageX = i%110
		ageXn = (i+1)%110
		nex = nEx(table, 0.0, 0.0, ageX, ageXn, -3)
		assert(abs(nex - npx(table, ageX, ageXn, -3)) < EPS)
		nex = nEx(table, 0.01, 0.0, ageX, ageXn, -3)
		assert(abs(nex - npx(table, ageX, ageXn, -3)
					* (1 + 0.01) ** (ageX - ageXn)) < EPS)
		nex = nEx(table, 0.012, 0.001, ageX, ageXn, -3)
		assert(abs(nex - npx(table, ageX, ageXn, -3)
					* ((1 + 0.012) / (1 + 0.001))
						** (ageX - ageXn)) < EPS)
	
def test_axn(i):
	for table in tables:
		ax = axn(table, 0.02, 0.001, 1, 12, i, i, -3)
		assert(0 == ax)
		ax = axn(table, 0.02, 0.001, 0, 1, i, i+1, -3)
		assert(1 == ax)
		ax = axn(table, 0.02, 0.001, 1, 1, i, i+1, -3)
		nex = nEx(table, 0.02, 0.001, i, i+1, -3)
		assert(abs(nex - ax) < EPS)

if __name__ == "__main__":
	N = 1024 
	for i in range(0, N):
		test_lx()
		test_npx(i)
		test_nEx(i)
		test_axn(i)


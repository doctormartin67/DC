import numpy as np
import pandas as pd
import datetime as dt
import actfuncs as af

MAXGEN = 8
MAXPROJ = 128
titles = ("CAP", "CAPPS", "CAPRED", "TAUX", "PREMIUM", "RES", "RESPS", "CAPDTH")

class Member:
#########################
# Assumptions
	DoC = dt.datetime(2016, 12, 1)
	RA_DEF = 65
	infl = 0.018
	cost_res = 0.001
	b2 = 0.0008
	pre_post = 1
	term = 12
	ps_is_mixed = 1
	def ss(self):
		return self.infl + 0.011
	
	def table_ins(self):
		if "UKMS" == self.combination:
			return "Lxnihil"
		elif "DEF" == self.status:
			if 1 == self.sex:
				return "LXMR"
			else:
				assert(2 == self.status)
				return "LXFR"
		else:
			assert("ACT" == self.status)
			if 1 == self.sex:
				return "LXMK"
			else:
				assert(2 == self.sex)
				return "LXFK'"

# Plan rules
	def contrA(self, k):
		ceil1 = self.projections["ceil1"][k]
		ceil2 = self.projections["ceil2"][k]
		nDoE = self.projections["SDoE"][k]
		if not self.active_contract:
			return 0

		if 7229 == self.reg:
			sal = self.projections["sal"][k] * 13.92
			S1 = min(sal, ceil1)
			S2 = max(sal - ceil1, 0)
			if nDoE < 5:
				return (0.02 * S1 + 0.05 * S2) * self.pt
			elif nDoE < 10:
				return (0.03 * S1 + 0.075 * S2) * self.pt
			else:
				return (0.04 * S1 + 0.1 * S2) * self.pt
		elif 14455 == self.reg:
			sal = self.projections["sal"][k] * 13.92
			S1 = min(sal, ceil1, ceil2)
			S2 = max(min(sal, ceil2) - ceil1, 0)
			return (0.05 * S1 + 0.11 * S2) * self.pt
		elif 7204 == self.reg or 7228 == self.reg:
			sal = self.projections["sal"][k] * 13.92
			if nDoE < 10:
				return 0.02 * sal * self.pt
			elif nDoE < 20:
				return 0.03 * sal * self.pt
			else:
				return 0.04 * sal * self.pt
		elif 7203 == self.reg:
			sal = self.projections["sal"][k] * 13.8975
			S1 = min(sal, ceil1)
			S2 = max(sal - ceil1, 0)
			return (0.02 * S1 + 0.08 * S2) * self.pt
		elif 14286 == self.reg:
			sal = self.projections["sal"][k] * 13.92
			S1 = min(sal, ceil1)
			S2 = max(sal - ceil1, 0)
			return (0.03 * S1 + 0.09 * S2) * self.pt
		elif 7161 == self.reg:
			return 444 * self.pt
		else:
			assert(0)

	def contrC(self, k):
		ceil1 = self.projections["ceil1"][k]
		ceil2 = self.projections["ceil2"][k]
		nDoE = self.projections["SDoE"][k]
		if not self.active_contract:
			return 0

		if 7229 == self.reg:
			sal = self.projections["sal"][k] * 13.92
			return 0.015 * sal * self.pt
		elif 14455 == self.reg:
			sal = self.projections["sal"][k] * 13.92
			S1 = min(sal, ceil2)
			return 0.015 * S1 * self.pt
		elif 7204 == self.reg or 7228 == self.reg:
			sal = self.projections["sal"][k] * 13.92
			if nDoE < 10:
				return 0.01 * sal * self.pt
			elif nDoE < 20:
				return 0.015 * sal * self.pt
			else:
				return 0.02 * sal * self.pt
		elif 7203 == self.reg:
			sal = self.projections["sal"][k] * 13.8975
			S1 = min(sal, ceil1)
			S2 = max(sal - ceil1, 0)
			return (0.01 * S1 + 0.04 * S2) * self.pt
		elif 14286 == self.reg:
			sal = self.projections["sal"][k] * 13.92
			S1 = min(sal, ceil1)
			S2 = max(sal - ceil1, 0)
			return (0.015 * S1 + 0.015 * S2) * self.pt

#########################

	def age_at(self, date):
		return af.calc_years(date, self.DoB, 1)
	
	def get_generations(self, name, AorC):
		title = name + "_" + AorC + "_" + "GEN"
		return [self.data[title + str(i)]
			for i in range(1, MAXGEN + 1)] 
	
	def res(self, cap, prem, age, table, i, dth_cap):
		key = str(cap) + str(prem) + str(int(age * 100)) + table + \
			str(i) + str(dth_cap)
		if not key in self.res.values:
			self.res.values[key] = \
			af.reserves(self.combination, cap, prem, self.pre_post,
			self.term, age, self.NRA, table, i, self.cost_res, 
			self.admin_cost, self.delta_cap, dth_cap,
			self.ps_is_mixed, self.X10, self.b2)
		return self.res.values[key]
	res.values = {}

	def ls(self, res, prem, age, table, i, dth_cap):
		key = str(res) + str(prem) + str(int(age * 100)) + table + \
			str(i) + str(dth_cap)
		if not key in self.ls.values:
			self.ls.values[key] = \
			af.lump_sum(self.combination, res, prem, self.pre_post,
			self.term, age, self.NRA, table, i, self.cost_res,
			self.admin_cost, self.delta_cap, dth_cap,
			self.ps_is_mixed, self.X10, self.b2)
		return self.ls.values[key]
	ls.values = {}

	def in_prolongation(self, i):
		return self.projections["DoC"][i] >= self.DoR
			
	def project_DoC(self):
		proj = [0 for i in range(0, MAXPROJ)]	
		proj[0] = self.DoS
		proj[1] = self.DoC
		i = 2
		while i < MAXPROJ:
			d = dt.datetime(proj[i-1].year+1, proj[i-1].month, 1)
			date = min(self.date_RA_DEF, d, self.DoR) 
			proj[i] = max(self.DoC, date)
			if proj[i] >= self.date_RA_DEF or proj[i] >= self.DoR:
				break
			i += 1

# prolongate
		i += 1
		while i < MAXPROJ:
			if proj[i-1].month >= self.DoC.month:
				extra_year = 1
			else:
				extra_year = 0
			d = dt.datetime(proj[i-1].year + extra_year,
				self.DoC.month, 1)
			proj[i] = min(self.date_RA_DEF, d)
			if proj[i] >= self.date_RA_DEF:
				break
			i += 1

		assert(MAXPROJ != i)
		return proj
	
	def project_age(self):
		proj = [0 for i in range(0, MAXPROJ)]	
		i = 0
		date = self.projections["DoC"][i]
		while date:
			proj[i] = self.age_at(date)
			i += 1
			date = self.projections["DoC"][i]

		return proj
	
	def project_sal(self):
		proj = [0 for i in range(0, MAXPROJ)]	
		proj[0] = self.sal
		proj[1] = self.sal * (1 + self.ss())
		i = 2
		date = self.projections["DoC"][i]
		while date:
			year1 = date.year
			year2 = self.projections["DoC"][i-1].year
			proj[i] = proj[i-1] * (1 + self.ss())**(year1 - year2)
			i += 1
			date = self.projections["DoC"][i]

		return proj
		
	def project_ceil1(self):
		proj = [0 for i in range(0, MAXPROJ)]	
		if 7229 == self.reg:
			proj[0] = 53823.35
			proj[1] = proj[0] * (1 + self.infl)
		else:
			proj[0] = 52972.54
			proj[1] = 53528.57
		i = 2
		date = self.projections["DoC"][i]
		while date:
			year1 = date.year
			year2 = self.projections["DoC"][i-1].year
			proj[i] = proj[i-1] * (1 + self.infl)**(year1 - year2)
			i += 1
			date = self.projections["DoC"][i]

		return proj

	def project_ceil2(self):
		proj = [0 for i in range(0, MAXPROJ)]	
		proj[0] = 150600
		i = 1
		date = self.projections["DoC"][i]
		while date:
			year1 = date.year
			year2 = self.projections["DoC"][i-1].year
			proj[i] = proj[i-1] * (1 + self.infl)**(year1 - year2)
			i += 1
			date = self.projections["DoC"][i]

		return proj

	def project_service(self, date_s):
		proj = [0 for i in range(0, MAXPROJ)]	
		i = 0
		date = self.projections["DoC"][i]
		if 1 == date_s.day:
			extra_month = 0
		else:
			extra_month = -1
		while date:
			proj[i] = af.calc_years(date, date_s, extra_month)
			i += 1
			date = self.projections["DoC"][i]

		return proj
	
	def project_contr(self, AorC):
		proj = np.zeros(MAXPROJ)
		i = 0
		while self.projections["DoC"][i]:
			if "A" == AorC:
				proj[i] = self.contrA(i)
			else:
				assert("C" == AorC)
				proj[i] = self.contrC(i)
			i += 1

		return proj
	
	def project_contr_gen(self, AorC, gen):
		proj = np.zeros(MAXPROJ)
		if "A" == AorC:
			minimum = self.generationsA["PREMIUM"][gen]
		else:
			assert("C" == AorC)
			minimum = self.generationsC["PREMIUM"][gen]

		min_array = np.zeros(MAXPROJ) + minimum

		for j in range(0, gen):
			proj -= -self.projections["CONTR " + AorC + " "
				+ str(j + 1)]
		proj += self.projections["contr" + AorC]
		proj = np.minimum(proj, min_array)

		return proj

	def project_capdth_gen(self, AorC, gen):
		proj = [0 for i in range(0, MAXPROJ)]	
		if "UKMT" != self.combination:
			return proj
		if "A" == AorC:
			proj[0] = self.generationsA["CAPDTH"][gen]
		else:
			assert("C" == AorC)
			proj[0] = self.generationsC["CAPDTH"][gen]
		i = 1
		while self.projections["DoC"][i]:
			prem = self.projections["CONTR " + AorC + " "
				+ str(gen + 1)][i-1] * (1 - self.admin_cost)
			age1 = self.projections["age"][i-1]
			age2 = self.projections["age"][i]
			proj[i] = proj[i-1] + prem * (age2 - age1)
			i += 1

		return proj
		
	def project_res_gen(self, res_type, AorC, gen):
		proj = [0 for i in range(0, MAXPROJ)]	
		if "A" == AorC:
			proj[0] = self.generationsA[res_type][gen]
			interest = self.generationsA["TAUX"][gen]
		else:
			assert("C" == AorC)
			proj[0] = self.generationsC[res_type][gen]
			interest = self.generationsC["TAUX"][gen]
		i = 1
		while self.projections["DoC"][i]:
			if "RES" == res_type:
				prem = self.projections["CONTR " + AorC + " "
					+ str(gen + 1)][i-1]
				capdth = self.projections["CAPDTH " + AorC
					+ " " + str(gen + 1)][i-1]
			else:
				assert("RESPS" == res_type)
				prem = 0
				capdth = 0
			age1 = self.projections["age"][i-1]
			age2 = self.projections["age"][i]
			t = self.table_ins()
			proj[i] = self.res(self.ls(proj[i-1], prem, age1, t,
				interest, capdth), prem, age2, t, interest,
				capdth)
			i += 1

		return proj

	def set_RA_DEF(self):
		year = self.DoB.year
		month = self.DoB.month
		if 12 == month:
			month = 1
			year += 1
		else:
			month += 1
		self.date_RA_DEF = dt.datetime(year + self.RA_DEF, month, 1)
	
	def set_projections(self):
		self.projections["DoC"] = self.project_DoC()
		self.projections["age"] = self.project_age()
		self.projections["sal"] = self.project_sal()
		self.projections["ceil1"] = self.project_ceil1()
		self.projections["ceil2"] = self.project_ceil2()
		self.projections["SDoA"] = self.project_service(self.DoA)
		self.projections["SDoE"] = self.project_service(self.DoE)
		self.projections["contrA"] = self.project_contr("A")
		self.projections["contrC"] = self.project_contr("C")
		res = ("RES", "RESPS")
		contract = ("A", "C")
		for i in range(0, MAXGEN):
			gen = str(i + 1)
			for c in contract:
				self.projections["CONTR " + c + " " + gen] \
					= self.project_contr_gen(c, i)
				self.projections["CAPDTH " + c + " " + gen] \
					= self.project_capdth_gen(c, i)
				for r in res:
					self.projections[r + " " + c + " " + gen] \
						= self.project_res_gen(r, c, i)

	def __init__(self, df, i):
		print("initing member " + str(i))
		self.data = df.loc[i]
		self.reg = self.data["NO REGLEMENT"]
		self.status = self.data["STATUS"]
		self.DoB = self.data["DOB"]
		self.DoE = self.data["DOE"]
		self.DoS = self.data["DOS"]
		self.DoA = self.data["DOA"]
		self.DoR = self.data["DOR"]
		self.active_contract = self.data["ACTIVE CONTRACT"]
		self.sex = self.data["SEX"]
		self.sal = self.data["SAL"]
		self.pt = self.data["PT"]
		self.NRA = self.data["NRA"]
		self.combination = self.data["TARIEF"]
		self.X10 = self.data["X/10"]
		self.admin_cost = 0.05 # TODO: update to reference the data
		self.delta_cap = 0 # TODO: update to reference the data

		self.set_RA_DEF()

		self.generationsA = {}
		self.generationsC = {}
		for t in titles:
			self.generationsA[t] = self.get_generations(t, "A")
			self.generationsC[t] = self.get_generations(t, "C")

		self.projections = {}
		self.set_projections()

class Data:
	def __init__(self, file_name, sheet_name):
		self.file_name = file_name
		self.sheet_name = sheet_name
		print("opening: " + file_name)
		print("sheet: " + sheet_name)
		self.df = pd.read_excel(io=file_name, sheet_name=sheet_name,
			skiprows = 10)
		self.df.fillna(0, inplace = True)
		print("opening complete.")
		assert("KEY" in self.df.keys())
		self.num_members = len(self.df["KEY"])
		
		
#########################
# testing
#########################

def test_member(df):
	member = Member(df, 0)
	date = dt.datetime(2016, 12, 1)
	age = member.age_at(date)
	assert(member.DoB == dt.datetime(1965, 12, 4))
	assert(51 - 1/12 == age)
	assert(65 == member.projections["age"][16])
	assert(16 + 1/12 == member.projections["SDoA"][16])
	assert(32 + 9/12 == member.projections["SDoE"][16])
	assert(abs(1577.758578020553 - member.projections["contrA"][16])
		< af.EPS)
	assert(abs(788.8792890102764 - member.projections["contrC"][16])
		< af.EPS)

if __name__ == "__main__":
	path = "../test/"
	file_name = "2019-03-06 Standard DC UKMS-UKZT.xlsm"
	sheet = "Data"

	data = Data(path + file_name, sheet)
	test_member(data.df)

	members = [Member(data.df, i) for i in range(0, data.num_members)]

import pandas as pd
import datetime as dt

MAXGEN = 8

class Member:
	
	def age_at(self, date):
		return date.year - self.DoB.year \
		+ (date.month - self.DoB.month - 1)/12
	
	def get_generations(self, name, AorC):
		title = name + "_" + AorC + "_" + "GEN"
		return [self.data[title + str(i)]
			for i in range(1, MAXGEN + 1)] 
			

	def __init__(self, df, i):
		self.data = df.loc[i]
		self.DoB = self.data["DOB"]
		self.combination = self.data["TARIEF"]

		self.lump_sums = self.get_generations("CAP", "A")
		self.lump_sums_ps = self.get_generations("CAPPS", "A")
		self.red_caps = self.get_generations("CAPRED", "A")
		self.rates = self.get_generations("TAUX", "A")
		self.premiums = self.get_generations("PREMIUM", "A")
		self.reserves = self.get_generations("RES", "A")
		self.reserves_ps = self.get_generations("RESPS", "A")

		self.lump_sums = self.get_generations("CAP", "C")
		self.lump_sums_ps = self.get_generations("CAPPS", "C")
		self.red_caps = self.get_generations("CAPRED", "C")
		self.rates = self.get_generations("TAUX", "C")
		self.premiums = self.get_generations("PREMIUM", "C")
		self.reserves = self.get_generations("RES", "C")
		self.reserves_ps = self.get_generations("RESPS", "C")
	
		
def test_age(member):
	date = dt.datetime(2016, 12, 1)
	age = member.age_at(date)
	assert(51 - 1/12 == age)

if __name__ == "__main__":
	path = "../test/"
	file_name = "2019-03-06 Standard DC UKMS-UKZT.xlsm"
	sheet = "Data"

	df = pd.read_excel(io = path + file_name, sheet_name = sheet,
		skiprows = 10)

	member = Member(df, 0)
	test_age(member)

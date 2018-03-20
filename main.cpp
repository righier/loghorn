
#include "horn.hpp"

#include "utils.cpp"
#include "generate.cpp"

std::mutex stdout_mutex;
bool print_messages = false;

void print(InputClauses &phi) {
	printf("---- Rules ----\n");
	for (size_t i = 0; i < phi.rules.size(); i++) {
		printFormula(phi, Formula::create(CLAUSE, i), true);
		printf("\n");
	}

	printf("---- Facts ----\n");
	for (auto fact : phi.facts) {
		printFormula(phi, fact, false);
		printf("\n");
	}
	printf("---------------\n\n");
}


int main(int argc, char **argv) {
	srand((unsigned int)time(NULL));

	
}

int amain(int argc, char **argv) {
	srand((unsigned int)time(NULL));

	auto filename = "test.horn";
	auto caseType = DISCRETE;

	InputClauses phi;
	print_messages = true;

	if (argc > 1) {
		filename = argv[1];
		phi = parseFile(filename);
	} else {
		phi = randomInput(10, 2, 3, 0.5);
	}

	if (argc > 2) {
		if (strcmp(argv[2], "FINITE") == 0) caseType = FINITE;
		else if (strcmp(argv[2], "NATURAL") == 0) caseType = NATURAL;
		else if (strcmp(argv[2], "DISCRETE") == 0) caseType = DISCRETE;
		else if (strcmp(argv[2], "ALL") == 0) caseType = ALL;
		else {
			printf("The case \"%s\" is not valid.\n", argv[2]);
			return 1;
		}
	}

	std::vector<Case> caseTypes;
	if (caseType == ALL)
		caseTypes = { FINITE, NATURAL, DISCRETE };
	else
		caseTypes = { caseType };

	print(phi);

	std::vector<std::thread> threads;
	for (auto caseType : caseTypes) {
		stdout_mutex.lock();
		printf("Starting check of the %s case.\n", caseStrings[caseType]);
		stdout_mutex.unlock();
		std::thread th(check, std::ref(phi), caseType);
		threads.push_back(std::move(th));
	}

	for (auto &th : threads) {
		th.join();
	};

	printf("Done.\n");
	return 0;
}

int bmain(int argc, char **argv) {
	using namespace std::chrono;
	srand((unsigned int)time(NULL));

	int min_clauses = 2;
	int max_clauses = 100;

	int min_letters = 2;
	int max_letters = 20;

	int clause_len = 3;
	auto case_type = DISCRETE;

	float falsehood_rate = 1.0;

	int num_tests = 10;

	printf("%s\t%s\t%s\t%s\n", "clauses", "letters", "seconds", "size");
	for (int num_clauses = min_clauses; num_clauses < max_clauses; num_clauses++) {
		for (int num_letters = min_letters; num_letters < max_letters; num_letters++) {

			float tsize = 0;
			double ttime = 0;
			int ntests = 0;

			std::vector<InputClauses> inputs;
			for (int i = 0; i < num_tests; i++) {
				InputClauses phi = randomInput(num_clauses, num_letters, clause_len, falsehood_rate);

				auto t1 = high_resolution_clock::now();

				int size = check(phi, case_type);

				auto t2 = high_resolution_clock::now();

				tsize += size;
				ttime += (duration_cast<duration<double>>(t2 - t1)).count();
				ntests++;
			}

			tsize /= ntests;
			ttime /= ntests;

			printf("%d\t%d\t%.10f\t%.3f\n", num_clauses, num_letters, ttime, tsize);

		}
	}
	
	return 0;
}

int check(InputClauses &phi, Case caseType) {
	int min, max;
	switch (caseType) {
		case FINITE:	min = 2; break;
		case NATURAL:	min = 3; break;
		case DISCRETE:	min = 4; break;
		default:		return false;
	}
	max = min + 6 * phi.rules.size(); 

	State state = {caseType, phi};
	FormulaSet literals(phi.facts.begin(), phi.facts.end());
	for (auto& clause : phi.rules)
		std::copy(clause.begin(), clause.end(), std::inserter(literals, literals.end()));
	for (auto l : literals) {
		if (l.type == BOXA)
			state.boxa.push_back(l);
		else if (l.type == BOXA_BAR)
			state.boxaBar.push_back(l);
	}

	int xmin = (caseType == DISCRETE);

	for (int k = min; k <= max; k++) {
		int ymax = k - (caseType != FINITE);

		if (print_messages) {
			stdout_mutex.lock();
			printf("%s - checking with size: %d\n", caseStrings[caseType], k);
			stdout_mutex.unlock();
		}

		for (int x = xmin; x < ymax - 1; x++)
			for (int y = x + 1; y < ymax; y++)
				if (saturate(k, x, y, state))
					return k;
	}

	if (print_messages)
		printf("The formula is NOT SATISFIABLE in the %s case.\n", caseStrings[caseType]);
	return max;
}

bool saturate(int d, int x, int y, const State& state) {
	IntervalVector<FormulaVector> hi(d);
	IntervalVector<FormulaSet> lo(d);

	for (int z = 0; z < d - 1; z++) {
		for (int t = z + 1; t < d; t++) {

			lo.get(z, t).insert(Formula::truth());

			auto& hizt = hi.get(z, t);
			for (auto i = 0U; i < state.phi.rules.size(); i++) {
				hizt.push_back(Formula::create(CLAUSE, i));
			}

		}
	}

	auto& hixy = hi.get(x, y);
	for (auto f : state.phi.facts) {
		hixy.push_back(f);
	}

	bool changed = true;
	while (changed) {
		changed = false;

		for (int z = 0; z < d -1; z++) {
			for (int t = z + 1; t < d; t++) {
				auto& hizt = hi.get(z, t);

				for (int ii = hizt.size()-1; ii >= 0; ii--) {
					auto f = hizt[ii];

					if (f.type == LETTER && f.id == TRUTH) {
						eraseFast(hizt, ii);

					} else if (f.type == LETTER && f.id == FALSEHOOD) {
						lo.get(z, t).insert(f);
						return false;

					} else if (f.type == LETTER) {
						eraseFast(hizt, ii);
						if (lo.get(z, t).insert(f).second) changed = true;

					} else if (f.type == BOXA) {
						eraseFast(hizt, ii);
						if (lo.get(z, t).insert(f).second) changed = true;
						for (int r = t + 1; r < d; r++) {
							if (lo.get(t, r).insert(Formula::create(LETTER, f.id)).second) changed = true;
							if (f.id == FALSEHOOD) return false;
						}

					} else if (f.type == BOXA_BAR) {
						eraseFast(hizt, ii);
						if (lo.get(z, t).insert(f).second) changed = true;
						for (int r = 0; r < z; r++) {
							if (lo.get(r, z).insert(Formula::create(LETTER, f.id)).second) changed = true;
							if (f.id == FALSEHOOD) return false;
						}

					} else if (f.type == CLAUSE) {
						Clause& clause = state.phi.rules[f.id];
						Formula last = clause.back();
						bool found = true;
						auto& lozt = lo.get(z, t);
						for (auto it = clause.begin(); it != clause.end()-1; it++) {
							auto l = *it;
							if (lozt.find(l) == lozt.end()) {
								found = false;
								break;
							}
						}

						if (found) {
							eraseFast(hizt, ii);
							lozt.insert(f);
							hizt.push_back(last); 
							changed = true;
						}
					}

				}

			}
		}

		int res = extend(d, hi, lo, state);
		changed = changed || (res == 1);

		if (res == 2) {
			return false;
		}
	}

	if (print_messages) {
		stdout_mutex.lock();
		printf("The formula is SATISFIABLE in the %s case, with size %d and starting interval [%d, %d]:\n", 
				caseStrings[state.caseType], d, x, y );
		printState(state.phi, lo, d);
		stdout_mutex.unlock();
	}
	return true;
}

int extend(int d, IntervalVector<FormulaVector>& hi, IntervalVector<FormulaSet>& lo, const State& state) {
	int changed = false;
	int min, max;

	//printState(state.phi, lo, d);
	//printState(state.phi, hi, d);

	if (state.caseType == FINITE) {
		min = 0;
		max = d;
	} else {
		min = 0;
		max = d - 2;

		for (int z = min; z < max; z++) {
			auto& hizm1 = hi.get(z, max+1);
			for (auto f : hi.get(z, max)) {
				if (f.type != CLAUSE) {
					hizm1.push_back(f);
					changed = 1;
				}
			}
			auto& lozm1 = lo.get(z, max+1);
			for (auto f : lo.get(z, max)) {
				if (f.type != CLAUSE) {
					if (lozm1.insert(f).second) changed = 1;
				}
			}
		}

		std::vector<Formula> temp;
		auto& last = lo.get(max, max+1);
		for (auto f : last) {
			if (f.type == LETTER) {
				temp.push_back(Formula::create(BOXA, f.id));
			} 
			else if (f.type == BOXA) {
				if (f.id == FALSEHOOD) return 2;
				temp.push_back(Formula::create(LETTER, f.id));
			} 
			else if (f.type == BOXA_BAR) {
				if (f.id == FALSEHOOD) return 2;
				temp.push_back(Formula::create(LETTER, f.id));
			}
		}
		for (auto f: temp) {
			if (last.insert(f).second) changed = 1;
		}
		temp.clear();

		if (state.caseType == DISCRETE) {
			min = 1;
			max = d - 1;

			for (int z = min + 1; z <= max; z++) {
				auto& hi0z = hi.get(0, z);
				for (auto f : hi.get(1, z)) {
					if (f.type != CLAUSE) {
						hi0z.push_back(f);
						changed = 1;
					}
				}
				auto& lo0z = lo.get(0, z);
				for (auto f : lo.get(1, z)) {
					if (f.type != CLAUSE) {
						if (lo0z.insert(f).second) changed = 1;
					}
				}
			}

			auto& first = lo.get(0, 1);
			for (auto f : first) {
				if (f.type == LETTER) {
					temp.push_back(Formula::create(BOXA_BAR, f.id));
				}
				else if (f.type == BOXA) {
					if (f.id == FALSEHOOD) return 2;
					temp.push_back(Formula::create(LETTER, f.id));
				}
				else if (f.type == BOXA_BAR) {
					if (f.id == FALSEHOOD) return 2;
					temp.push_back(Formula::create(LETTER, f.id));
				}
			}
			for (auto f: temp) {
				if (first.insert(f).second) changed = 1;
			}
		}
	}

	for (int z = min; z < max; z++) {

		for (auto f : state.boxa) {
			bool found = 1;
			for (int t = z + 1; t < d; t++) {
				auto p = Formula::create(LETTER, f.id);
				if (lo.get(z, t).count(p) == 0) {
					found = false;
					break;
				}
			}
			if (found) {
				for (int r = 0; r < z; r++)
					if (lo.get(r, z).insert(f).second) changed = 1;
			}
		}

		for (auto f : state.boxaBar) {
			bool found = 1;
			for (int r = 0; r < z; r++) {
				auto p = Formula::create(LETTER, f.id);
				if (lo.get(r, z).count(p) == 0) {
					found = false;
					break;
				}
			}
			if (found) {
				for (int t = z + 1; t < d; t++)
					if (lo.get(z, t).insert(f).second) changed = 1;
			}
		}

	}

	return changed;
}

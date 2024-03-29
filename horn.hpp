#pragma once

#include <random>
#include <iostream>
#include <fstream>
#include <streambuf>
#include <string>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <iterator>
#include <algorithm>
#include <thread>
#include <mutex>
#include <chrono>

enum FormulaType {
	LETTER,
	BOXA,
	BOXA_BAR,
	CLAUSE,
	INVALID_FORMULA,
};

enum Case {
	FINITE,
	NATURAL,
	DISCRETE,
	ALL_CASES,
	INVALID_CASE,
};

const char *caseStrings[] = {
	"FINITE",
	"NATURAL",
	"DISCRETE",
	"ALL_CASES",
	"INVALID",
};


#define FALSEHOOD 0
#define TRUTH 1

struct Formula {
	FormulaType type;
	int id;

	static Formula create(FormulaType type, int id) { return { type, id }; }
	static Formula truth() { return { LETTER, TRUTH }; }
	static Formula falsehood() { return { LETTER, FALSEHOOD }; }
};

typedef std::pair<int, int> Interval;
typedef std::vector<Formula> Clause;

struct InputClauses {
	std::vector<Clause> rules;
	std::vector<Formula> facts;
	std::vector<std::string> labels;
};

struct State {
	Case caseType;
	InputClauses& phi;
	std::vector<Formula> boxa;
	std::vector<Formula> boxaBar;
};

inline bool operator==(const Formula& lhs, const Formula& rhs) {
	return lhs.type == rhs.type && lhs.id == rhs.id;
}
bool operator!=(const Formula& lhs, const Formula& rhs) {
	return !operator==(lhs, rhs);
}
std::size_t i2hash(int a, int b) {
	return std::hash<int>()(a) ^ std::hash<int>()(b);
}
struct IntervalHash {
	std::size_t operator()(const Interval &i) const { return i2hash(i.first, i.second); }
};
struct FormulaHash {
	std::size_t operator()(const Formula &f) const { return i2hash(f.id, f.type); }
};

typedef std::unordered_set<Formula, FormulaHash> FormulaSet;
typedef std::vector<Formula> FormulaVector;
// since the array isn't ordered we can swap elements and delete in constant time
inline void eraseFast(FormulaVector& v, int i) {
	int last = v.size() - 1;
	if (i > last) return;
	v[i] = v[last];
	v.pop_back();
}

template<typename T> struct IntervalVector {
	private:
		size_t n;
		std::vector<T> v;
		int getIndex(int x, int y) {
			x = (n - x) - 2;
			y = (n - y) - 1;
			return (x * (x + 1) / 2) + y;
		}

	public:
		IntervalVector() : n(0), v() {}
		IntervalVector(size_t size) : n(size), v(size * (size + 1) /2) {}
		T& get(int x, int y) {
			return v[getIndex(x, y)];
		}
		size_t size() {
			return n;
		}
};

struct Model {
	Model(IntervalVector<FormulaSet> lo, bool satisfied, Interval start)
		: lo(lo), satisfied(satisfied), start(start) {}
	static Model unsatisfied() { return Model({}, false, {}); }
	IntervalVector<FormulaSet> lo;
	bool satisfied = false;
	Interval start;
};

/* Satisfiability Checker */
Model check(InputClauses& phi, Case caseType);
Model saturate(int d, int x, int y, const State& phi);
int extend(int d, IntervalVector<FormulaVector>& hi, IntervalVector<FormulaSet>& lo, const State& phi);

/* Print Utilities */
void printFormula(const InputClauses& phi, const Formula f, bool universal);
void printInterval(const InputClauses& phi, const Interval& interval, const FormulaSet& formulas);
void printInterval(const InputClauses& phi, const Interval& interval, const FormulaVector& formulas);
void printState(const InputClauses& phi, IntervalVector<FormulaSet> &intervals, int d);
void printState(const InputClauses& phi, IntervalVector<FormulaVector> &intervals, int d);

void printFormula(FILE *stream, const InputClauses& phi, const Formula f, bool universal);
void printInterval(FILE *stream, const InputClauses& phi, const Interval& interval, const FormulaSet& formulas);
void printInterval(FILE *stream, const InputClauses& phi, const Interval& interval, const FormulaVector& formulas);
void printState(FILE *stream, const InputClauses& phi, IntervalVector<FormulaSet> &intervals, int d);
void printState(FILE *stream, const InputClauses& phi, IntervalVector<FormulaVector> &intervals, int d);

/* Parser\\Generator Utilities */
std::string numToLabel(int n);
InputClauses parseFile(const char* path);
InputClauses randomInput(int n_clauses, int letters, int clause_len, int max_falsehood);


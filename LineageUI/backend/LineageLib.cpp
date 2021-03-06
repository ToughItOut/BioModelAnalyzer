// Copyright (c) Microsoft Research 2016
// License: MIT. See LICENSE
#include <memory>
#include <algorithm>
#include <numeric>
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include "LineageLib.h"
#include "Simulation.h"

using std::vector;
using std::map;
using std::string;
using std::stringstream;
using std::unique_ptr;
using std::pair;

const string INITIALPROG{ "P0" };

namespace linux_problem {
	template <class T>
	std::string to_string(const T& obj) {
		std::stringstream s;
		s << obj;
		return s.str();
	}
};

vector<string> programs(vector<string> pgm) {
	try {
		unique_ptr<Simulation> s{ new Simulation(pgm) };
		return s->programs();
	}
	catch (const string& err) {
		vector<string> errv;
		string ferr = "Error: " + err;
		errv.push_back(ferr);
		return errv;
	}
}

vector<string> conditions(vector<string> pgm, string program) {
    vector<string> ret;
	try {
		unique_ptr<Simulation> s{ new Simulation(pgm) };
		CellProgram* prog = s->program(program);
		if (nullptr == prog) {
			ret.push_back("Error: Could not compile the program.");
			return ret;
		}
		for (CellProgram::iterator it = prog->begin(); it != prog->end(); ++it) {
			stringstream val;
			val << *it;
			ret.push_back(val.str());
		}
	}
	catch (const string& err) {
		vector<string> errv;
		string ferr = "Error: " + err;
		errv.push_back(ferr);
		return errv;
	}
	return ret;
}

vector<string> simulate(vector<string> pgm, string condition) {
	try {
		unique_ptr<Simulation> s{ new Simulation(pgm) };
		s->run(INITIALPROG, condition, -1.0, -1.0);
		return s->toVectorString();
	}
	catch (const string& err) {
		vector<string> errv;
		string ferr = "Error: " + err;
		errv.push_back(ferr);
		return errv;
	}
}

vector<string> checkTimeOverlap(vector<string> pgm, string condition, string firstCell, string secondCell, unsigned int numSimulations, bool rawData) {
	try {
		unique_ptr<Simulation> s{ new Simulation(pgm) };
		vector<float> results1;
		vector<float> results2;
		for (unsigned int i{ 0 }; i < numSimulations; ++i) {
			s->clear();
			s->run(INITIALPROG, condition, -1.0, -1.0);
			pair <float, bool> p(s->overlap(firstCell, secondCell));
			if (p.second)
				results1.push_back(p.first);
			else
				results2.push_back(p.first);
		}

		auto statistics = [&](vector<float>& vec) -> vector<float> {
			std::sort(vec.begin(), vec.end());
			float min(vec[0]);
			float max(vec[vec.size() - 1]);
			const float ZERO{ 0.0 };
			float mean(std::accumulate(vec.begin(), vec.end(), ZERO) / vec.size());
			float accum = 0.0;
			std::for_each(vec.begin(), vec.end(), [&](const float f) {
				accum += (f - mean) * (f - mean);
			});
			float stdev = std::sqrt(accum / vec.size());
			float q1(vec.size() % 4 ? vec[vec.size() / 4] : (vec[vec.size() / 4 - 1] + vec[vec.size() / 4]) / 2);
			float median(vec.size() % 2 ? vec[vec.size() / 2] : (vec[vec.size() / 2 - 1] + vec[vec.size() / 2]) / 2);
			float q3(vec.size() % 4 ? vec[3 * vec.size() / 4] : (vec[3 * vec.size() / 4 - 1] + vec[3 * vec.size() / 4]) / 2);
			return vector<float> {min, max, mean, stdev, q1, median, q3};
		};

		vector<string> names{ "q1", "min", "median", "max", "q3", "mean+std", "mean-std", "mean", "stdev" };

		vector<string> result;

		auto addEntryToVec = [&](const string& name, const float& val) {
			string toAdd{ name };
			toAdd += ": ";
			toAdd += linux_problem::to_string(val);
			result.push_back(toAdd);
		};

		auto addToVec = [&](vector<float>& vec) {
			if (rawData) {
				string raw{};
				bool first = true;
				for (float val : vec) {
					if (!first)
						raw += ", ";
					raw += linux_problem::to_string(val);
					first = false;
				}
				result.push_back(raw);
			}

			vector<float> stat{ statistics(vec) };
			vector<float> vals{ stat[4], stat[0], stat[5], stat[1], stat[6], stat[2] + stat[3], stat[2] - stat[3], stat[2], stat[3] }; 

			for (unsigned int i = 0; i < names.size(); ++i) {
				addEntryToVec(names[i], vals[i]);
			}
		};

		if (results1.size()) {
			string temp{ firstCell };
			temp += " born before ";
			temp += secondCell;
			temp += ":";
			result.push_back(temp);
			addToVec(results1);
		}

		if (results2.size()) {
			string temp{ secondCell };
			temp += " born before ";
			temp += firstCell;
			temp += ":";
			result.push_back(temp);
			addToVec(results2);
		}

		return result;
	}
	catch (const string& err) {
		vector<string> errv;
		string ferr = "Error: " + err;
		errv.push_back(ferr);
		return errv;
	}
}

vector<string> cellExistence(vector<string> programs, string condition, unsigned int numSimulations) {
	try {
		unique_ptr<Simulation> s{ new Simulation(programs) };
		map<string, unsigned int> total{};
		for (unsigned int i{ 0 }; i < numSimulations; ++i) {
			s->clear();
			s->run(INITIALPROG, condition, -1.0, -1.0);
			map<string, unsigned int> res{ s->cellCount() }; 
			for (auto nameCount : res) {
				if (total.find(nameCount.first) == total.end()) {
					total.insert(nameCount);
				}
				else {
					total[nameCount.first] += nameCount.second;
				}
			}
		}
		vector<string> results;
		for (auto nameCount : total) {
			string temp{ nameCount.first };
			temp += ": ";
			temp += linux_problem::to_string(nameCount.second);
			results.push_back(temp); 
		}
		return results;
	}
	catch (const string& err) {
		vector<string> errv;
		string ferr = "Error: " + err;
		errv.push_back(ferr);
		return errv;
	}
}

vector<string> simulateAbnormal(vector<string> programs, string condition, unsigned int repetitions) {
	try {
		unique_ptr<Simulation> s{ new Simulation(programs) };
		vector<string> progs{ s->programs() };

		for (unsigned int i{ 0 }; i<repetitions; ++i) {
			s->clear();
			s->run(INITIALPROG, condition, -1.0, -1.0);
			map<string, unsigned int> res{ s->cellCount() };
			for (auto name : progs) {
				if (res.find(name) == res.end()) {
					vector<string> results{ s->toVectorString() };
					string temp{ name };
					temp += " was not created.";
					results.push_back(temp); 
					return results;
				}
				if (res.find(name)->second > 1) {
					vector<string> results{ s->toVectorString() };
					string temp{ name };
					temp += " was created more than once.";
					results.push_back(temp);
					return results;
				}
			}
		}

		string temp{ "Could not find an abnormal simulation." }; 
		vector<string> results;
		results.push_back(temp);
		return results;
	}
	catch (const string& err) {
		vector<string> errv;
		string ferr = "Error: " + err;
		errv.push_back(ferr);
		return errv;
	}
}

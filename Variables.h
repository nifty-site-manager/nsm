#ifndef VARIABLES_H_
#define VARIABLES_H_

#include <atomic>
#include <map>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <fstream>
#include <set>
#include <unordered_set>
#include <vector>

#include "NumFns.h"
#include "Path.h"
#include "StrFns.h"

struct VLayer
{
	std::string scope;
	std::unordered_set<std::string> constants, privates;

	std::unordered_map<std::string, std::unordered_set<std::string> > inScopes;
	std::unordered_map<std::string, std::string> scopeOf;
	std::unordered_map<std::string, std::string> typeOf;

	std::unordered_map<std::string, std::string> functions;
	std::unordered_set<std::string> nFns, unscopedFns, noOutput;
	std::map<std::string, Path> paths;
	//std::map<std::string, int> funcDefLineNo;

	std::map<std::string, bool> bools;
	std::unordered_map<std::string, int> ints;
	std::map<std::string, long long int> llints;
	std::map<std::string, double> doubles;
	std::map<std::string, char> chars;
	std::map<std::string, std::string> strings;
	std::map<std::string, std::vector<double> > doubVecs;
	std::map<std::string, std::vector<std::string> > strVecs;
	std::map<std::string, std::fstream> fstreams;
	std::map<std::string, std::ifstream> ifstreams;
	std::map<std::string, std::ofstream> ofstreams;

	VLayer();
};

struct VPos
{
	int layer;
	std::string name, type;

	VPos();
};

struct Variables
{
	std::vector<VLayer> layers;
	std::unordered_set<std::string> basic_types;
	std::map<std::string, std::string> typeDefs;
	std::unordered_set<std::string> nTypes;
	std::map<std::string, Path> typeDefPath;
	std::map<std::string, int> typeDefLineNo;

	int precision;
	bool fixedPrecision, scientificPrecision;

	Variables();

	std::string double_to_string(const double& d, const bool& round);

	int add_layer(const std::string& scope);
	bool find(const std::string& name, VPos& vpos);
	bool find_fn(const std::string& name, VPos& vpos);

	int get_bool_from_var(const VPos& vpos, bool& val);
	int get_double_from_var(const VPos& vpos, double& val);
	int get_str_from_var(const VPos& vpos,
	                     std::string& str,
	                     const bool& round,
	                     const bool& indent);
	int add_str_from_var(const VPos& vpos,
	                     std::string& str,
	                     const bool& round,
	                     const bool& indent,
	                     std::string& indentAmount);

	int set_var_from_str(const VPos& vpos, const std::string& value);
	int set_var_from_double(const VPos& vpos, const double& value);
};

#endif //VARIABLES_H_

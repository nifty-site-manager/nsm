#include "Variables.h"

std::string Variables::double_to_string(const double& d, const bool& round)
{
	std::ostringstream oss;
	if(round)
	{
		if(fixedPrecision)
		{
			if(scientificPrecision)
				oss << std::fixed << std::scientific << std::setprecision(precision) << d;
			else
				oss << std::fixed << std::setprecision(precision) << d;
		}
		else if(scientificPrecision)
			oss << std::scientific << std::setprecision(precision) << d;
		else
			oss << std::setprecision(precision) << d;
	}
	else
		oss << d;

	return oss.str();
}


VLayer::VLayer()
{
}

VPos::VPos()
{
}

Variables::Variables()
{
	add_layer("");

	precision = 6;

	basic_types.insert("bool");
	basic_types.insert("int");
	basic_types.insert("double");
	basic_types.insert("char");
	basic_types.insert("string");
	basic_types.insert("std::bool");
	basic_types.insert("std::int");
	basic_types.insert("std::llint");
	basic_types.insert("std::double");
	basic_types.insert("std::char");
	basic_types.insert("std::string");
	basic_types.insert("std::vector<double>");
	basic_types.insert("std::vector<string>");
	basic_types.insert("fstream");
	basic_types.insert("ifstream");
	basic_types.insert("ofstream");
	basic_types.insert("sstream");
}

int Variables::add_layer(const std::string& scope)
{
	layers.push_back(VLayer());

	layers[layers.size()-1].scope = scope;


	return 0;
}

bool Variables::find(const std::string& name, VPos& vpos)
{
	for(int l=layers.size()-1; l>=0; --l)
	{
		if(layers[l].typeOf.count(name))
		{
			vpos.name = name;
			vpos.layer = l;
			vpos.type = layers[l].typeOf[name];

			return 1;
		}
	}

	return 0;
}

bool Variables::find_fn(const std::string& name, VPos& vpos)
{
	for(int l=layers.size()-1; l>=0; --l)
	{
		if(layers[l].functions.count(name))
		{
			vpos.name = name;
			vpos.layer = l;
			vpos.type = layers[l].typeOf[name];

			return 1;
		}
	}

	return 0;
}

int Variables::get_bool_from_var(const VPos& vpos, bool& val)
{
	val = 0;
	if(vpos.type.substr(0, 5) == "std::")
	{
		if(vpos.type == "std::bool")
		{
			val = layers[vpos.layer].bools[vpos.name];
			return 1;
		}
		else if(vpos.type == "std::int")
		{
			val = layers[vpos.layer].ints[vpos.name];
			return 1;
		}
		else if(vpos.type == "std::double")
		{
			val = layers[vpos.layer].doubles[vpos.name];
			return 1;
		}
		else if(vpos.type == "std::llint")
		{
			val = layers[vpos.layer].llints[vpos.name];
			return 1;
		}
	}
	else if(vpos.type == "bool" ||
	        vpos.type == "int" ||
	        vpos.type == "double")
	{
		val = layers[vpos.layer].doubles[vpos.name];
		return 1;
	}

	return 0;
}

int Variables::get_double_from_var(const VPos& vpos, double& val)
{
	val = 0;
	if(vpos.type.substr(0, 5) == "std::")
	{
		if(vpos.type == "std::bool")
		{
			val = layers[vpos.layer].bools[vpos.name];
			return 1;
		}
		else if(vpos.type == "std::int")
		{
			val = layers[vpos.layer].ints[vpos.name];
			return 1;
		}
		else if(vpos.type == "std::double")
		{
			val = layers[vpos.layer].doubles[vpos.name];
			return 1;
		}
		else if(vpos.type == "std::llint")
		{
			val = layers[vpos.layer].llints[vpos.name];
			return 1;
		}
	}
	else if(vpos.type == "bool" ||
	        vpos.type == "int" ||
	        vpos.type == "double")
	{
		val = layers[vpos.layer].doubles[vpos.name];
		return 1;
	}

	return 0;
}

int Variables::get_str_from_var(const VPos& vpos, std::string& str, const bool& round, const bool& indent)
{
	std::string indentAmount = "";
	return add_str_from_var(vpos, str, round, indent, indentAmount);
}

int Variables::add_str_from_var(const VPos& vpos, std::string& str, const bool& round, const bool& indent, std::string& indentAmount)
{
	if(vpos.type.substr(0, 5) == "std::")
	{
		if(vpos.type == "std::bool")
		{
			if(layers[vpos.layer].bools[vpos.name])
				str += "1";
			else
				str += "0";
			if(indent)
				indentAmount += ' ';

			return 1;
		}
		else if(vpos.type == "std::int")
		{
			std::string val = std::to_string(layers[vpos.layer].ints[vpos.name]);
			str += val;
			if(indent)
				indentAmount += into_whitespace(val);

			return 1;
		}
		else if(vpos.type == "std::double")
		{
			std::string val = double_to_string(layers[vpos.layer].doubles[vpos.name], round);
			str += val;
			if(indent)
				indentAmount += into_whitespace(val);

			return 1;
		}
		else if(vpos.type == "std::char")
		{
			char val = layers[vpos.layer].chars[vpos.name];
			if(val == '\n')
				str += "\n" + indentAmount;
			else
			{
				str += val;
				if(indent)
				{
					if(val == '\t')
						indentAmount += '\t';
					else
						indentAmount += ' ';
				}
			}

			return 1;
		}
		else if(vpos.type == "std::string")
		{
			std::istringstream iss(layers[vpos.layer].strings[vpos.name]);
			std::string ssLine, oldLine;
			int ssLineNo = 0;

			while(getline(iss, ssLine))
			{
				if(0 < ssLineNo++)
					str += "\n" + indentAmount;
				oldLine = ssLine;
				str += ssLine;
			}
			if(indent)
				indentAmount += into_whitespace(oldLine);

			return 1;
		}
		else if(vpos.type == "std::llint")
		{
			str += std::to_string(layers[vpos.layer].llints[vpos.name]);
			if(indent)
				indentAmount += into_whitespace(std::to_string(layers[vpos.layer].llints[vpos.name]));

			return 1;
		}
	}
	else if(vpos.type == "bool")
	{
		if(layers[vpos.layer].doubles[vpos.name])
			str += "1";
		else
			str += "0";
		if(indent)
			indentAmount += ' ';

		return 1;
	}
	else if(vpos.type == "int")
	{
		std::string val = std::to_string((int)layers[vpos.layer].doubles[vpos.name]);
		str += val;
		if(indent)
			indentAmount += into_whitespace(val);

		return 1;
	}
	else if(vpos.type == "double")
	{
		std::string val = double_to_string(layers[vpos.layer].doubles[vpos.name], round);
		str += val;
		if(indent)
			indentAmount += into_whitespace(val);

		return 1;
	}
	else if(vpos.type == "char")
	{
		std::string valStr = layers[vpos.layer].strings[vpos.name];
		char val;
		if(valStr.size())
			val = valStr[0];
		else
			val = ' ';

		if(val == '\n')
			str += "\n" + indentAmount;
		else
		{
			str += val;
			if(indent)
			{
				if(val == '\t')
					indentAmount += '\t';
				else
					indentAmount += ' ';
			}
		}

		return 1;
	}
	else if(vpos.type == "string")
	{
		std::istringstream iss(layers[vpos.layer].strings[vpos.name]);
		std::string ssLine, oldLine;
		int ssLineNo = 0;

		while(getline(iss, ssLine))
		{
			if(0 < ssLineNo++)
				str += "\n" + indentAmount;
			oldLine = ssLine;
			str += ssLine;
		}
		if(indent)
			indentAmount += into_whitespace(oldLine);

		return 1;
	}
	else if(vpos.type == "function" || vpos.type == "fn")
	{
		std::istringstream iss(layers[vpos.layer].functions[vpos.name]);
		std::string ssLine, oldLine;
		int ssLineNo = 0;

		while(getline(iss, ssLine))
		{
			if(0 < ssLineNo++)
				str += "\n" + indentAmount;
			oldLine = ssLine;
			str += ssLine;
		}
		if(indent)
			indentAmount += into_whitespace(oldLine);

		return 1;
	}

	return 0;
}

int Variables::set_var_from_str(const VPos& vpos, const std::string& value)
{
	if(layers[vpos.layer].constants.count(vpos.name))
	{
		return 0;
	}
	if(layers[vpos.layer].privates.count(vpos.name))
	{
		if(!layers[vpos.layer].inScopes[vpos.name].count(layers[layers.size()-1].scope))
			return 0;
	}

	std::string varType = layers[vpos.layer].typeOf[vpos.name];
	if(varType.substr(0, 5) == "std::")
	{
		if(varType == "std::bool")
		{
			if(isInt(value))
				layers[vpos.layer].bools[vpos.name] = (bool)std::atoi(value.c_str());
			else if(isDouble(value))
				layers[vpos.layer].bools[vpos.name] = (bool)std::strtod(value.c_str(), NULL);
			else
				return 0;
		}
		else if(varType == "std::int")
		{
			if(isInt(value))
				layers[vpos.layer].ints[vpos.name] = std::atoi(value.c_str());
			else if(isDouble(value))
				layers[vpos.layer].ints[vpos.name] = (int)std::strtod(value.c_str(), NULL);
			else
				return 0;
		}
		else if(varType == "std::double")
		{
			if(isDouble(value))
				layers[vpos.layer].doubles[vpos.name] = std::strtod(value.c_str(), NULL);
			else
				return 0;
		}
		else if(varType == "std::char")
		{
			if(value.size() == 1)
				layers[vpos.layer].chars[vpos.name] = value[0];
			else
				return 0;
		}
		else if(varType == "std::string")
			layers[vpos.layer].strings[vpos.name] = value;
		else if(varType == "std::llint")
		{
			if(isInt(value))
				layers[vpos.layer].llints[vpos.name] = std::atoll(value.c_str());
			else if(isDouble(value))
				layers[vpos.layer].ints[vpos.name] = (long long int)std::strtod(value.c_str(), NULL);
			else
				return 0;
		}
	}
	else if(varType == "bool")
	{
		if(isInt(value))
			layers[vpos.layer].doubles[vpos.name] = (bool)std::atoi(value.c_str());
		else if(isDouble(value))
			layers[vpos.layer].doubles[vpos.name] = (bool)std::strtod(value.c_str(), NULL);
		else
			return 0;
	}
	else if(varType == "int")
	{
		if(isInt(value))
			layers[vpos.layer].doubles[vpos.name] = std::atoi(value.c_str());
		else if(isDouble(value))
			layers[vpos.layer].doubles[vpos.name] = (int)std::strtod(value.c_str(), NULL);
		else
			return 0;
	}
	else if(varType == "double")
	{
		if(isDouble(value))
			layers[vpos.layer].doubles[vpos.name] = std::strtod(value.c_str(), NULL);
		else
			return 0;
	}
	else if(varType == "char")
	{
		if(value.size() == 1)
			layers[vpos.layer].strings[vpos.name] = value[0];
		else
			return 0;
	}
	else if(varType == "string")
		layers[vpos.layer].strings[vpos.name] = value;
	else
		return 0;

	return 1;
}

int Variables::set_var_from_double(const VPos& vpos, const double& value)
{
	if(layers[vpos.layer].constants.count(vpos.name))
	{
		return 0;
	}
	if(layers[vpos.layer].privates.count(vpos.name))
	{
		if(!layers[vpos.layer].inScopes[vpos.name].count(layers[layers.size()-1].scope))
			return 0;
	}

	std::string varType = layers[vpos.layer].typeOf[vpos.name];
	if(varType.substr(0, 5) == "std::")
	{
		if(varType == "std::bool")
			layers[vpos.layer].bools[vpos.name] = (bool)value;
		else if(varType == "std::int")
			layers[vpos.layer].ints[vpos.name] = (int)value;
		else if(varType == "std::double")
			layers[vpos.layer].doubles[vpos.name] = value;
		else if(varType == "std::char")
			layers[vpos.layer].chars[vpos.name] = std::to_string(value)[0];
		else if(varType == "std::string")
			layers[vpos.layer].strings[vpos.name] = std::to_string(value);
		else if(varType == "std::llint")
			layers[vpos.layer].llints[vpos.name] = (long long int)value;
	}
	else if(varType == "bool")
		layers[vpos.layer].doubles[vpos.name] = (bool)value;
	else if(varType == "int")
		layers[vpos.layer].doubles[vpos.name] = (int)value;
	else if(varType == "double")
		layers[vpos.layer].doubles[vpos.name] = value;
	else if(varType == "char")
		layers[vpos.layer].strings[vpos.name] = std::to_string(value)[0];
	else if(varType == "string")
		layers[vpos.layer].strings[vpos.name] = std::to_string(value);
	else
		return 0;

	return 1;
}

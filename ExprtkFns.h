#ifndef EXPRTKFNS_H_
#define EXPRTKFNS_H_

#include <cstdio>
#include <iostream>

#include "Consts.h"
#include "Expr.h"
#include "FileSystem.h"
#include "Quoted.h"
#include "Variables.h"

template <typename T>
struct exprtk_cd : public exprtk::igeneric_function<T>
{
	typedef typename exprtk::igeneric_function<T>::parameter_list_t parameter_list_t;

	exprtk_cd() : exprtk::igeneric_function<T>("S")
	{}

	inline T operator()(parameter_list_t parameters)
	{
		typedef typename exprtk::igeneric_function<T>::generic_type generic_type;
		typedef typename generic_type::string_view string_t;

		generic_type& gt = parameters[0];
		string_t param_strt(gt);
		std::string inStr = std::string(param_strt.begin(), param_strt.size());
		std::string target = replace_home_dir(inStr);

		if(!dir_exists(target))
			return 1;

		if(chdir(target.c_str()))
			return 1;

		return 0;
	}
};

template <typename T>
struct exprtk_sys : public exprtk::igeneric_function<T>
{
	typedef typename exprtk::igeneric_function<T>::parameter_list_t parameter_list_t;

	int* nsm_mode;

	exprtk_sys() : exprtk::igeneric_function<T>("S")
	{}

	inline T operator()(parameter_list_t parameters)
	{
		typedef typename exprtk::igeneric_function<T>::generic_type generic_type;
		typedef typename generic_type::string_view string_t;

		generic_type& gt = parameters[0];
		string_t param_strt(gt);
		std::string sys_call = std::string(param_strt.begin(), param_strt.size());

		#if defined _WIN32 || defined _WIN64
			if(unquote(sys_call).substr(0, 2) == "./")
				sys_call = unquote(sys_call).substr(2, unquote(sys_call).size()-2);
		#else  //*nix
			if(file_exists("/.flatpak-info"))
				sys_call = "flatpak-spawn --host bash -c " + quote(sys_call);
		#endif

		int result = system(sys_call.c_str());

		if(result && (*nsm_mode == MODE_INTERP || *nsm_mode == MODE_SHELL))
			std::cout << "\a" << std::flush;

		return result;
	}

	void setModePtr(int* modePtr)
	{
		nsm_mode = modePtr;
	}
};

template <typename T>
struct exprtk_to_string : public exprtk::igeneric_function<T>
{
	typedef typename exprtk::igeneric_function<T>::parameter_list_t parameter_list_t;

	exprtk_to_string() : exprtk::igeneric_function<T>("T",exprtk::igeneric_function<T>::e_rtrn_string)
	{}

	inline T operator()(std::string& result, parameter_list_t parameters)
	{
		typedef typename exprtk::igeneric_function<T>::generic_type generic_type;
		typedef typename generic_type::scalar_view scalar_t;

		generic_type& gt = parameters[0];
		double val = scalar_t(gt).v_;
		if(std::trunc(val) == val)
			result = std::to_string((int)val);
		else
			result = std::to_string(val);

		return T(0);
	}
};

template <typename T>
struct exprtk_nsm_lang : public exprtk::igeneric_function<T>
{
	typedef typename exprtk::igeneric_function<T>::parameter_list_t parameter_list_t;

	std::string* nsm_lang;

	exprtk_nsm_lang() : exprtk::igeneric_function<T>("S")
	{}

	inline T operator()(parameter_list_t parameters)
	{
		typedef typename exprtk::igeneric_function<T>::generic_type generic_type;
		typedef typename generic_type::string_view string_t;

		generic_type& gt = parameters[0];
		string_t param_strt(gt);
		std::string langStr = std::string(param_strt.begin(), param_strt.size());

		int pos = langStr.find_first_of("fFnNtTlLeExX", 0);
		if(pos >= 0)
		{
			if(langStr[pos] == 'f' || langStr[pos] == 'F')
				*nsm_lang = "f++";
			else if(langStr[pos] == 'n' || langStr[pos] == 'N' || langStr[pos] == 't' || langStr[pos] == 'T')
				*nsm_lang = "n++";
			else if(langStr[pos] == 'l' || langStr[pos] == 'L')
				*nsm_lang = "lua";
			else if(langStr[pos] == 'e' || langStr[pos] == 'E' || langStr[pos] == 'x' || langStr[pos] == 'X')
				*nsm_lang = "exprtk";

			return 1;
		}

		return 0;
	}

	void setLangPtr(std::string* langPtr)
	{
		nsm_lang = langPtr;
	}
};

template <typename T>
struct exprtk_nsm_mode : public exprtk::igeneric_function<T>
{
	typedef typename exprtk::igeneric_function<T>::parameter_list_t parameter_list_t;

	int* nsm_mode;

	exprtk_nsm_mode() : exprtk::igeneric_function<T>("S")
	{}

	inline T operator()(parameter_list_t parameters)
	{
		typedef typename exprtk::igeneric_function<T>::generic_type generic_type;
		typedef typename generic_type::string_view string_t;

		generic_type& gt = parameters[0];
		string_t param_strt(gt);
		std::string modeStr = std::string(param_strt.begin(), param_strt.size());

		int pos = modeStr.find_first_of("sSiI", 0);
		if(pos >= 0)
		{
			if(modeStr[pos] == 's' || modeStr[pos] == 'S')
				*nsm_mode = MODE_SHELL;
			else if(modeStr[pos] == 'i' || modeStr[pos] == 'I')
				*nsm_mode = MODE_INTERP;

			return 1;
		}

		return 0;
	}

	void setModePtr(int* modePtr)
	{
		nsm_mode = modePtr;
	}
};

template <typename T>
struct exprtk_nsm_write : public exprtk::igeneric_function<T>
{
	typedef typename exprtk::igeneric_function<T>::parameter_list_t parameter_list_t;

	Variables* vars;
	std::string* parsedText;
	bool indent;
	std::string* indentAmount;
	bool* consoleLocked; 
	std::mutex* os_mtx;

	exprtk_nsm_write()
	{
		indent = 1;
	}

	void add_info(Variables* Vars, 
	              std::string* ParsedText,
	              std::string* IndentAmount,
	              bool* ConsoleLocked,
	              std::mutex* OS_mtx)
	{
		vars = Vars;
		parsedText = ParsedText;
		indentAmount = IndentAmount;
		consoleLocked = ConsoleLocked;
		os_mtx = OS_mtx;
	}

	inline T operator()(parameter_list_t parameters)
	{
		typedef typename exprtk::igeneric_function<T>::generic_type generic_type;
		typedef typename generic_type::scalar_view scalar_t;
		typedef typename generic_type::vector_view vector_t;
		typedef typename generic_type::string_view string_t;

		std::string txt;

		if(parameters.size() < 2)
			return 0;

		for(std::size_t i = 1; i < parameters.size(); ++i)
		{
			generic_type& gt = parameters[i];

			if(generic_type::e_scalar == gt.type)
			{
				double val = scalar_t(gt).v_;
				if(val == NSM_ENDL)
					txt += "\r\n";
				else
					txt += std::to_string(val);
			}
			else if(generic_type::e_vector == gt.type)
			{
				vector_t param_vect(gt);
				//what to do here? repeat above? separate with ", "?
			}
			else if(generic_type::e_string == gt.type)
			{
				string_t param_strt(gt);
				txt += std::string(param_strt.begin(), param_strt.size());
			}
		}

		generic_type& gt = parameters[0];

		if(generic_type::e_scalar == gt.type)
		{
			double param = scalar_t(gt).v_;

			if(param == NSM_OFILE)
			{
				std::istringstream iss(txt);
				std::string ssLine, oldLine;
				int ssLineNo = 0;

				while(!iss.eof())
				{
					getline(iss, ssLine);
					if(0 < ssLineNo++)
						*parsedText += "\n" + *indentAmount;
					oldLine = ssLine;
					*parsedText += ssLine;
				}
				if(indent)
					*indentAmount += into_whitespace(oldLine);

				return 1;
			}
			else if(param == NSM_CONS)
			{
				if(!*consoleLocked)
					os_mtx->lock();
				std::cout << txt;
				if(!*consoleLocked)
					os_mtx->unlock();

				return 1;
			}
			else
				return 0;
		}
		else if(generic_type::e_vector == gt.type)
			return 0;
		else if(generic_type::e_string == gt.type)
		{
			string_t param_strt(gt);
			std::string varName = std::string(param_strt.begin(), param_strt.size());

			VPos vpos;
			if(vars->find(varName, vpos))
			{
				if(vpos.type == "fstream")
				{
					vars->layers[vpos.layer].fstreams[varName] << txt;
					return 1;
				}
				else if(vpos.type == "ofstream")
				{
					vars->layers[vpos.layer].ofstreams[varName] << txt;
					return 1;
				}
				else
				{
					//write not defined for varName of type vpos.type
					return 0;
				}
			}
			else
			{
				//no variable named varName
				return 0;
			}
		}

		return 0;
	}
};


template <typename T>
struct exprtk_nsm_tonumber : public exprtk::igeneric_function<T>
{
	typedef typename exprtk::igeneric_function<T>::parameter_list_t parameter_list_t;

	Variables* vars;

	exprtk_nsm_tonumber() : exprtk::igeneric_function<T>("S")
	{}

	inline T operator()(parameter_list_t parameters)
	{
		typedef typename exprtk::igeneric_function<T>::generic_type generic_type;
		typedef typename generic_type::string_view string_t;

		generic_type& gt = parameters[0];
		string_t param_strt(gt);
		std::string varName = std::string(param_strt.begin(), param_strt.size());

		VPos vpos;
		double result = 0;
		if(vars->find(varName, vpos))
			vars->get_double_from_var(vpos, result);

		return result;
	}

	void setVars(Variables* Vars)
	{
		vars = Vars;
	}
};

template <typename T>
struct exprtk_nsm_tostring : public exprtk::igeneric_function<T>
{
	typedef typename exprtk::igeneric_function<T>::parameter_list_t parameter_list_t;

	Variables* vars;

	exprtk_nsm_tostring() : exprtk::igeneric_function<T>("S",exprtk::igeneric_function<T>::e_rtrn_string)
	{}

	inline T operator()(std::string& result, parameter_list_t parameters)
	{
		typedef typename exprtk::igeneric_function<T>::generic_type generic_type;
		typedef typename generic_type::string_view string_t;

		generic_type& gt = parameters[0];
		string_t param_strt(gt);
		std::string varName = std::string(param_strt.begin(), param_strt.size());

		result = "";

		VPos vpos;
		if(vars->find(varName, vpos))
			return vars->get_str_from_var(vpos, result, 1, 1);
		else
			result = "#error";

		return T(0);
	}

	void setVars(Variables* Vars)
	{
		vars = Vars;
	}
};

template <typename T>
struct exprtk_nsm_setnumber : public exprtk::igeneric_function<T>
{
	typedef typename exprtk::igeneric_function<T>::parameter_list_t parameter_list_t;

	Variables* vars;

	exprtk_nsm_setnumber() : exprtk::igeneric_function<T>("ST")
	{}

	inline T operator()(parameter_list_t parameters)
	{
		typedef typename exprtk::igeneric_function<T>::generic_type generic_type;
		typedef typename generic_type::scalar_view scalar_t;
		typedef typename generic_type::string_view string_t;

		generic_type& gt = parameters[0];
		string_t varName_strt(gt);
		std::string varName = std::string(varName_strt.begin(), varName_strt.size());

		gt = parameters[1];
		//scalar_t value_numt(gt);
		double value = scalar_t(gt).v_;

		VPos vpos;
		double result = 0;
		if(vars->find(varName, vpos))
			result = vars->set_var_from_double(vpos, value);

		return result;
	}

	void setVars(Variables* Vars)
	{
		vars = Vars;
	}
};

template <typename T>
struct exprtk_nsm_setstring : public exprtk::igeneric_function<T>
{
	typedef typename exprtk::igeneric_function<T>::parameter_list_t parameter_list_t;

	Variables* vars;

	exprtk_nsm_setstring() : exprtk::igeneric_function<T>("SS")
	{}

	inline T operator()(parameter_list_t parameters)
	{
		typedef typename exprtk::igeneric_function<T>::generic_type generic_type;
		typedef typename generic_type::string_view string_t;

		generic_type& gt = parameters[0];
		string_t varName_strt(gt);
		std::string varName = std::string(varName_strt.begin(), varName_strt.size());

		gt = parameters[1];
		string_t value_strt(gt);
		std::string value = std::string(value_strt.begin(), value_strt.size());

		VPos vpos;
		double result = 0;
		if(vars->find(varName, vpos))
			result = vars->set_var_from_str(vpos, value);

		return result;
	}

	void setVars(Variables* Vars)
	{
		vars = Vars;
	}
};

template <typename T>
struct gen_fn : public exprtk::igeneric_function<T>
{
	typedef typename exprtk::igeneric_function<T>::parameter_list_t parameter_list_t;

	Variables* vars;

	gen_fn()
	{}

	inline T operator()(parameter_list_t parameters)
	{
		typedef typename exprtk::igeneric_function<T>::generic_type generic_type;

		typedef typename generic_type::scalar_view scalar_t;
		typedef typename generic_type::vector_view vector_t;
		typedef typename generic_type::string_view string_t;

		for(std::size_t i = 0; i < parameters.size(); ++i)
		{
			generic_type& gt = parameters[i];

			if(generic_type::e_scalar == gt.type)
			{
				scalar_t param_numt(gt);
			}
			else if(generic_type::e_vector == gt.type)
			{
				vector_t param_vect(gt);
			}
			else if(generic_type::e_string == gt.type)
			{
				string_t param_strt(gt);
				std::string param = std::string(param_strt.begin(), param_strt.size());
			}
		}

		return -1;
	}
};

#endif //EXPRTKFNS_H_

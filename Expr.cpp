#include "Expr.h"

Expr::Expr()
{
}

void Expr::register_symbol_table(exprtk::symbol_table<double> &Symbol_Table)
{
	expression.register_symbol_table(Symbol_Table);
}

int Expr::set_expr(const std::string &Expr_Str)
{
	if(expr_str == Expr_Str)
		return 1;
	else
	{
		int result = parser.compile(Expr_Str, expression);
		if(result)
			expr_str = Expr_Str;
		else
			expr_str = "##ERROR##";
		return result;
	}
}

double Expr::evaluate()
{
	return expression.value();
}

double Expr::evaluate(const std::string &Expr_Str)
{
	if(Expr_Str != "" && expr_str != Expr_Str)
	{
		expr_str = Expr_Str;
		if(!parser.compile(Expr_Str,expression))
		{
			std::cout << "Compilation error..." << std::endl;
			//return 1;
		}
	}
	return expression.value();
}

//// ExprSet

ExprSet::ExprSet()
{
}

void ExprSet::add_symbol_table(exprtk::symbol_table<double>* Symbol_Table)
{
	symbol_table = Symbol_Table;
	for(auto expression=expressions.begin(); expression!=expressions.end(); ++expression)
		expression->second.register_symbol_table(*symbol_table);
}

int ExprSet::add_expr(const std::string &Expr_Str)
{
	if(!expressions.count(Expr_Str))
	{
		expressions[Expr_Str].register_symbol_table(*symbol_table);
		int result = parser.compile(Expr_Str, expressions[Expr_Str]);
		if(!result)
			expressions.erase(Expr_Str);
		return result;
	}
	else
		return 1;
}

double ExprSet::evaluate_last()
{
	return expressions[c_expr_str].value();
}

double ExprSet::evaluate(const std::string &Expr_Str)
{
	c_expr_str = Expr_Str;
	//add_expr(c_expr_str);

	return expressions[c_expr_str].value();
}

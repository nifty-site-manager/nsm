#include "Expr.h"

Expr::Expr()
{
}

void Expr::register_symbol_table(exprtk::symbol_table<double> &Symbol_Table)
{
	expression.register_symbol_table(Symbol_Table);
}

int Expr::compile(const std::string &Expr_Str)
{
	expr_str = Expr_Str;
	return parser.compile(expr_str, expression);
}

double Expr::evaluate()
{
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

int ExprSet::compile(const std::string& Name, const std::string &Expr_Str)
{
	expressions[Name].register_symbol_table(*symbol_table);
	int result = parser.compile(Expr_Str, expressions[Name]);
	if(result)
		expr_strs[Name] = Expr_Str;
	else
		expressions.erase(Name);
	return result;
}

double ExprSet::evaluate(const std::string &Name)
{
	return expressions[Name].value();
}

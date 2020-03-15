#ifndef EXPR_H_
#define EXPR_H_


#include <cstdio>
#include <iostream>

#include "exprtk/exprtk.h"

struct Expr
{
	std::string expr_str;
	exprtk::expression<double> expression;
	exprtk::parser<double> parser;

	Expr();

	void register_symbol_table(exprtk::symbol_table<double>& Symbol_Table);

	int set_expr(const std::string &Expr_Str);
	double evaluate();
	double evaluate(const std::string &Expr_Str);
};

struct ExprSet
{
	exprtk::symbol_table<double>* symbol_table;
	std::map<std::string, exprtk::expression<double> > expressions;
	std::string c_expr_str;
	exprtk::parser<double> parser;
	

	ExprSet();

	void add_symbol_table(exprtk::symbol_table<double>* Symbol_Table);

	int add_expr(const std::string &Expr_Str);
	double evaluate_last();
	double evaluate(const std::string &Expr_Str);
};


#endif //EXPR_H_

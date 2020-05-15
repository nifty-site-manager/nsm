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

	int compile(const std::string &Expr_Str);
	double evaluate();
};

struct ExprSet
{
	exprtk::symbol_table<double>* symbol_table;
	std::map<std::string, exprtk::expression<double> > expressions;
	std::map<std::string, std::string> expr_strs;
	exprtk::parser<double> parser;
	

	ExprSet();

	void add_symbol_table(exprtk::symbol_table<double>* Symbol_Table);

	int compile(const std::string& Name, const std::string &Expr_Str);
	double evaluate_last();
	double evaluate(const std::string &Name);
};


#endif //EXPR_H_

#include "PrimitiveProcedureCell.hpp"
#include "IntCell.hpp"
#include "DoubleCell.hpp"
#include "SymbolCell.hpp"
#include "ConsCell.hpp"
#include <functional>
#include <set>
#include <iterator>
using namespace std;

list<map<string, Cell*> > symbol_table(1, PrimitiveProcedureCell::create_map());


PrimitiveProcedureCell::PrimitiveProcedureCell(Cell* (*func)(Cell* const))
	:ProcedureCell(nil, nil), func_m(func)
{

}

PrimitiveProcedureCell::~PrimitiveProcedureCell()
{

}

Cell* PrimitiveProcedureCell::apply(Cell* const args)
{
	return func_m(args);
}

void PrimitiveProcedureCell::print(ostream& os) const
{
	os << "#<primitive>";
}

Cell* PrimitiveProcedureCell::add(Cell* const args)
{
	if(args->is_nil())
		return new IntCell(0);
	else
		return arithmetic_operation(args, plus<int>(), plus<double>(), "+");
}

Cell* PrimitiveProcedureCell::subtract(Cell* const args)
{
	if(args->is_nil())
		throw runtime_error("- operator requires at least one operand");
	else if(args->get_cdr()->is_nil()){
		Cell* operand = args->get_car()->eval();
		if(operand->is_int())
			return new IntCell(0 - operand->get_int());
		else if(operand->is_double())
			return new DoubleCell(0 - operand->get_double());
		else
			throw runtime_error("operant for - is neither an int nor a double");
	}
	else
		return arithmetic_operation(args, minus<int>(), minus<double>(), "-");
}

Cell* PrimitiveProcedureCell::multiply(Cell* const args)
{
	if(args->is_nil())
		return new IntCell(1);
	else
		return arithmetic_operation(args, multiplies<int>(), multiplies<double>(), "*");
}

Cell* PrimitiveProcedureCell::divide(Cell* const args)
{
	if(args->is_nil())
		throw runtime_error("/ operator requires at least one operand");
	else if(args->get_cdr()->is_nil()){
		Cell* operand = args->get_car()->eval();
		if(operand->is_int())
			return new IntCell(1 / operand->get_int());
		else if(operand->is_double())
			return new DoubleCell(1.0 / operand->get_double());
		else
			throw runtime_error("operant for / is neither an int nor a double");
	}
	else
		return arithmetic_operation(args, divides<int>(), divides<double>(), "/");
}

Cell* PrimitiveProcedureCell::less_than(Cell* const args)
{
	if(args->is_nil())
		return new IntCell(1);
	int is_increasing = 1;
	double prev, curr;
	Cell* curr_cons = args;
	Cell* curr_car = curr_cons->get_car()->eval();
	if(curr_car->is_int())
		prev = curr_car->get_int();
	else
		prev = curr_car->get_double();
	curr_cons = curr_cons->get_cdr();
	while(!(curr_cons->is_nil())){
		curr_car = curr_cons->get_car()->eval();
		if(curr_car->is_int())
			curr = curr_car->get_int();
		else
			curr = curr_car->get_double();
		if(prev >= curr){
			is_increasing = 0;
			// check whether the remaining operands are valid or not;
			curr_cons = curr_cons->get_cdr();
			while(!curr_cons->is_nil()){
				curr_car = curr_cons->get_car()->eval();
				if(!(curr_car->is_int() || curr_car->is_double()))
					throw runtime_error("try to use < operator with a Cell that cannot do");
				curr_cons = curr_cons->get_cdr();
			}
			break;
		}
		prev = curr;
		curr_cons = curr_cons->get_cdr();
	}
	return new IntCell(is_increasing);
}

Cell* PrimitiveProcedureCell::ceiling(Cell* const args)
{
	if(args->is_nil() || !(args->get_cdr()->is_nil()))
		throw runtime_error("ceiling operator requires exactly one double operand");
	Cell* operand = args->get_car()->eval();
	if(operand->is_double()){
		double d = operand->get_double();
		int i = static_cast<int>(d);
		if(d > 0 && d > i)
			++i;
		return new IntCell(i);
	}
	else
		throw runtime_error("try to use ceiling operator with non-double Cell");
}

Cell* PrimitiveProcedureCell::pri_floor(Cell* const args)
{
	if(args->is_nil() || !(args->get_cdr()->is_nil()))
		throw runtime_error("floor operator requires exactly one double operand");
	Cell* operand = args->get_car()->eval();
	if(operand->is_double()){
		double d = operand->get_double();
		int i = static_cast<int>(d);
		if(d < 0 && d < i)
			--i;
		return new IntCell(i);
	}
	else
		throw runtime_error("try to use floor operator with non-double Cell");
}

Cell* PrimitiveProcedureCell::ifelse(Cell* const args)
{
	Cell* curr_cons = args;
	if(curr_cons->is_nil() || curr_cons->get_cdr()->is_nil() 
	   || !(curr_cons->get_cdr()->get_cdr()->is_nil() 
			|| curr_cons->get_cdr()->get_cdr()->get_cdr()->is_nil()))
		throw runtime_error("if operator requires either two or three operands");
	Cell* condition_cell = curr_cons->get_car()->eval();
	if(condition_cell->is_int() && condition_cell->get_int() == 0
	   || condition_cell->is_double() && condition_cell->get_double() == 0){

		Cell* false_cell = curr_cons->get_cdr()->get_cdr();
		if(false_cell->is_nil())
			return nil;
		else
			return false_cell->get_car()->eval();
	}
	else
		return curr_cons->get_cdr()->get_car()->eval();
}

Cell* PrimitiveProcedureCell::quote(Cell* const args)
{
	if(args->is_nil() || !(args->get_cdr()->is_nil()))
		throw runtime_error("quote operator requires exactly one operand");
	return args->get_car();
}

Cell* PrimitiveProcedureCell::cons(Cell* const args)
{
	if(args->is_nil() || !(args->get_cdr()->is_nil() || args->get_cdr()->get_cdr()->is_nil()))
		throw runtime_error("cons operator requires exactly two operands");
	Cell* car = args->get_car()->eval();
	Cell* cdr = args->get_cdr()->get_car()->eval();
	return new ConsCell(car, cdr);
}

Cell* PrimitiveProcedureCell::car(Cell* const args)
{
	if(args->is_nil() || !(args->get_cdr()->is_nil()))
		throw runtime_error("car operator requires exactly one operand");
	Cell* operand = args->get_car()->eval();
	if(operand->is_cons())
		return operand->get_car();
	else
		throw runtime_error("car operator requires exactly one list operand");
}

Cell* PrimitiveProcedureCell::cdr(Cell* const args)
{
	if(args->is_nil() || !(args->get_cdr()->is_nil()))
		throw runtime_error("cdr operator requires exactly one operand");
	Cell* operand = args->get_car()->eval();
	if(operand->is_cons())
		return operand->get_cdr();
	else
		throw runtime_error("cdr operator requires exactly one list operand");
}

Cell* PrimitiveProcedureCell::nullp(Cell* const args)
{
	if(args->is_nil() || !(args->get_cdr()->is_nil()))
		throw runtime_error("nullp operator requires exactly one operand");
	Cell* operand = args->get_car()->eval();
	return new IntCell(operand->is_nil());
}

Cell* PrimitiveProcedureCell::pri_not(Cell* const args)
{
	if(args->is_nil() || !(args->get_cdr()->is_nil()))
		throw runtime_error("not operator requires exactly one operand");
	Cell* operand = args->get_car()->eval();
	if(operand->is_int() && operand->get_int() == 0 || operand->is_double() && operand->get_double() == 0)
		return new IntCell(1);
	else
		return new IntCell(0);
}

Cell* PrimitiveProcedureCell::define(Cell* const args)
{
	if(args->is_nil() || !(args->get_cdr()->is_nil() || args->get_cdr()->get_cdr()->is_nil()))
		throw runtime_error("define operator requires exactly two operands");
	string s = args->get_car()->get_symbol();
	if(symbol_table.begin()->find(s) != symbol_table.begin()->end())
		throw runtime_error("symbol " + s + " cannot be re-defined");
	else
		symbol_table.begin()->insert(map<string, Cell*>::value_type(s, args->get_cdr()->get_car()->eval()));
	return nil;
}

Cell* PrimitiveProcedureCell::pri_print(Cell* const args)
{
	if(args->is_nil() || !(args->get_cdr()->is_nil()))
		throw runtime_error("print operator requires exactly one operand");
	args->get_car()->eval()->print();
	cout << endl;
	return nil;
}

Cell* PrimitiveProcedureCell::pri_eval(Cell* const args)
{
	if(args->is_nil() || !(args->get_cdr()->is_nil()))
		throw runtime_error("eval operator requires exactly one operand");
	return args->get_car()->eval()->eval();
}

Cell* PrimitiveProcedureCell::lambda(Cell* const args)
{
	if(args->is_nil() || args->get_cdr()->is_nil())
		throw runtime_error("lambda operator requires two or more operands");
	Cell* formals = args->get_car();
	Cell* body = args->get_cdr();
	if(formals->is_symbol())
		throw runtime_error("variable number of arguments is not supported in the current version");
	else{
		set<string> arg_set; // use to check duplicate argument name
		string s;
		Cell* curr_cons = formals;
		while(!curr_cons->is_nil()){
			try{
				s = curr_cons->get_car()->get_symbol();
			}catch(const runtime_error& e){
				throw runtime_error("the arguments list contains invalid identifier(s)");
			}
			if(arg_set.insert(s).second == false)
				throw runtime_error("duplicate argument name: " + s);
			curr_cons = curr_cons->get_cdr();
		}
	}
	return new ProcedureCell(formals, body);
}

Cell* PrimitiveProcedureCell::pri_apply(Cell* const args)
{
	if(args->is_nil() || !(args->get_cdr()->is_nil() || args->get_cdr()->get_cdr()->is_nil()))
		throw runtime_error("apply operator requires exactly two operands");
	Cell* procedure = args->get_car()->eval();
	Cell* arguments = args->get_cdr()->get_car()->eval();
	return procedure->apply(arguments);
}

Cell* PrimitiveProcedureCell::let(Cell* const args)
{
	if(args->is_nil() || args->get_cdr()->is_nil())
		throw runtime_error("let operator requires two or more operands");
	Cell *formals = nil, *arguments = nil, *body = args->get_cdr(),
		*curr_cons = args->get_car(), *curr_pair;
	set<string> arg_set;
	string s;
	while(!curr_cons->is_nil()){
		curr_pair = curr_cons->get_car();
		if(curr_pair->is_nil() || curr_pair->get_cdr()->is_nil() || !(curr_pair->get_cdr()->get_cdr()->is_nil()))
			throw runtime_error("bad syntax");
		s = curr_pair->get_car()->get_symbol();
		if(arg_set.insert(s).second == false)
			throw runtime_error("duplicate argument name: " + s);
		formals = new ConsCell(new SymbolCell(s), formals);
		arguments = new ConsCell(curr_pair->get_cdr()->get_car(), arguments);
		curr_cons = curr_cons->get_cdr();
	}
	return ProcedureCell(formals, body).apply(arguments);
}

template <typename IntOp, typename DoubleOp>
Cell* PrimitiveProcedureCell::arithmetic_operation(Cell* const operands, IntOp int_op, DoubleOp double_op, const std::string& op)
{
	int int_result = 0;
	double double_result = 0;
	Cell* curr_operand = operands->get_car()->eval();
	Cell* curr_cons = operands->get_cdr();
	if(curr_operand->is_int()){
		int_result = curr_operand->get_int();
		while(!(curr_cons->is_nil())){
			curr_operand = curr_cons->get_car()->eval();
			curr_cons = curr_cons->get_cdr();
			if(curr_operand->is_int())
				int_result = int_op(int_result, curr_operand->get_int());
			else if(curr_operand->is_double()){
				double_result = double_op(int_result, curr_operand->get_double());
				if(curr_cons->is_nil())
					return new DoubleCell(double_result);
				break;
			}
			else
				throw runtime_error("operant for " + op + " is neither an int nor a double");
		}
		if(curr_cons->is_nil())
			return new IntCell(int_result);
	}
	else if(curr_operand->is_double())
		double_result = curr_operand->get_double();
	else
		throw runtime_error("operant for " + op + " is neither an int nor a double");

	while(!(curr_cons->is_nil())){
		curr_operand = curr_cons->get_car()->eval();
		curr_cons = curr_cons->get_cdr();
		if(curr_operand->is_int())
			double_result = double_op(double_result, curr_operand->get_int());
		else if(curr_operand->is_double())
			double_result = double_op(double_result, curr_operand->get_double());
		else
			throw runtime_error("operant for " + op + " is neither an int nor a double");
	}
	return new DoubleCell(double_result);
}

map<string, Cell*> PrimitiveProcedureCell::create_map()
{
	map<string, Cell*> init_map;
	init_map.insert(pair<string, Cell*>("+", new PrimitiveProcedureCell(&add)));
	init_map.insert(pair<string, Cell*>("-", new PrimitiveProcedureCell(&subtract)));
	init_map.insert(pair<string, Cell*>("*", new PrimitiveProcedureCell(&multiply)));
	init_map.insert(pair<string, Cell*>("/", new PrimitiveProcedureCell(&divide)));
	init_map.insert(pair<string, Cell*>("<", new PrimitiveProcedureCell(&less_than)));
	init_map.insert(pair<string, Cell*>("ceiling", new PrimitiveProcedureCell(&ceiling)));
	init_map.insert(pair<string, Cell*>("floor", new PrimitiveProcedureCell(&pri_floor)));
	init_map.insert(pair<string, Cell*>("if", new PrimitiveProcedureCell(&ifelse)));
	init_map.insert(pair<string, Cell*>("quote", new PrimitiveProcedureCell(&quote)));
	init_map.insert(pair<string, Cell*>("cons", new PrimitiveProcedureCell(&cons)));
	init_map.insert(pair<string, Cell*>("car", new PrimitiveProcedureCell(&car)));
	init_map.insert(pair<string, Cell*>("cdr", new PrimitiveProcedureCell(&cdr)));
	init_map.insert(pair<string, Cell*>("nullp", new PrimitiveProcedureCell(&nullp)));
	init_map.insert(pair<string, Cell*>("not", new PrimitiveProcedureCell(&pri_not)));
	init_map.insert(pair<string, Cell*>("define", new PrimitiveProcedureCell(&define)));
	init_map.insert(pair<string, Cell*>("print", new PrimitiveProcedureCell(&pri_print)));
	init_map.insert(pair<string, Cell*>("eval", new PrimitiveProcedureCell(&pri_eval)));
	init_map.insert(pair<string, Cell*>("lambda", new PrimitiveProcedureCell(&lambda)));
	init_map.insert(pair<string, Cell*>("apply", new PrimitiveProcedureCell(&pri_apply)));
	init_map.insert(pair<string, Cell*>("let", new PrimitiveProcedureCell(&let)));
	return init_map;
}
#ifndef STRFUNCTION_H
#define STRFUNCTION_H

//Standard includes
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

//Root includes
#include "TTree.h"
#include "TNtuple.h"

//Local includes

//Definitions

using namespace std;

vector<string> GetExprArgs(string expr)
{
	vector<string> args = {};
	string arg, c;
	int i = 0;

	//remove spaces
	arg = "";
	for(i = 0; i < (int)expr.length(); i++)
	{
		c = expr.substr(i, 1);
		if(c == " ")continue;
		arg += c;
	}
	expr = arg;

	arg = "";
	int depth = 0;
	for(i = 0; i < (int)expr.length(); i++)
	{
		c = expr.substr(i, 1);

		if(c == ")")depth--;
		if(depth >= 1)
		{
			if(depth == 1 and c == ",")
			{
				args.push_back(arg);
				arg = "";
			}
			else
			{
				arg += c;
			}
		}
		if(c == "(")depth++;		
	}
	if(arg.length() > 0)args.push_back(arg);

	return args;
}

vector<string> GetExprVars(string expr)
{
	vector<string> vars = {};
	
	vector<string> args = GetExprArgs(expr);
	if(args.size() > 0)
	{
		int i, j, k;
		bool b;

		vector<string> chld_vars;
		for(i = 0; i < args.size(); i++)
		{
			chld_vars = GetExprVars(args[i]);
			for(j = 0; j < chld_vars.size(); j++)
			{
				b = true;
				for(k = 0; k < vars.size(); k++)
				{
					if(chld_vars[j] == vars[k])
					{
						b = false;
						break;
					}
				}
				if(b)
				{
					vars.push_back(chld_vars[j]);
				}
			}
			chld_vars.clear();
		}
	}
	else
	{
		vars.push_back(expr);
	}

	return vars;	
}

template <class T>
class NamedFunction
{
public:
	string name;
	T (*func)(T*);

	NamedFunction(string n, T (*f)(T*))
	{
		name = n;
		func = f;
	}
};

template <class T>
class StrFunction;

template <class T>
class StrFunction
{
public:
	string expr;
	NamedFunction<T>* named_func;

	int num_args;
	StrFunction<T>** arg_funcs;
	T* arg_vals;

	void Free()
	{
		if(arg_funcs != 0x0)
		{
			for(int i = 0; i < num_args; i++)
			{
				if(arg_funcs[i] != 0x0) //This check should be superfluous
				{
					arg_funcs[i]->Free(); //When constructing or setting, memory was allocated recursively

					delete arg_funcs[i];
					arg_funcs[i] = 0x0; //This step should be moot
				}
			}
			delete[] arg_funcs;
			arg_funcs = 0x0;
			
			delete[] arg_vals; //arg_vals would only have been allocated dynamically if arg_funcs also was
			arg_vals = 0x0;

			num_args = 0;
		}
	}

	void SetAddresses(vector<NamedFunction<T>*> named_funcs, TNtuple* nt)
	{
		Free();

		int i;
		string name, c;

		named_func = 0x0;

		vector<string> args = GetExprArgs(expr);
		num_args = (int)args.size();

		name = "";
		for(i = 0; i < (int)expr.length(); i++)
		{
			c = expr.substr(i, 1);
			if(c == " ")continue;
			if(c == "(")break;

			name += c;
		}

		if(num_args == 0)
		{
			//If there are no arguments, we are at the inner most level of the recurrsion
			//"name" is the name of a branch in the nt, and calling Evaluate() should return its value

			//Find the identity function I, which returns the value at the address of *args
			for(i = 0; i < named_funcs.size(); i++)
			{
				if(named_funcs[i]->name == "I")
				{
					named_func = named_funcs[i];
				}
			}
			if(named_func == 0x0)
			{
				cout << "Error: Could not find identity function \"I\" in list of named functions" << endl;
			}

			//Find the address of the branch "name" and give this address to *args
			if(nt->GetBranch(name.c_str()) != 0x0)
			{
				arg_vals = (T*)((nt->GetBranch(name.c_str()))->GetAddress());
			}
			else
			{
				arg_vals = 0x0;
				cout << "Error: Could not find branch \"" << name << "\" in given TNtuple" << endl;
			}

			//There are now arguments to call recursively, so we'll set them to null here
			arg_funcs = 0x0;
			
			//Notice no memory is allocated at the innermost level of recursion
		}
		else
		{
			//If there are arguments, "name" is the name of some named function
			//There will be expressions for the arguments

			//Find the function "name" in the give list of named functions
			for(i = 0; i < (int)named_funcs.size(); i++)
			{
				if(named_funcs[i]->name == name)
				{
					named_func = named_funcs[i];
				}
			}
			if(named_func == 0x0)
			{
				cout << "Error: Could not find function \"" << name << "\" in list of named functions" << endl;
			}
			
			//Allocate memory for the arguments
			arg_funcs = new StrFunction*[num_args];
			arg_vals = new T[num_args];
			for(i = 0; i < num_args; i++)
			{
				arg_funcs[i] = new StrFunction(args[i], named_funcs, nt);
			}
		}
	}

	T Evaluate()
	{
		for(int i = 0; i < num_args; i++)
		{
			arg_vals[i] = arg_funcs[i]->Evaluate();
		}

		return named_func->func(arg_vals);
	}

	StrFunction(string expression, vector<NamedFunction<T>*> named_funcs, TNtuple* nt)
	{
		named_func = 0x0;

		num_args = 0;
		arg_funcs = 0x0;
		arg_vals = 0x0;

		expr = expression;
		SetAddresses(named_funcs, nt);
	}

	~StrFunction()
	{
		Free();
	}
};

#endif

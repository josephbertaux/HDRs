#ifndef COMMONFUNCTIONS_H
#define COMMONFUNCTIONS_H

//Standard includes
#include <cmath>

//Root includes

//Local includes
#include "StrFunction.h"

//Definitions

using namespace std;

template <class T>
class CommonFunctions
{
public:
	static T I(T* args){return args[0];}

	static T ADD(T* args){return args[0] + args[1];}
	static T SUB(T* args){return args[0] - args[1];}

	static T MUL(T* args){return args[0] * args[1];}
	static T DIV(T* args){return args[0] / args[1];}

	static T EXP(T* args){return exp(args[0]);}
	static T LOG(T* args){return log(args[0]);}

	vector<NamedFunction<T>*> common_funcs = {};

	CommonFunctions()
	{
		NamedFunction<T>* NAMED_I = new NamedFunction<T>("I", &I);		common_funcs.push_back(NAMED_I);

		NamedFunction<T>* NAMED_ADD = new NamedFunction<T>("ADD", &ADD);	common_funcs.push_back(NAMED_ADD);
		NamedFunction<T>* NAMED_SUB = new NamedFunction<T>("SUB", &SUB);	common_funcs.push_back(NAMED_SUB);

		NamedFunction<T>* NAMED_MUL = new NamedFunction<T>("MUL", &MUL);	common_funcs.push_back(NAMED_MUL);
		NamedFunction<T>* NAMED_DIV = new NamedFunction<T>("DIV", &DIV);	common_funcs.push_back(NAMED_DIV);

		NamedFunction<T>* NAMED_EXP = new NamedFunction<T>("EXP", &EXP);	common_funcs.push_back(NAMED_EXP);
		NamedFunction<T>* NAMED_LOG = new NamedFunction<T>("LOG", &LOG);	common_funcs.push_back(NAMED_LOG);
	}
};

#endif

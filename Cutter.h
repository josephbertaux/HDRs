#ifndef CUTTER_H
#define CUTTER_H

//Standard includes
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

//Root includes
#include "TROOT.h"
#include "TSystem.h"
#include "TCanvas.h"
#include "TFile.h"
#include "TTree.h"
#include "TNtuple.h"
#include "TH1.h"
#include "TF1.h"

//Local includes
#include "StrFunction.h"
#include "CommonFunctions.h"

//Definitions
static int MAX_CHAR_LEN = 256;

using namespace std;

class VarCut
{
public:
	string name; //The name of the cut when writing short strings to specify the range used for a specific application of this cut
	string expr; //The name of a branch (or StrFunction expression of branches) used to compare to bin edges

	vector<pair<float, float>> bounds; //The edges of bins

	VarCut(string n, string expression, int num_edges, float edges[])
	{
		//edges assumed to have size num_bins + 1

		name = n;
		expr = expression;

		for(int i = 0; i < num_edges-1; i++)
		{
			bounds.push_back(pair<float, float>(edges[i], edges[i+1]));
		}
	}

	VarCut(string config_str)
	{
		//Assumes config_str is formatted as
		//config_str = "name:expr:edges[0]:...:edges[num_edges-1]"
		
		int i = 0;
		
		name = "";
		while(i < config_str.length())
		{
			if(config_str.substr(i, 1) == ":")break;

			name += config_str.substr(i, 1);
			i++;
		}
		i++;

		expr = "";
		while(i < config_str.length())
		{
			if(config_str.substr(i, 1) == ":")break;

			expr += config_str.substr(i, 1);
			i++;
		}
		i++;

		string temp1 = "";
		while(i < config_str.length())
		{
			if(config_str.substr(i, 1) == ":")break;

			temp1 += config_str.substr(i, 1);
			i++;
		}
		i++;

		string temp2 = "";
		while(i < config_str.length())
		{
			if(config_str.substr(i, 1) == ":")
			{
				bounds.push_back(pair<float, float>(stof(temp1), stof(temp2)));
				temp1 = temp2;
				temp2 = "";
			}
			else
			{
				temp2 += config_str.substr(i, 1);
			}

			i++;
		}

		bounds.push_back(pair<float, float>(stof(temp1), stof(temp2)));
	}

	float min(int i)
	{
		if(i < 0)
		{
			cout << "In VarCut::min(int i):" << endl;
			cout << "i < 0, using i = 0" << endl;
			i = 0;
		}
		if(i > (int)bounds.size() - 1)
		{
			cout << "In VarCut::min(int i):" << endl;
			cout << "i > bounds.size()-1, using i = bounds.size()-1" << endl;
			i = (int)bounds.size() - 1;
		}

		return bounds[i].first;
	}

	float max(int i)
	{
		if(i < 0)
		{
			cout << "In VarCut::max(int i):" << endl;
			cout << "i < 0, using i = 0" << endl;
			i = 0;
		}
		if(i > (int)bounds.size() - 1)
		{
			cout << "In VarCut::max(int i):" << endl;
			cout << "i > bounds.size()-1, using i = bounds.size()-1" << endl;
			i = (int)bounds.size() - 1;
		}

		return bounds[i].second;
	}

	int Size()
	{
		return bounds.size();
	}

	void AddBound(float min, float max)
	{
		bounds.push_back(pair<float, float>(min, max));
	}
};



class Cutter
{
private:
	int i, j;
	float f;
	bool b;
public:
	CommonFunctions<float>* cf = {};
	TNtuple* nt;

	//Maybe wrap these into a vector<tuple<VarCut, int, StrFunction<float>*>>?
	vector<VarCut> var_cuts = {};
	vector<StrFunction<float>*> var_funcs = {};
	vector<int> indexes = {};

	Cutter()
	{
		cf = new CommonFunctions<float>();
		nt = 0x0;		

		i = 0; j = 0;
		f = 0.0;
		b = false;
	}

	void Free()
	{
		//Free memory that would have been allocated previously
		for(i = 0; i < (int)var_funcs.size(); i++)
		{
			var_funcs[i]->Free();
			delete var_funcs[i];
		}
		var_funcs.clear();
	}

	void AddCut(VarCut vc)
	{
		var_cuts.push_back(vc);
		indexes.push_back(0);
	}

	int NumCuts()
	{
		return var_cuts.size();
	}

	int NumBins()
	{
		j = 1;
		for(i = 0; i < NumCuts(); i++)
		{
			j *= var_cuts[i].Size();
		}

		return j;
	}

	void Index(int k)
	{
		j = NumBins();
		
		for(i = NumCuts()-1; i >= 0; i--)
		{
			j /= var_cuts[i].Size();
			indexes[i] = (int)(k / j);
			k %= j;
		}
	}

	void SetNtuple(TNtuple* ntuple)
	{
		//This must be called AFTER addressing the branches of tree in the main program
		//With some extra trickery, though, it might not have to but I'm not doing that just yet
		Free();

		nt = ntuple;

		for(i = 0; i < NumCuts(); i++)
		{
			var_funcs.push_back(new StrFunction<float>(var_cuts[i].expr, cf->common_funcs, nt));
		}
	}

	bool Check()
	{
		if(nt == 0x0)
		{
			cout << "In Cutter::Check(vector<int> indexes):" << endl;
			cout << "TNtuple* nt was null during call" << endl;
			cout << "Returning false" << endl;
			return false;
		}

		b = true;
		for(i = 0; i < NumCuts(); i++)
		{
			j = indexes[i];
			f = var_funcs[i]->Evaluate();
			if(!(var_cuts[i].min(j) <= f and f < var_cuts[i].max(j)))b = false;
		}

		return b;
	}

	void Write(char* c, string f = "%s_%+08.3f_%+08.3f_")
	{
		char temp[MAX_CHAR_LEN];
		strcpy(c, "");
		for(i = 0; i < var_cuts.size(); i++)
		{
			j = indexes[i];

			sprintf(temp, f.c_str(), var_cuts[i].name.c_str(), var_cuts[i].min(j), var_cuts[i].max(j));
			for(j = 0; j < MAX_CHAR_LEN; j++)
			{
				if(temp[j] == '+' or temp[j] == '.')
				{
					temp[j] = 'p';
				}
				else if(temp[j] == '-')
				{
					temp[j] == 'm';
				}
			}
			strcat(c, temp);
		}
	}

	~Cutter()
	{
		Free();
		delete cf;
	}
};

#endif

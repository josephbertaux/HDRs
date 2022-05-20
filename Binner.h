#ifndef BINNER_H
#define BINNER_H

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
#include "CommonFunctions.h"

//Definitions
static int MAX_CHAR_LEN = 256;

using namespace std;

class VarBinner
{
protected:
	vector<pair<float, float>> bins;
	int index;

public:
	string name; //The name of the cut when writing short strings to specify the range used for a specific application of this cut
	string expr; //The name of a branch (or StrFunction expression of branches) used to compare to bin edges

	VarBinner()
	{
		name = "";
		expr = "";
		index = 0;

		bins.clear();
	}

	VarBinner(string n, string expression, int num_edges, float edges[])
	{
		//edges assumed to have size num_bins + 1

		name = n;
		expr = expression;
		index = 0;

		for(int i = 0; i < num_edges-1; i++)
		{
			bins.push_back(pair<float, float>(edges[i], edges[i+1]));
		}
	}

	VarBinner(string config_str)
	{
		//Assumes config_str is formatted as
		//config_str = "name:expr:edges[0]:...:edges[num_edges-1]"
		
		index = 0;

		int i = 0;
		
		cout << "Making VarBinner from string: " << config_str << endl;
		
		cout << "Name: ";
		name = "";
		while(i < config_str.length())
		{
			if(config_str.substr(i, 1) == ":")break;

			name += config_str.substr(i, 1);
			i++;
		}
		cout << name << endl;
		i++;

		cout << "Expression: ";
		expr = "";
		while(i < config_str.length())
		{
			if(config_str.substr(i, 1) == ":")break;

			expr += config_str.substr(i, 1);
			i++;
		}
		cout << expr << endl;
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
				bins.push_back(pair<float, float>(stof(temp1), stof(temp2)));
				temp1 = temp2;
				temp2 = "";
			}
			else
			{
				temp2 += config_str.substr(i, 1);
			}

			i++;
		}

		bins.push_back(pair<float, float>(stof(temp1), stof(temp2)));
		cout << "Bins: ";
		for(i = 0; i < bins.size(); i++)
		{
			cout << "(" << bins[i].first << ", " << bins[i].second << ") ";
		}
		cout << endl;
		cout << endl;
	}

	void SetIndex(int i)
	{
		if(0 <= i and i < Size())
		{
			index = i;
		}
	}

	int GetIndex()
	{
		return index;
	}

	int Size()
	{
		return bins.size();
	}

	float Min(int i = -1)
	{
		if(0 <= i and i < Size())
		{
			index = i;
		}

		if(0 <= index and index < Size())
		{
			return bins[index].first;
		}

		cout << "Error in VarBinner::Min(), index out of bounds" << endl;
		return 0.0;
	}

	float Max(int i = -1)
	{
		if(0 <= i and i < Size())
		{
			index = i;
		}

		if(0 <= index and index < Size())
		{
			return bins[index].second;
		}

		cout << "Error in VarBinner::Max(), index out of bounds" << endl;
		return 0.0;
	}

	string Write(int k = -1, string f = "_%s_%+08.3f_%+08.3f")
	{
		SetIndex(k);
		char temp[MAX_CHAR_LEN];

		string str = "";
		sprintf(temp, f.c_str(), name.c_str(), Min(), Max());
		for(int i = 0; i < MAX_CHAR_LEN; i++)
		{
			if(temp[i] == '+' or temp[i] == '.')
			{
				temp[i] = 'p';
			}
			else if(temp[i] == '-')
			{
				temp[i] = 'm';
			}
		}
		str += temp;

		return str;
	}

	void AddBin(float min, float max)
	{
		bins.push_back(pair<float, float>(min, max));
	}

	void SetBinBounds(float min, float max, int i = -1)
	{
		if(0 <= i and i < Size())
		{
			index = i;
		}

		if(0 <= index and index < Size())
		{
			bins[index].first = min;
			bins[index].second = max;
		}
	}
};

class Binner
{
protected:
	int i, j;
	float f;
	bool b;

	vector<VarBinner> var_binners = {};
	vector<StrFunction<float>*> var_strfuncs = {};

	CommonFunctions<float>* cf = {};
	TNtuple* nt;

	vector<string> branch_names = {};
	float* branch_vals;

public:
	void Free()
	{
		//Free memory that would have been allocated previously
		for(i = 0; i < (int)var_strfuncs.size(); i++)
		{
			if(var_strfuncs[i] != 0x0)
			{
				var_strfuncs[i]->Free();
				delete var_strfuncs[i];
				var_strfuncs[i] = 0x0;
			}
			
		}
		var_strfuncs.clear();

		if(branch_vals != 0x0)
		{
			delete branch_vals;
			branch_vals = 0x0;
		}
	}

	void AddBinner(VarBinner vb)
	{
		var_binners.push_back(vb);
	}

	VarBinner* GetBinner(string name)
	{
		for(i = 0; i < var_binners.size(); i++)
		{
			if(var_binners[i].name == name)
			{
				return &(var_binners[i]);
			}
		}

		return 0x0;
	}

	int NumBinners()
	{
		return var_binners.size();
	}

	int NumBins()
	{
		j = 1;
		for(i = 0; i < NumBinners(); i++)
		{
			j *= var_binners[i].Size();
		}

		return j;
	}

	void Index(int k)
	{
		j = NumBins();
		
		for(i = NumBinners()-1; i >= 0; i--)
		{
			j /= var_binners[i].Size();
			var_binners[i].SetIndex(k / j);
			k %= j;
		}
	}

	bool Check(int k = -1)
	{
		if(k > -1)Index(k);

		if(nt == 0x0)
		{
			cout << "In Binner::Check(int i):" << endl;
			cout << "TNtuple* nt was null during call" << endl;
			cout << "Returning false" << endl;
			return false;
		}

		b = true;
		for(i = 0; i < NumBinners(); i++)
		{
			if(var_strfuncs[i] == 0x0)
			{
				cout << "In Binner::Check(int i):" << endl;
				cout << "StrFunction var_strfuncs[" << i << "] was null" << endl;
				cout << "Returning false" << endl;

				return false;
			}
			f = var_strfuncs[i]->Evaluate();
			if(!(var_binners[i].Min() <= f and f < var_binners[i].Max()))b = false;
		}

		return b;
	}

	string Write(int k = -1, string f = "_%s_%+08.3f_%+08.3f")
	{
		if(k > -1)Index(k);

		char temp[MAX_CHAR_LEN];

		string str = "";
		for(i = 0; i < NumBinners(); i++)
		{
			sprintf(temp, f.c_str(), var_binners[i].name.c_str(), var_binners[i].Min(), var_binners[i].Max());
			for(j = 0; j < MAX_CHAR_LEN; j++)
			{
				if(temp[j] == '+' or temp[j] == '.')
				{
					temp[j] = 'p';
				}
				else if(temp[j] == '-')
				{
					temp[j] = 'm';
				}
			}
			str += temp;
		}

		return str;
	}

	float GetVarVal(string name)
	{
		for(i = 0; i < var_binners.size(); i++)
		{
			if(var_binners[i].name == name)
			{
				j = i;
				break;
			}
		}

		if(var_strfuncs[j] == 0x0)
		{
			cout << name << "'s strfunc was null" << endl;
			cout << "returning 0.0" << endl;

			return 0.0;
		}

		return var_strfuncs[i]->Evaluate();
	}

	void PrintVarVals()
	{
		for(i = 0; i < NumBinners(); i++)
		{
			if(var_strfuncs[i] == 0x0)
			{
				cout << "var_strfuncs[" << i << "] was null, skipping" << endl;
				continue;
			}

			cout << var_binners[i].name << ": " << var_strfuncs[i]->Evaluate() << endl;
		}
	}

	void SetUp()
	{
		Free();

		int k;
		bool b = false;
		vector<string> vars = {};

		branch_names.clear();
		for(i = 0; i < NumBinners(); i++)
		{
			vars = GetExprVars(var_binners[i].expr);
			for(j = 0; j < vars.size(); j++)
			{
				b = true;
				for(k = 0; k < branch_names.size(); k++)
				{
					if(branch_names[k] == vars[j])
					{
						b = false;
						break;
					}
				}

				if(b)
				{
					branch_names.push_back(vars[j]);
				}
			}
			vars.clear();
		}

		branch_vals = new float[branch_names.size()];
	}

	void SetNtuple(TNtuple* ntuple)
	{
		nt = ntuple;
		if(nt == 0x0)
		{
			cout << "In Binner::SetNtuple(TNtuple* ntuple)" << endl;
			cout << "ntuple was null, exiting" << endl;
			return;
		}

		nt->ResetBranchAddresses();
		nt->SetBranchStatus("*", 0);
		for(i = 0; i < branch_names.size(); i++)
		{
			if(nt->GetBranch(branch_names[i].c_str()) == 0x0)
			{
				cout << "In Binner::SetNtuple(TNtuple* ntuple)" << endl;
				cout << "branch \"" << branch_names[i] << "\" not found in ntuple, continuing" << endl;
				continue;
			}

			nt->SetBranchStatus(branch_names[i].c_str(), 1);
			nt->SetBranchAddress(branch_names[i].c_str(), &(branch_vals[i]));
		}


		for(i = 0; i < NumBinners(); i++)
		{
			var_strfuncs.push_back(new StrFunction<float>(var_binners[i].expr, cf->common_funcs, nt));
		}
	}

	Binner()
	{
		cf = new CommonFunctions<float>();
		nt = 0x0;		

		i = 0; j = 0;
		f = 0.0;
		b = false;

		branch_vals = 0x0;
	}

	Binner(string config_filename)
	{
		cf = new CommonFunctions<float>();
		nt = 0x0;		

		i = 0; j = 0;
		f = 0.0;
		b = false;

		branch_vals = 0x0;

		string config_str;
		ifstream config;
		config.open(config_filename, ifstream::in);

		if(config.fail())
		{
			cout << "Could not open file:" << endl;
			cout << config_filename << endl;

			return;
		}

		config.clear();
		config.seekg(0, ios::beg);
		while(true)
		{
			config >> config_str;

			//if(config.eof())break;

			AddBinner(VarBinner(config_str));

			if(config.peek())break;
		}
		config.close();

		SetUp();
	}

	~Binner()
	{
		Free();
		if(cf != 0x0)
		{
			delete cf;
			cf = 0x0;
		}
	}
};

#endif

#ifndef FILLER_H
#define FILLER_H

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
#include "Binner.h"

//Definitions

using namespace std;

class Filler: public Binner
{
private:
	StrFunction<float>* fill_strfunc;

public:
	string fill_name;
	string fill_expr;
	
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

		if(fill_strfunc != 0x0)
		{
			fill_strfunc->Free();
			delete fill_strfunc;
			fill_strfunc = 0x0;
		}

		if(branch_vals != 0x0)
		{
		        delete branch_vals;
		        branch_vals = 0x0;
		}
        }

	string Write(int k = -1, string f = "_%s_%+08.3f_%+08.3f")
	{
		if(k > -1)Index(k);

		char temp[MAX_CHAR_LEN];

		string str = fill_name;
		for(i = 0; i < NumBinners(); i++)
		{
			j = indexes[i];

			sprintf(temp, f.c_str(), var_binners[i].name.c_str(), var_binners[i].min(j), var_binners[i].max(j));
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

	void PrintVarVals()
	{
		if(fill_strfunc == 0x0)
		{
			cout << "Fill StrFunction was null during call, skipping" << endl;
		}
		else
		{
			cout << "Fill value \"" << fill_name << "\": " << fill_strfunc->Evaluate() << endl;
		}

		for(i = 0; i < var_binners.size(); i++)
		{
			if(var_strfuncs[i] == 0x0)
			{
				cout << "var_binners[" << i << "] was null, skipping" << endl;
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

		vars = GetExprVars(fill_expr);
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

		fill_strfunc = new StrFunction<float>(fill_expr, cf->common_funcs, nt);
	}

	float GetFillVal()
	{
		if(fill_strfunc == 0x0)
		{
			cout << "In Filler::GetFillVal, fill_strfunc was null at call" << endl;
			return 0.0;
		}

		return fill_strfunc->Evaluate();
	}

	void TryFill(TH1* h, int k=-1)
	{
		if(h == 0x0)
		{
			cout << "In Filler::TryFill, histogram is null" << endl;
			cout << endl;
			return;
		}

		if(fill_strfunc == 0x0)
		{
			cout << "In Filler::TryFill, fill_strfunc is null" << endl;
			cout << endl;
			return;
		}

		if(Check(k))h->Fill(fill_strfunc->Evaluate());
	}

	Filler()
	{
		cf = new CommonFunctions<float>();
		nt = 0x0;		

		i = 0; j = 0;
		f = 0.0;
		b = false;

		branch_vals = 0x0;
		fill_strfunc = 0x0;
	}

	Filler(string config_filename)
	{
		cout << "Reading Filler from config file " << config_filename << endl;

		cf = new CommonFunctions<float>();
		nt = 0x0;		

		i = 0; j = 0;
		f = 0.0;
		b = false;

		branch_vals = 0x0;
		fill_strfunc = 0x0;

		string config_str;
		ifstream config(config_filename);

		config >> config_str; //First line is reserved for histogram
		if(config.eof())return;

		config >> config_str; //Second line contains information about the fill quantity
		if(config.eof())return;
		
		cout << "Name: ";
		fill_name = "";
		while(i < config_str.length())
		{
			if(config_str.substr(i, 1) == ":")
			{
				break;
			}

			fill_name += config_str.substr(i,1);
			i++;
		}
		cout << fill_name << endl;
		i++;

		cout << "Expression: ";
		fill_expr = "";
		while(i < config_str.length())
		{
			if(config_str.substr(i, 1) == ":")
			{
				break;
			}

			fill_expr += config_str.substr(i,1);
			i++;
		}
		cout << fill_expr << endl;
		cout << endl;

		while(true)
		{
			config >> config_str;

			if(config.eof())break;

			AddBinner(VarBinner(config_str));
		}

		SetUp();
	}

	~Filler()
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

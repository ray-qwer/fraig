/****************************************************************************
  FileName     [ cirCut.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir cut functions ]
  Author       [ Tonytony, Ray, Wilson ]
  Copyright    [ Copyleft(c) 2021-present SatMeeting, NTUEE, Taiwan ]
****************************************************************************/

#include <fstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cassert>
#include <map>
#include <time.h>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"
#include "unordered_map"
#include "sat.h"

using namespace std;

bool CutMatching(vector<CirGate*>& a_list, vector<CirGate*>& b_list, vector<CirGate*>& new_b_list)
{
    SatSolver solver;
    solver.initialize();
    int size_a=a_list.size(),size_b=b_list.size(), size_newb=new_b_list.size();
    cout<<"size_a: "<<size_a<<", size_b: "<<size_b<<", size_newb: "<<size_newb<<endl;
    vector<vector<Var> > Vars(size_a,vector<Var>(size_b+size_newb,0));
    for(int i=0;i<size_a;++i){
        for(int j=0;j<size_b+size_newb;++j){
            Vars[i][j] = solver.newVar();
        }
    }
    vector<Var> var_col(size_a,0);
    vector<Var> row(size_b+size_newb,0);
    vector<Var> col(size_a+1,0);
    vector<bool> row_bool(size_b+size_newb,true);
    vector<bool> col_bool(size_a,true);

//row sum==1
    for(int i=0;i<size_a;++i){
        for(int j=0;j<size_b+size_newb;++j){
            row[j]=solver.newVar();
            fill(row_bool.begin(),row_bool.end(),true);
            row_bool[j]=false;
            solver.addAndCNF(row[j],Vars[i],row_bool);
        }
        fill(row_bool.begin(),row_bool.end(),false);
        solver.addClause(row,row_bool);
    }
// column sum<=1
    for(int j=0;j<size_b+size_newb;++j){
        for(int i=0;i<size_a;i++){
            var_col[i]=Vars[i][j];
        }
        for(int i=0;i<size_a+1;++i){
            col[i]=solver.newVar();
            fill(col_bool.begin(),col_bool.end(),true);
            if(i!=size_a){
                col_bool[i]=false;
            }
            solver.addAndCNF(col[i], var_col, col_bool);
        }

        fill(col_bool.begin(),col_bool.end(),false);
        col_bool.push_back(false);
        solver.addClause(col, col_bool);
        col_bool.pop_back();
    }
//new b sum>=1
    if(size_newb!=0){
        vector<Var> newbs(size_a*size_newb,0);
        vector<bool> newbs_bool(size_a*size_newb, false);
        for(int i=0;i<size_a;++i){
            for(int j=size_b;j<size_b+size_newb;++j){
                newbs.push_back(Vars[i][j]);
            }
        }
        solver.addClause(newbs, newbs_bool);
    }
    int ans=solver.solve();
    solver.printStats();
    cout << (ans? "SAT" : "UNSAT") << endl;
    if (ans) {
        for(int i=0;i<size_a;++i){
            for(int j=0;j<size_b+size_newb;++j){
                cout << solver.getValue(Vars[i][j]) << endl;
            }
        }
    }

    return ans;
}

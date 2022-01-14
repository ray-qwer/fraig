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

extern CirMgr *original;
extern CirMgr *golden;
extern TwoCirFECG* _FECgroups;
void addCNF(SatSolver&, CirGate*);
void build_up_circuot_cnf(SatSolver&);

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
    while(true) {
        int ans=solver.assumpSolve();
        solver.printStats();
        cout << (ans? "SAT" : "UNSAT") << endl;
        if (ans) {
            //build up circuit
            SatSolver solver2;
            solver2.initialize();
            for(int i=0;i<size_a;i++){
                addCNF(solver2,a_list[i]);
            }

            for(int i=0;i<size_b;i++){
                addCNF(solver2,b_list[i]);
            }

            for(int i=0;i<original->_polist.size();i++){
                Var newV;
                bool flag1, flag2;
                if(original->_polist[i]->get_sat_var()==0){
                    newV=solver2.newVar();
                    original->_polist[i]->set_sat_var(newV);
                }
                if(golden->_polist[i]->get_sat_var()==0){
                    newV=solver2.newVar();
                    golden->_polist[i]->set_sat_var(newV);
                }
                Var f=solver2.newVar();
                flag1=golden->_polist[i]->get_fanin()[0].get_inv();
                flag2=golden->_polist[i]->get_fanin()[1].get_inv();
                solver2.addXorCNF(f,original->_polist[i]->get_sat_var(),flag1,golden->_polist[i]->get_sat_var(),flag2);
            }
            // xor a&b inputs
            vector<vector<Var> > Vars2(size_a,vector<Var>(size_b+size_newb,0));
            for(int i=0;i<size_a;++i){
                for(int j=0;j<size_b+size_newb;++j){
                    Vars2[i][j] = solver2.newVar();
                    solver2.addXorCNF(Vars2[i][j],a_list[i]->get_sat_var(),0,b_list[j]->get_sat_var(),0);
                }
            }
            bool ans2 = solver2.solve();
            if(ans2){
                // successful
                for(int i=0;i<size_a;++i){
                    for(int j=0;j<size_b+size_newb;++j){
                        if(solver.getValue(Vars[i][j]) == 1) {
                            cout<<"i: "<<i<<", j: "<<j<<endl;
                        }
                    }
                }
                break;
            }
            else {
                // put all Var = true in solver into tmp
                vector<Var> tmp;
                for(int i=0;i<size_a;++i){
                    for(int j=0;j<size_b+size_newb;++j){
                        if(solver.getValue(Vars[i][j]) == 1) {
                            tmp.push_back(Vars[i][j]);
                        }
                    }
                }
                vector<bool> tmp_bool(tmp.size(), true);
                solver.addClause(tmp, tmp_bool);
            }
        }
        // failed
        else break;

    }
    return 1;
}

void addCNF(SatSolver& s, CirGate* g)
{
    Var newV;
    for(int i=0;i<g->get_fanout().size();i++){
    
        if(g->get_fanout()[i].get_gate()->get_sat_var()==0){
            newV=s.newVar();
            g->get_fanout()[i].get_gate()->set_sat_var(newV);
        }
        bool flag1=0,flag2=0;
        if(g->getType()==PO_GATE){return;}
        else{
            if(g->get_fanout()[i].get_gate()->get_fanin()[0].get_gate()->get_sat_var()==0){
                newV=s.newVar();
                g->get_fanout()[i].get_gate()->get_fanin()[0].get_gate()->set_sat_var(newV);
            }
            if(g->get_fanout()[i].get_gate()->get_fanin()[1].get_gate()->get_sat_var()==0){
                newV=s.newVar();
                g->get_fanout()[i].get_gate()->get_fanin()[1].get_gate()->set_sat_var(newV);
            }
            flag1=g->get_fanout()[i].get_gate()->get_fanin()[0].get_inv();
            flag2=g->get_fanout()[i].get_gate()->get_fanin()[1].get_inv();
            s.addAigCNF(g->get_fanout()[i].get_gate()->get_sat_var(), g->get_fanout()[i].get_gate()->get_fanin()[0].get_gate()->get_sat_var(), flag1, g->get_fanout()[i].get_gate()->get_fanin()[1].get_gate()->get_sat_var(), flag2);
        }
        addCNF(s,g->get_fanout()[i].get_gate());
    }
}

void CutFinding(vector<CirGate*>& aList, vector<CirGate*>& bList){
    // to get list and throw them to cut matching
    for(auto m = _FECgroups->_groups.begin(); m != _FECgroups->_groups.end(); m++ ){
        if (!(*m)->_o_pairs.empty() && !(*m)->_g_pairs.empty()){
            aList.insert(aList.end(), (*m)->_o_pairs.begin(), (*m)->_o_pairs.end());
            bList.insert(bList.end(), (*m)->_g_pairs.begin(), (*m)->_g_pairs.end());
        }
    }
}
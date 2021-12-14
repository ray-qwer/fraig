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

void CutMatching(vector<CirGate*>& a_list, vector<CirGate*>& b_list)
{
    SatSolver solver;
    solver.initialize();
    int size_a=a_list.size(),size_b=b_list.size();
    for(int i=0;i<size_a;++i){
        for(int j=0;j<size_b;++j){
            newV = solver.newVar();
            solver.addAigCNF()
        }
    }
    solver.addClause()
}

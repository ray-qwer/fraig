/****************************************************************************
  FileName     [ cirGate.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define class CirAigGate member functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdarg.h>
#include <cassert>
#include "cirGate.h"
#include "cirMgr.h"
#include "util.h"
#include <bits/stdc++.h> 

using namespace std;

// TODO: Keep "CirGate::reportGate()", "CirGate::reportFanin()" and
//       "CirGate::reportFanout()" for cir cmds. Feel free to define
//       your own variables and functions.

extern CirMgr *cirMgr;

/**************************************/
/*   class CirGate member functions   */
/**************************************/
unsigned CirGate::_globalRef = 0;

void
CirGate::reportGate() const
{
   cout <<"================================================================================"<< endl;
   string s = "= " + getTypeStr() + "(" + to_string(_var) + ")" \
      + (_symbo == "" ? "" : "\"" + _symbo + "\"") + ", line " + to_string(_lineNo);
   cout << s <<  endl;
   string s1 = "= FECs:";
   if(_fecpair==0||_fecpair->_pairs.size()==1) s+="";
   else{
      _fecpair->sorting();
      for(auto i :_fecpair->_pairs){
         string s2 = "";
         if(i == this)continue;
         else{
            s2 +=" ";
            if(is_inv(_sim,i->_sim)) s2+="!";
            s2+=to_string(i->getVar()); 
         }
         if(s1.length()+s2.length()<80)   s1 = s1+s2;
         else{
            cout<<s1<<endl;
            s1 = s2.substr(1);
         }
      }
   }
   cout<<s1<<endl;
   s = "= Value: ";
   size_t si = _sim;
   for(size_t i =0 ; i< sizeof(size_t)*8;i++){
      int a = (_sim>>(sizeof(size_t)*8-i-1))%2;
      s += a==0?"0":"1";
      if(i%8==7&&i!=63)  s+="_";
   }
   cout<<s<<endl;
   cout << "================================================================================"<< endl;
}

void
CirGate::reportFanin(int level)
{
   assert (level >= 0);
   setGlobalRef();
   _dfsFanin(this, 0, 0, level);
}

void
CirGate::_dfsFanin(CirGate* g, unsigned spaces, bool inv, int level) {
   for (size_t i = 0; i < spaces; ++i) cout << " ";
   if (inv) cout << "!";
   cout << g->getTypeStr() << " " << g->_var;
   // check if need to add (*)
   if (g->isGlobalRef() && level > 0 && !g->_fanin.empty()) {
      cout << " (*)" << endl; return;
   }
   else cout << endl;
   // dfs next level
   if (level == 0) return;
   g->setToGlobalRef();
   for (size_t i = 0; i < g->_fanin.size(); ++i) {
      _dfsFanin(g->_fanin[i]._gate, spaces + 2, g->_fanin[i]._inv, level - 1);
   }
}

void
CirGate::reportFanout(int level)
{
   assert (level >= 0);
   setGlobalRef();
   _dfsFanout(this, 0, 0, level);
}

void
CirGate::_dfsFanout(CirGate* g, unsigned spaces, bool inv, int level) {
   for (size_t i = 0; i < spaces; ++i) cout << " ";
   if (inv) cout << "!";
   cout << g->getTypeStr() << " " << g->_var;
   // check if need to add (*)
   if (g->isGlobalRef() && level > 0 && !g->_fanout.empty()) {
      cout << " (*)" << endl; return;
   }
   else cout << endl;
   // dfs next level
   if (level == 0) return;
   g->setToGlobalRef();
   for (size_t i = 0; i < g->_fanout.size(); ++i) {
      _dfsFanout(g->_fanout[i].gate(), spaces + 2, g->_fanout[i].inv(), level - 1);
   }
}

 void 
 CirGate::connect(map<unsigned, CirGate*>& gatelist) {
    for (size_t i = 0;i < _fanin.size(); ++i) {
      size_t inVar = (size_t)(void*)_fanin[i]._gate;
       if (gatelist.find(inVar) == gatelist.end()) {
          CirGate* floatGate = new CirGate(inVar, 0, UNDEF_GATE);
          gatelist[inVar] = floatGate;
       }
      // set _fanin and _fanout
      _fanin[i]._gate = gatelist[inVar];
      _fanin[i]._gate->_fanout.emplace_back(this, _fanin[i]._inv);
      // _fanin[i]._gate->_outv.push_back(_inv[i]);
    }
 }

void
CirGate::reset() {
   _fanin.clear();
   _fanout.clear();
   _fecpair = 0;
   // _inv.clear();
   // _outv.clear();
}

size_t
CirGate::simulate(){
   if(isGlobalRef())    return _sim;
   if(_fanin.empty())   {setToGlobalRef();return _sim;}
   _sim =SIZE_MAX;

   for(int i =0;i<_fanin.size();i++){
      if(_fanin[i]._inv) _sim &= ~_fanin[i]._gate->simulate();
      else               _sim &= _fanin[i]._gate->simulate();
   }
   setToGlobalRef();
   return _sim;
}

void
FECpair::sorting(){
   if(is_sort) return ;
   if(_pairs.size() == 1){
      is_sort = true;
      return ;
   }
   sort(_pairs.begin(),_pairs.end(),compare_CirGate);
   is_sort = true;
   return;
}

bool 
compare_CirGate(CirGate* a,CirGate* b){
   return a->getVar()<b->getVar();
}

void
FECgroups::sorting(){
   if(is_sort) return;
   for(auto i = _groups.begin();i!=_groups.end();++i) {
      if((*i)->_pairs.size()==1) {
         (*i)->_pairs[0]->set_FECpair(0);
         _groups.erase(i);
      }
      (*i)->sorting();
   }
   sort(_groups.begin(),_groups.end(),compare_sorted_FECpairs);
   is_sort = true;
}

bool
compare_sorted_FECpairs(FECpair* a,FECpair* b){
   return a->_pairs[0]->getVar()<b->_pairs[0]->getVar();
}
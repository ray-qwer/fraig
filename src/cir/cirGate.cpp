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

extern CirMgr *original;
extern CirMgr *golden;
TwoCirFECG _FECgroups;
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
   string s1 = "= Origin:";
   if (_fecpair==0) s+=" no fecpair";
   else if(_fecpair->get_o_pairs().size()==1) s+="";
   else{
      _fecpair->sorting(true);
      for(auto i :_fecpair->get_o_pairs()){
         string tmp = "";
         if(i == this)continue;
         else{
            tmp +=" ";
            if(is_inv(_sim,i->_sim)) tmp+="!";
            tmp+=to_string(i->getVar()); 
         }
         if(s1.length()+tmp.length()<80)   s1 = s1+tmp;
         else{
            cout<<s1<<endl;
            s1 = tmp.substr(1);
         }
      }
   }
   cout<<s1<<endl;
   string s2 = "= Golden:";
   if (_fecpair==0) s+=" no fecpair";
   else{
      _fecpair->sorting(false);
      for(auto i :_fecpair->get_g_pairs()){
         string tmp = "";
         if(i == this)continue;
         else{
            tmp +=" ";
            if(is_inv(_sim,i->_sim)) tmp+="!";
            tmp+=to_string(i->getVar()); 
         }
         if(s2.length()+tmp.length()<80)   s2 = s2+tmp;
         else{
            cout<<s2<<endl;
            s2 = tmp.substr(1);
         }
      }
   }
   cout<<s2<<endl;
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
TwoCirFECP::sorting(bool is_o){
   if (is_o){
      if(o_is_sort) return; 
      if(_o_pairs.size()==1){
         o_is_sort = true; return;
      }
      sort(_o_pairs.begin(),_o_pairs.end(),compare_CirGate);
      o_is_sort = true;
   } else{
      if(g_is_sort) return;
      if(_g_pairs.size()==1){
         g_is_sort = true; return;
      }
      sort(_g_pairs.begin(),_g_pairs.end(),compare_CirGate);
      g_is_sort = true;
   }
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
TwoCirFECG::sorting(){
   if (is_sort) return;
   for (auto i = _groups.begin();i!=_groups.end();++i){
      if ((*i)->_o_pairs.size()==1 || (*i)->_g_pairs.size()==1) {
         bool deleteGroups = false;
         if((*i)->_o_pairs.empty()){
            (*i)->_o_pairs[0]->set_FECpair(0);
            deleteGroups = true;
         }
         else if((*i)->_g_pairs.empty()){
            (*i)->_g_pairs[0]->set_FECpair(0);
            deleteGroups = true;
         }
         if (deleteGroups){
            i--;
            _groups.erase(i+1);
            continue;
         }
      }
      (*i)->sorting(true);
   }
   is_sort = true;
   return;
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
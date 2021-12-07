/****************************************************************************
  FileName     [ cirFraig.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir FRAIG functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2012-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
#include "cirMgr.h"
#include "cirGate.h"
#include "sat.h"
#include "myHashMap.h"
#include "util.h"
#include "unordered_map"

using namespace std;

// TODO: Please keep "CirMgr::strash()" and "CirMgr::fraig()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/*******************************************/
/*   Public member functions about fraig   */
/*******************************************/
// _floatList may be changed.
// _unusedList and _undefList won't be changed
void
CirMgr::strash()
{
  if(do_sim)  {
    cerr<<"Error: circuit has been simulated!! Do \"CIRFraig\" first!!"<<endl;
    return;
  }
  if(dostrash)  {
    cerr<<"Error: strash operation has already been performed!!"<<endl; 
    return ;
  }
  else dostrash = true;
  unordered_map<size_t,CirGate*> strash_table;
  //只輸入dfslist的值
  for(auto i = _dfslist.begin();i!=_dfslist.end();){
    CirGate* tmp = *i;
    if(tmp->getType()==AIG_GATE){}
    else  {++i;continue;}
    unordered_map<size_t,CirGate*>::iterator iter = strash_table.find(tmp->hash_key());
    if(iter == strash_table.end())  strash_table.insert(pair<size_t,CirGate*>(tmp->hash_key(),tmp));
    else{
      //replace 
      CirGate* keeping = iter->second;
      cout<<"strashing: "<<keeping->getVar()<<" merging "<<tmp->getVar()<<"..."<<endl;
      remove_from_fanin(tmp,keeping);
      remove_from_fanout(tmp,keeping,false);
      remove_from_aiglist(tmp);
      size_t del = tmp->getVar();
      tmp = 0;  
      i = _dfslist.erase(i);
      _gatelist.erase(del);
      continue;
    }
    ++i;
  }
  resetdfs();
}

void
CirMgr::fraig()
{
  do_sim = false;
  dostrash = false;
  //dfs
  for(auto i = _FECgroups._groups.begin();i!=_FECgroups._groups.end();i++){
    FECpair* tmp = *i;
    if(tmp->_pairs.size()==1){
      tmp->_pairs[0]->set_FECpair(0);
    }
    else{
      tmp->sorting();
      for(size_t n = 1;n<tmp->_pairs.size();n++){
        cout<<"fraig: "<<tmp->_pairs[0]->getVar()<<" merging "<<tmp->_pairs[n]->getVar()<<endl;
        remove_from_fanin(tmp->_pairs[n],tmp->_pairs[0]);
        if(is_inv(tmp->_pairs[0]->_sim,tmp->_pairs[n]->_sim))
          remove_from_fanout(tmp->_pairs[n],tmp->_pairs[0],true);
        else
          remove_from_fanout(tmp->_pairs[n],tmp->_pairs[0],false);
        remove_from_aiglist(tmp->_pairs[n]);
        _gatelist.erase(tmp->_pairs[n]->getVar());          
      }
      tmp->_pairs[0]->set_FECpair(0);
    }
  }
  _FECgroups.reset();
  Const0->set_FECpair(0);
  resetdfs();
}

/********************************************/
/*   Private member functions about fraig   */
/********************************************/

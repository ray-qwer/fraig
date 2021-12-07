/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir optimization functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"
#include "cirDef.h"

using namespace std;

// TODO: Please keep "CirMgr::sweep()" and "CirMgr::optimize()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/**************************************************/
/*   Public member functions about optimization   */
/**************************************************/
// Remove unused gates
// DFS list should NOT be changed
// UNDEF, float and unused list may be changed
void
CirMgr::sweep()
{
  dostrash = false;
  if(_dfslist.empty())  genDFSList();
  //vector<CirGate*> _removelist;
  map<unsigned,CirGate*>::iterator _gateend = _gatelist.end();
  vector<CirGate*>::iterator _dend = _dfslist.end();
  vector<CirGate*>::iterator _dbegin = _dfslist.begin();
  map<unsigned,CirGate*>::iterator _gatetest = _gatelist.begin();
  vector<CirGate*>::iterator _dfstest;
  while(_gatetest!=_gateend){
    CirGate* tmp = _gatetest->second;
    GateType tmp_type = tmp->getType();
    if(tmp_type==AIG_GATE||tmp_type == UNDEF_GATE){}
    else  {++_gatetest;continue;}
    if(!tmp->_inDFSlist){
      cout<<tmp->getVar()<<" is going to be deleted..."<<endl;
      _removeGate(tmp);//remove it from other gate
      _gatetest = _gatelist.erase(_gatetest);
    }  //need to be deleted, and need to delete it from fanin and fanout
    else  ++_gatetest;
  }
  
}

void
CirMgr::_removeGate(CirGate* _remove)//let the element in the vector be zero and then erase it
{
  //fanin
  remove_from_fanin(_remove,0);
  //fanout
  remove_from_fanout(_remove,0,false);
  if(_remove->isAig()){
    remove_from_aiglist(_remove);
  }
}
// Recursively simplifying from POs;
// _dfsList needs to be reconstructed afterwards
// UNDEF gates may be delete if its fanout becomes empty...
void
CirMgr::optimize()//from po
{
  dostrash = false;
  if(do_sim)  {
    cerr<<"Error: circuit has been simulated!! Do \"CIRFraig\" first!!"<<endl;
    return;
  }
  if(_dfslist.empty())  genDFSList();
  //from the origin 
  vector<CirGate*>::iterator i = _dfslist.begin();
  vector<CirGate*>::iterator iend = _dfslist.end();
  for(;i!=iend;){
    CirGate* tmp = *i;
    if(tmp->getType()==AIG_GATE){}//no other gates??
    else  {++i;continue;}
    //just has two legs->fanin = 2
    cmp_type cmp = compare(tmp);
    if(combine(tmp,cmp)){
      int id = tmp->getVar();
      remove_from_aiglist(tmp);
      tmp = 0;
      i = _dfslist.erase(i);
      _gatelist.erase(id);
    }
    else  ++i;
  }
  resetdfs();  
}

/***************************************************/
/*   Private member functions about optimization   */
/***************************************************/
cmp_type 
CirMgr::compare(CirGate* _remove)
{
  assert(_remove->_fanin.size()==2);
  CirGateV f_leg = _remove->_fanin[0];
  CirGateV s_leg = _remove->_fanin[1];
  if(f_leg.gate()==Const0){
    if(f_leg.inv()) return FIRST_CONST1;
    else            return CONST0;
  }
  if(s_leg.gate()==Const0){
    if(s_leg.inv()) return SECOND_CONST1;
    else            return CONST0;
  }
  if(f_leg.gate()==s_leg.gate()){
    if(f_leg.inv()==s_leg.inv())  return SAME_ORG;
    else                          return OPP_ORG;
  }
  else                            return TWO_DIF;
}

bool 
CirMgr::combine(CirGate* _replace,cmp_type cmp)
{
  if(cmp == TWO_DIF)  return false;
  cout<<"Simplifying: ";
  if(cmp == CONST0||cmp == OPP_ORG){
    cout<<"0"<<" merging "<<_replace->getVar()<<endl;
    remove_from_fanin(_replace,Const0);
    remove_from_fanout(_replace,Const0,false);
  }
  else if(cmp==SAME_ORG||cmp==SECOND_CONST1){
    cout<<_replace->_fanin[0].gate()->getVar()<<" merging "<<(_replace->_fanin[0].inv() ? "!" : "")<<_replace->getVar()<<endl;
    remove_from_fanin(_replace,_replace->_fanin[0].gate());
    remove_from_fanout(_replace,_replace->_fanin[0].gate(),_replace->_fanin[0].inv());
  }
  else if(cmp == FIRST_CONST1){
    cout<<_replace->_fanin[1].gate()->getVar()<<" merging "<<(_replace->_fanin[1].inv() ? "!" : "")<<_replace->getVar()<<endl;
    remove_from_fanin(_replace,_replace->_fanin[1].gate());
    remove_from_fanout(_replace,_replace->_fanin[1].gate(),_replace->_fanin[1].inv());
  }
  return true;
}

void
CirMgr::remove_from_fanin(CirGate* _remove,CirGate* comb=0){
  for(auto i = _remove->_fanin.begin();i!=_remove->_fanin.end();++i){
    CirGateV& tmp = *i;
    auto ii = tmp.gate()->_fanout.begin();
    auto iend = tmp.gate()->_fanout.end();
    for(;ii!=iend;++ii){
      CirGateV& comp = *ii;
      if(comp.gate()==_remove){
        comp.set_gate(0);
        tmp.gate()->_fanout.erase(ii);
        break;
      }
    }
  }
  if(comb==0)  return;
  else{
    comb->_fanout.insert(comb->_fanout.end(),_remove->_fanout.begin(),_remove->_fanout.end());
  }
}
void
CirMgr::remove_from_fanout(CirGate* _remove,CirGate* comb = 0,bool inv = false){
  for(auto i = _remove->_fanout.begin();i!=_remove->_fanout.end();++i){
    CirGateV& tmp = *i;
    auto ii = tmp.gate()->_fanin.begin();
    auto iend = tmp.gate()->_fanin.end();
    for(;ii!=iend;++ii){
      CirGateV& comp =*ii;
      if(comp.gate()==_remove){
        comp.set_gate(comb);
        comp.set_inv(inv);
        if(comb==0) ii=tmp.gate()->_fanin.erase(ii);
      }
    }
  }
}
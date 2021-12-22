/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir simulation functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <fstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cassert>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"
#include "unordered_map"
#include <map>
#include <time.h>

extern CirMgr* original;
extern CirMgr* golden;

using namespace std;

// TODO: Keep "CirMgr::randimSim()" and "CirMgr::fileSim()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/************************************************/
/*   Public member functions about Simulation   */
/************************************************/
void
CirMgr::randomSim()
{ 
  do_sim =true;
  srand(time(NULL));
  size_t num_of_pairs = _FECgroups._groups.size();
  int pairs_count = 0;
  size_t pattern = 0;
  vector<size_t> sim_sizet;
  sim_sizet.reserve(_pilist.size());
  size_t test_times = (2>_pilist.size()/64)?2:_pilist.size()/16;
  while(pairs_count<test_times){
    sim_sizet.resize(_pilist.size(),0);
    for(int i =  0 ;i<_pilist.size();i++){
      int a = rand();
      int b = rand();
      size_t sim = (((size_t)(unsigned)a)<<31)+(size_t)(unsigned)b;
      sim_sizet[i] = sim;
    }
    pattern += 64;
    simulate(sim_sizet);
    if(_simLog!=0)  logout(sim_sizet,pattern%64); 
    if(_FECgroups._groups.size()==num_of_pairs)
      pairs_count++;
    else{
      pairs_count = 0;
      num_of_pairs = _FECgroups._groups.size();
    }
  }
  cout<<pattern<<" patterns simulated."<<endl;
}

void
CirMgr::fileSim(ifstream& patternFile)
{
  //處理data?
  //先處理string？
  //vector<size_t>* _input_data = new vector<size_t>[_pilist.size()];
  do_sim = true;
  if(!patternFile.is_open()){cerr<<"Cannot open file!!"<<endl;return;}
  vector<size_t> sim_sizet ;
  sim_sizet.reserve(_pilist.size());
  size_t pattern = 0;
  while(patternFile){
    sim_sizet.resize(_pilist.size(),0);
    for(size_t i  =0;i<sizeof(size_t)*8;i++){
      string line;
      if(!getline(patternFile,line))  break;
      //空白行要繼續
      //if(empty_line(line))  {--i;continue;}
      if(line[0]=='\0'||line[0]=='\n') continue;
      if(line.length()!=_pilist.size()) {cerr<<"Error: numbers of variable at simulation("<<line.length()<<") not equal to numbers of PI("<<_pilist.size()<<")"<<endl;return;}
      if(line.length() == 0)  break;
      for(size_t j =0;j<_pilist.size();j++){
        size_t tmp =line[j]-'0';
        if(tmp!=0&&tmp!=1){
          cerr<<"Error:illegal character("<<line[j]<<")"<<endl;
          return ;
        }
        sim_sizet[j] = (sim_sizet[j]<<1)|tmp;
      }
      pattern++;
    }
    simulate(sim_sizet);
    if(_simLog!=0)  logout(sim_sizet,pattern%64); 
  }
  cout<<pattern<<" patterns simulated."<<endl;
}

/*************************************************/
/*   Private member functions about Simulation   */
/*************************************************/

void
CirMgr::simulate(vector<size_t>& sim)
{
  
  for(size_t i = 0;i<_pilist.size();i++){
    CirPiGate* tmp = _pilist[i];
    tmp->set_sim(sim[i]);
  }
  Const0->setGlobalRef();
  //all gates simulate
  size_t tmp;
  for(size_t i=0;i<_polist.size();i++)  tmp = _polist[i]->simulate();
  classify();
  // two circuit classify, how??
}
void 
CirMgr::classify(){
  if(_FECgroups.is_first()){
    if(!Const0->_inDFSlist) _dfslist.push_back(Const0);
    class_by_hash(_dfslist);
    _FECgroups.set_first_time(false);
    auto i = _dfslist.end();
    --i;
    *i = 0; _dfslist.pop_back();
  }
  else{
    for(size_t m  = 0;m<_FECgroups._groups.size();m++){
      if(_FECgroups._groups[m]->_o_pairs.size()>100){
        //class_by_hash
        class_by_hash(_FECgroups._groups[m]->_o_pairs);
        _FECgroups._groups.erase(_FECgroups._groups.begin()+m);
      }
      else if(_FECgroups._groups[m]->_o_pairs.size()==1){
        continue;
      }
      else{
        //class_by_map
        class_by_map(_FECgroups._groups[m]->_o_pairs);
        _FECgroups._groups.erase(_FECgroups._groups.begin()+m);
      }
    }
  }
};
void
CirMgr::class_by_hash(vector<CirGate*>& list){
  unordered_map<size_t,TwoCirFECP*> Hash;
  for(size_t i =  0;i<list.size();i++){
    if(list[i]->getType()==PI_GATE||list[i]->getType()==PO_GATE)  continue;
    auto m = Hash.find(list[i]->_sim);
    auto n = Hash.find(~list[i]->_sim);
    if(m==Hash.end()&&n==Hash.end()){
      TwoCirFECP* tmp_pair = new TwoCirFECP;
      tmp_pair->append(list[i],is_origin);
      Hash.insert(pair<size_t,TwoCirFECP*>(list[i]->_sim,tmp_pair));
      _FECgroups.append(tmp_pair);
      list[i]->set_FECpair(tmp_pair);
    }
    else if(m!=Hash.end())  {
      m->second->append(list[i],is_origin);
      list[i]->set_FECpair(m->second);
    }
    else{
      n->second->append(list[i],is_origin);
      list[i]->set_FECpair(n->second);
    }    
  }
}
void 
CirMgr::class_by_map(vector<CirGate*>& list){
  map<size_t,TwoCirFECP*> the_map;
  for(size_t i =0;i<list.size();i++){
    auto m  = the_map.find(list[i]->_sim);
    auto n = the_map.find(~list[i]->_sim);
    if(m==the_map.end()&&n==the_map.end()){
      TwoCirFECP* tmp_pair = new TwoCirFECP;
      tmp_pair->append(list[i],is_origin);
      the_map[list[i]->_sim] = tmp_pair;
      _FECgroups.append(tmp_pair);
      list[i]->set_FECpair(tmp_pair);
    }
    else if(m!=the_map.end()) {
      m->second->append(list[i],is_origin);
      list[i]->set_FECpair(m->second);
    }
    else {
      n->second->append(list[i],is_origin);
      list[i]->set_FECpair(n->second);
    }
  }
}
void
CirMgr::logout(vector<size_t>& sim,size_t pattern){
  if(pattern == 0)  pattern = sizeof(size_t)*8;
  for(size_t k= 0;k<pattern;k++){
    for(size_t input = 0;input<_pilist.size();input++){
      int a  = (sim[input]>>(pattern-k-1))%2;
      char c = a==0?'0':'1';
      _simLog->put(c);
    }
    _simLog->put(' ');
    cout<<" ";
    for(size_t output = 0;output<_polist.size();output++){
      int a = (((_polist[output]->simulate())>>(pattern-k-1))%2);
      char c = a==0?'0':'1';
      _simLog->put(c);
    }
    _simLog->put('\n');
  }
}
// TODO:
// let _FECgroups as an external global varible, just like cirMgr
void
CirMgr::class_by_hash(vector<CirGate*>& list,unordered_map<size_t,TwoCirFECP*>& Hash){
  for (size_t i=0;i<list.size();i++){
    auto m = Hash.find(list[i]->_sim);
    auto n = Hash.find(~list[i]->_sim);
    if ( m==Hash.end() && n==Hash.end() ){
      TwoCirFECP* tmp_pair = new TwoCirFECP;
      tmp_pair->append(list[i],is_origin);
      Hash.insert(pair<size_t,TwoCirFECP*>(list[i]->_sim,tmp_pair));
      _FECgroups.append(tmp_pair);
      list[i]->set_FECpair(tmp_pair);
    }
    else if(m!=Hash.end())  {
      m->second->append(list[i],is_origin);
      list[i]->set_FECpair(m->second);
    }
    else{
      n->second->append(list[i],is_origin);
      list[i]->set_FECpair(n->second);
    }    
  }
  return;
}
void 
CirMgr::class_by_map(vector<CirGate*>& list,map<size_t,TwoCirFECP*>& Map){
  for (size_t i = 0;i<list.size();i++){
    auto m = Map.find(list[i]->_sim);
    auto n = Map.find(~list[i]->_sim);
    if ( m == Map.end() && n == Map.end() ){
      TwoCirFECP* tmp_pair = new TwoCirFECP;
      tmp_pair->append(list[i],is_origin);
      Map[list[i]->_sim] = tmp_pair;
      _FECgroups.append(tmp_pair);
      list[i]->set_FECpair(tmp_pair);
    }
    else if ( m != Map.end() ){
      m->second->append(list[i],is_origin);
      list[i]->set_FECpair(m->second);
    }
    else {
      n->second->append(list[i],is_origin);
      list[i]->set_FECpair(n->second);
    }
  }
  return;
}

bool simTwoCir(bool doRandom, ofstream* simLog, ifstream* patternFile){
   // check if all input are the same
   // if golden is less than original, add some redundant inputs at golden
   // if more, just random simulate them

   // do compare
  if (simLog != 0){
    original->setSimLog(simLog);
    golden->setSimLog(simLog);
  }
  if (doRandom){
    // randomSim
    size_t input_size = (golden->_pilist.size()>original->_pilist.size())?golden->_pilist.size():original->_pilist.size();
    srand(time(NULL));
    vector<size_t> sim_sizet;
    sim_sizet.reserve(input_size);
    for (int i = 0;i<input_size;i++){
      int a = rand();
      int b = rand();
      size_t sim = (((size_t)(unsigned)a) << 31) | (size_t)(unsigned)b;
      sim_sizet[i] = sim;
    }
    golden->simulate(sim_sizet);
    original->simulate(sim_sizet);
    golden->do_sim = true; original->do_sim = true;
  } else {
    // file
    size_t input_size = (golden->_pilist.size()>original->_pilist.size())?golden->_pilist.size():original->_pilist.size();
    if (!patternFile->is_open()) { cerr<<"Cannot open file!!"<<endl; return false;}
    vector<size_t> sim_sizet;
    sim_sizet.reserve(input_size);
    size_t pattern = 0;
    while(patternFile){
      sim_sizet.resize(input_size,0);
      for (size_t i = 0;i<sizeof(size_t)*8;i++){
        string line;
        if(!getline(*patternFile,line)) break;
        // if next line is space or nextline, continue
        if(line[0]=='\0' || line[0]=='\n') continue;
        if (line.length()==0) break;
        //
        for (size_t j = 0;j<original->_pilist.size();j++){
          size_t tmp = line[j]-'0';
          if(tmp>1 || tmp < 0){
            cerr<<"Error: illegal character("<<line[j]<<")"<<endl;
            return false;
          }
          sim_sizet[j] = sim_sizet[j]<<1 | tmp;
        }
        pattern ++ ;      
      }
      if(input_size > original->_pilist.size()){
        for (size_t j = input_size;j<original->_pilist.size();j++ ){
          srand(time(NULL));
          int a = rand(); int b = rand();
          size_t sim = (((size_t)(unsigned)a) << 31) | (size_t)(unsigned)b;
          // pattern % sizeof(size_t)*8: remain
          if (pattern % sizeof(size_t)*8 == 0 ) sim = sim << (sizeof(size_t)*8-(pattern % sizeof(size_t)*8));
          sim_sizet[j] = sim;
        }
      }
      original->simulate(sim_sizet);
      golden->simulate(sim_sizet);
    }
    cout<<pattern<<" patterns simulated."<<endl;
  }
  return true;
}
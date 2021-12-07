/****************************************************************************
  FileName     [ cirMgr.h ]
  PackageName  [ cir ]
  Synopsis     [ Define circuit manager ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_MGR_H
#define CIR_MGR_H

#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <map>

using namespace std;

#include "cirGate.h"
#include "cirDef.h"

extern CirMgr *cirMgr;


// TODO: Define your own data members and member functions
class CirMgr
{
public:
  CirMgr(): _M(0), _I(0), _L(0), _O(0), _A(0), _doComment(0), _comment(""), _type("") {}
  ~CirMgr() { reset(); }

  // Access functions
  // return '0' if "gid" corresponds to an undefined gate.
  CirGate* getGate(unsigned gid) const {
    if (_gatelist.find(gid) == _gatelist.end()) return 0;
    return _gatelist.find(gid)->second;
  }

  // Member functions about circuit construction
  bool readCircuit(const string&);

  // Member functions about circuit reporting
  void printSummary() const;
  void printNetlist() const;
  void printPIs() const;
  void printPOs() const;
  void printFloatGates() const;
  void writeAag(ostream&) const;
  void writeGate(ostream&,CirGate*)const;
  void printFECPairs();
  void sweep();
  void optimize();
  void randomSim();
  void fileSim(ifstream&);
  void strash();
  void fraig();
  void setSimLog(ofstream* m){_simLog = m;}

  // Member functions about circuit DFS
  void genDFSList();

  // CONST 0 gate
  static CirGate* Const0;
  
  // reseting
  void reset();

private:
  //friend function
  friend void CirGate::connect(map<unsigned,CirGate*>&);
  // for parsing
  bool _readInitial(fstream&);
  bool _readPI(fstream&);
  bool _readPO(fstream&);
  bool _readAIG(fstream&);
  // bool _readSymbI(int , const string&);
  // bool _readSymbO(int, const string&);
  bool _readSymb(fstream&);

  bool _notSpace(char);
  bool _beSpace(char);
  bool _readNum(string&, int&, string);

  void _buildConnect();
  void _removeGate(CirGate*);
  bool combine(CirGate*,cmp_type);
  cmp_type compare(CirGate*);
  //remove but not delete 
  void remove_from_fanin(CirGate*,CirGate*);
  void remove_from_fanout(CirGate*,CirGate*,bool);
  void remove_from_aiglist(CirGate*);

  void resetdfs(){
    _dfslist.clear();
    for(size_t i =  0 ; i < _pilist.size(); i++)  _pilist[i]->_inDFSlist = false;
    genDFSList();  
  }
  void simulate(vector<size_t>&);
  void class_by_hash(vector<CirGate*>&);
  void class_by_map(vector<CirGate*>&);
  void _dfs(CirGate*);
  void g_dfs(CirGate*,vector<CirGate*>&)const;
  void logout(vector<size_t>&,size_t);

  bool _doComment;
  string _comment;
  string _type;
  bool dostrash = false;
  bool do_sim = false;
  
  
  int _M, _I, _L, _O, _A;
  vector<CirPiGate*> _pilist;
  vector<CirPoGate*> _polist;
  vector<CirAigGate*> _aiglist;
  vector<CirGate*> _dfslist;
  map<unsigned, CirGate*> _gatelist;
  ofstream *_simLog;
  FECgroups _FECgroups;
};
bool compare_gate(CirGate*,CirGate*);
bool is_inv(size_t,size_t);
#endif // CIR_MGR_H

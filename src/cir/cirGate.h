/****************************************************************************
  FileName     [ cirGate.h ]
  PackageName  [ cir ]
  Synopsis     [ Define basic gate data structures ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_GATE_H
#define CIR_GATE_H

#include <string>
#include <vector>
#include <iostream>
#include <map>
#include "cirDef.h"
#include "sat.h"


using namespace std;

//------------------------------------------------------------------------
//   Define classes
//------------------------------------------------------------------------
bool compare_CirGate(CirGate*,CirGate*);
class FECpair
{
  public:
    FECpair(){}
    ~FECpair(){ _pairs.clear();}
    void append(CirGate* t){_pairs.emplace_back(t);is_sort = false;}
    void print(bool);
    void sorting();
  //member
    vector<CirGate*>  _pairs;
    bool is_sort =  false;
};
bool compare_sorted_FECpairs(FECpair*,FECpair*);
class FECgroups
{ 
  public:
    FECgroups(){}
    ~FECgroups(){_groups.clear();}
    bool is_first(){return first_time;}
    vector<FECpair*> _groups;
    void set_first_time(bool b ){first_time =b ;}
    void append(FECpair* t){_groups.emplace_back(t);is_sort = false;}
    void sorting();
    void reset(){
      _groups.clear();
      first_time = true;
      is_sort = false;
    }
  private:
    bool first_time = true;
    bool is_sort = false;
};

class CirGate;
class CirPiGate;
class CirPoGate;
class CirAigGate;
class CirGateV;
class CirGateV {
  public:
  friend class CirGate;
  friend class CirPiGate;
  friend class CirPoGate;
  friend class CirAigGate;

  CirGateV() {}
  CirGateV(CirGate* g, bool inv): _gate(g), _inv(inv) {}
  CirGate* gate() const { return _gate; }
  bool inv() const { return _inv; }
  void set_gate(CirGate* tmp){_gate = tmp;}
  void set_inv(bool m){if(m==true&&_inv==true)_inv=false;
  else if(m==false&&_inv==false)_inv = false;
  else _inv = true;}
  CirGateV operator = (CirGateV m){_gate = m._gate;_inv = m._inv;}

  protected:
  CirGate* _gate;
  bool _inv;
};

class CirGate
{
public:
  CirGate() {}
  CirGate(int var, int lineNo, GateType gateType) {
    _var = var; _lineNo = lineNo; _gateType = gateType;
    _inDFSlist = false;
  }
  virtual ~CirGate() { reset(); }
  //operator overloading
  size_t hash_key(){
    size_t k = 0;
    for(size_t i =0;i<_fanin.size();i++){
      k ^= ((size_t)_fanin[i]._gate<<_fanin[i]._inv);
    }
    return k;
  }
  //friend function
  friend class CirMgr;
  friend class CirGateV;
  // Basic access methods
  string getTypeStr() const {
    switch(_gateType) {
      case UNDEF_GATE: return "UNDEF";
      case PI_GATE: return "PI";
      case PO_GATE: return "PO";
      case AIG_GATE: return "AIG";
      case CONST_GATE: return "CONST";
      case TOT_GATE: return "TOT_GATE";
      default: return "";
    }
  }
  unsigned getLineNo() const { return _lineNo; }
  virtual bool isAig() const { return false; }
  virtual size_t simulate();
  // Printing functions
  // virtual void printGate() const = 0;
  void printGate() {} // this is I add
  void reportGate() const;
  void reportFanin(int level);
  void reportFanout(int level);
  void set_FECpair(FECpair* g){
    _fecpair = g;
  }
  unsigned getVar() { return _var; }
  GateType getType() { return _gateType; }

  void connect(map<unsigned, CirGate*>&);

  void addSymbol(const string& symb) {
    _symbo = symb;
  }

  bool isGlobalRef() { return _ref == _globalRef; }
  void setToGlobalRef() { _ref = _globalRef; }
  static void setGlobalRef() { ++_globalRef; }
  void reset();

private:
  static unsigned _globalRef;
  unsigned _ref;
  void _dfsFanin(CirGate*, unsigned, bool, int);
  void _dfsFanout(CirGate*, unsigned, bool, int);

protected:
  size_t _sim = 0;
  GateType _gateType;
  unsigned _var;
  unsigned _lineNo;
  vector<CirGateV> _fanin;
  vector<CirGateV> _fanout;
  // vector<CirGate*> _fanin;
  // vector<CirGate*> _fanout;
  // vector<bool> _inv; // input inverse
  // vector<bool> _outv; // output inverse
  string _symbo;

  bool _inDFSlist;
  FECpair* _fecpair = 0;
  //pair<FECpair*,bool> _fecpair = pair<FECpair*,bool>(0,false);
};

class CirPiGate : public CirGate
{
public:
  CirPiGate() {}
  CirPiGate(int lit, unsigned lineNo) {
    _gateType = PI_GATE;
    _lineNo = lineNo;
    _var = lit / 2;
    _symbo = "";
    _inDFSlist = false;
  }
  ~CirPiGate() { reset(); }
  void set_sim(size_t s){_sim = s;}
  size_t simulate(){return _sim;}
};

class CirPoGate : public CirGate
{
public:
  CirPoGate() {}
  CirPoGate(int srclit, int var, unsigned lineNo) {
    _gateType = PO_GATE;
    _lineNo = lineNo;
    _var = var;
    _inDFSlist = false;

    // _inv.push_back(srclit % 2 == 1 ? 1 : 0);

    size_t srcVar = (size_t)(srclit / 2);
    // _fanin.push_back((CirGate*)srcVar);
    _fanin.emplace_back((CirGate*)srcVar, srclit % 2 == 1 ? 1 : 0);//(CirGate*)srcVar:use when it create  
    _symbo = "";
  }
  ~CirPoGate() { reset(); }
};

class CirAigGate : public CirGate
{
public:
  CirAigGate() {}
  CirAigGate(int lit, int src1, int src2, unsigned lineNo) {
    _gateType = AIG_GATE;
    _lineNo = lineNo;
    _var = lit / 2;
    _inDFSlist = false;

    // _inv.push_back(src1 % 2 == 1 ? 1 : 0);
    // _inv.push_back(src2 % 2 == 1 ? 1 : 0);
    
    size_t var1 = (size_t)(src1 / 2);
    size_t var2 = (size_t)(src2 / 2);
    // _fanin.push_back((CirGate*)var1);
    // _fanin.push_back((CirGate*)var2);
    _fanin.emplace_back((CirGate*)var1, src1 % 2 == 1 ? 1 : 0);
    _fanin.emplace_back((CirGate*)var2, src2 % 2 == 1 ? 1 : 0);
    _symbo = "";
  }
  ~CirAigGate() { reset(); }
  bool isAig()const{return true;}
};
//bool compare_sort(CirGate*,CirGate*);
#endif // CIR_GATE_H

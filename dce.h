#ifndef __DEC_H__
#define __DEC_H__

#include <cstdlib>
#include <list>
#include <set>
#include <vector>
#include <string>
#include <utility>
using namespace std;

enum OpType {REG, VAR, ADDR, POINTER, NONE};
// for POINTER OpType, register number 0 stands for GP, 1 for FP
extern set<int> br_target;
struct Function;

inline int instr_num(string instr) {
  int pos1 = instr.find("instr ")+6;
  int len = instr.find(":")-pos1;
  
  return atoi(instr.substr(pos1, len).c_str());
}

struct Exp {
  list<pair<OpType, int> > use;
  int opcode_num;
};

struct Instr {
  int num;
  list<pair<OpType, int> > use;
  list<pair<OpType, int> > def;

  int opcode_num;
  string opcode; 
  string instr;

  bool populate(string, bool&, set<Exp*>*);
};

struct BasicBlock {
  int num;
  
  set<int> use;  // always refers to C variables
  set<int> def;  // always refers to C variables
  set<int> live_list;  // always refers to C variables
  list<Instr*> instr;
  set<int> children; // indexed by bb number, used in population only
  set<BasicBlock*> children_p; // children pointers
  set<BasicBlock*> parent_p; // children pointers
  set<int> children_live; // live C variables of children

  int branch_target;
  bool main;

  // CFG
  bool populate(set<Exp*>*);

  // DCE
  void compute_defuse();
  bool dce(Function*, set<int>&);
  inline void add_instr_def(list<Instr*>::iterator);
  inline void add_instr_use(list<Instr*>::iterator);
};

struct Function {
  vector<BasicBlock*> bb;
  set<Exp*> base;
  set<int> dead_var_offset;

  BasicBlock* get_bb(int);

  // CFG
  void populate();
  void print_CFG();
  void print_instr();

  // DCE
  void compute_live();
  bool compute_bb_live(int);
  void dce();
  void reconnect();
  int next_instr_num(int);
};


pair<OpType, int> get_1op(string instr);
pair<OpType, int> get_2op(string instr);
#endif

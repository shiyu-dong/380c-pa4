#ifndef __DEC_H__
#define __DEC_H__

#include <cstdlib>
#include <list>
#include <set>
#include <vector>
#include <string>
#include <utility>
#include <map>
using namespace std;

#define PRE_OPCODE_RANGE 9 //(3+5+1)

// for POINTER OpType, register number 0 stands for GP, 1 for FP
extern set<int> br_target;
struct Function;

enum OpType {REG, VAR, ADDR, POINTER, CONSTANT, NONE};

inline int instr_num(string instr) {
  int pos1 = instr.find("instr ")+6;
  int len = instr.find(":")-pos1;
  
  return atoi(instr.substr(pos1, len).c_str());
}

struct Exp {
  // Exp include all arith instructions
  // emposing order of operands on these instructions
  int opcode_num;
  int instr_num;
  list<pair<OpType, int> > use;
  string exp;

  // Exp always have at least on operator
  inline bool operator<(const Exp& e) const {
    if (opcode_num != e.opcode_num)
      return opcode_num < e.opcode_num;
    else if (use.front().first != e.use.front().first)
      return use.front().first < e.use.front().first;
    else if (use.front().second != e.use.front().second)
      return use.front().second < e.use.front().second;
    else if (use.size() != e.use.size()) 
      return use.size() < e.use.size();
    else if (use.back().first != e.use.back().first)
      return use.back().first < e.use.back().first;
    else
      return use.back().second < e.use.back().second;
  }

  inline bool operator==(const Exp& e) const {
    if (opcode_num != e.opcode_num)
      return 0;
    else if (use.front().first != e.use.front().first)
      return 0;
    else if (use.front().second != e.use.front().second)
      return 0;
    else if (use.size() != e.use.size()) 
      return 0;
    else if (use.back().first != e.use.back().first)
      return 0;
    else
      return use.back().second == e.use.back().second;
  }

  Exp(int _opcode_num, int _instr_num, list<pair<OpType, int> > _use, string instr) :
    opcode_num(_opcode_num), instr_num(_instr_num), use(_use) {
      int found = instr.find(':');
      exp = instr.substr(found+2);
    }
};


struct Instr {
  int num;
  list<pair<OpType, int> > use;
  list<pair<OpType, int> > def;
  string instr; // used for printout

  // following three are for PRE only
  int opcode_num;
  string opcode; 

  bool populate(string, bool&);
};

struct BasicBlock {
  int num;
  bool main;
  int branch_target;
  set<int> children; // indexed by bb number, used in population only
  set<BasicBlock*> children_p; // children pointers
  set<BasicBlock*> parent_p; // children pointers
  list<Instr*> instr;
  
  set<int> use;  // always refers to C variables
  set<int> def;  // always refers to C variables
  set<int> live_list;  // always refers to C variables
  set<int> children_live; // live C variables of children

  set<Exp> UEE;
  set<Exp> DEE;
  set<Exp> KILL;

  set<Exp> AVAIL_IN;
  set<Exp> AVAIL_OUT;
  set<Exp> ANT_IN;
  set<Exp> ANT_OUT;

  set<Exp> LATER_IN;
  set<Exp> DELETE;

  set<Exp> fixed_exp;

  // temporary variable
  set<pair<OpType, int> > KILL_t;

  // CFG
  bool populate();

  // DCE
  void compute_defuse();
  bool dce(Function*, set<int>&);
  inline void add_instr_def(list<Instr*>::iterator);
  inline void add_instr_use(list<Instr*>::iterator);

  // PRE
  void compute_UEE(); // eliminate local redundancy
  void compute_DEE();
  void compute_KILL(set<Exp>*);
};

struct Edge {
  BasicBlock* parent;
  BasicBlock* child;

  set<Exp> EARLIEST;
  set<Exp> LATER;
  set<Exp> INSERT;
};

struct Function {
  vector<BasicBlock*> bb;
  map<pair<int, int>, Edge*> edge;
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

  // PRE
  set<Exp> base;

  void compute_UEE();
  void compute_DEE();
  void compute_KILL();
  void compute_base();
  void PRE_init();
  void compute_AVAIL();
  void compute_ANT();
  void compute_EARLIEST();
  void compute_LATER();
  void compute_INSERT();
  void compute_DELETE();
  void rewrite();

  int find_ref(BasicBlock*&, const Exp&, set<BasicBlock*>&);
  int find_ref_helper(BasicBlock* const &, const Exp&, set<BasicBlock*>&);

  void fix_up(BasicBlock*&, const Exp&, int, set<BasicBlock*>&);
  void fix_up_helper(BasicBlock* const &, const Exp&, int, set<BasicBlock*>&);


  void eliminate(BasicBlock*, BasicBlock*, Exp, int, set<BasicBlock*>&);

  set<Exp> Intersect(const set<Exp>*, const set<Exp>*);
  set<Exp> Union(const set<Exp>*, const set<Exp>*);
  set<Exp> Not(const set<Exp>*);
};


pair<OpType, int> get_1op(string instr);
pair<OpType, int> get_2op(string instr);

string itoa(int);
int atoi(string);
#endif

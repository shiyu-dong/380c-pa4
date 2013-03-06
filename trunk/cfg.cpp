#include <assert.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include "dce.h"

set<int> br_target;
bool main_next = 0;

string opcode[] = {"add", "mul", "cmpeq", "sub", "div", "mod", "cmple", "cmplt", "neg"};


//#define PRE_OPCODE_RANGE 9 //(3+5+1)
// type 0
// 1 reg def + 2 use (commutative)
#define DEF_REG0_SIZE 3
string def_reg0[] = {"add", "mul", "cmpeq"};
// type 1
// 1 reg def + 2 use
#define DEF_REG1_SIZE 5
string def_reg1[] = {"sub", "div", "mod", "cmple", "cmplt"};
// type 2
// 1 reg def + 1 use of the only op
#define DEF_REG2_SIZE 2
string def_reg2[] = {"neg", "load"};
// type 3
// 0 reg def + 2 use
#define DEF_REG3_SIZE 1
string def_reg3[] = {"store"};
// type 4
// 0 def + 1st use of two ops
// need to check BB boundary
#define DEF_REG4_SIZE 2
string def_reg4[] = {"blbc", "blbs"};
// type 5
// 1 def + 1 use, define in 2nd operand position
#define DEF_REG5_SIZE 1
string def_reg5[] = {"move"};
// type 6
// 0 def + 1 use of the only op
#define DEF_REG6_SIZE 2
string def_reg6[] = {"write", "param"};
// type 7, branches whose destination might be changed
// br, blbs, blbc
// type 8, ret, call, enter, entrypc, read, wrl, nop won't be deleted and depend on nothing
#define BB_END_SIZE 5
string bb_end[] = {"br", "blbc", "blbs", "ret", "call"};

inline int atoi(string s) {
  stringstream ss;
  ss << s;
  int ret;
  ss >> ret;
  return ret;
}

pair<OpType, int> get_1op(string instr) {
  int pos1, pos2, pos3;
  string op1str;
  pos2 = instr.find_last_of(" ");
  pos1 = instr.find_last_of(" ", pos2-1);
  op1str = instr.substr(pos1+1, pos2-pos1-1);
  // check if it is a reg
  pos1 = op1str.find('(');
  if (pos1 != string::npos) {
    int reg_num = atoi(op1str.substr(pos1+1, op1str.length()-1).c_str());
    return make_pair(REG, reg_num);
  }

  // check if it is a var
  pos1 = op1str.find("#");
  pos2 = op1str.find("_base");
  pos3 = op1str.find("_offset");
  if (pos1 != string::npos && pos2 == string::npos && pos3 == string::npos) {
    int reg_num = atoi(op1str.substr(pos1+1, op1str.length()-1).c_str());
    return make_pair(VAR, reg_num);
  }
  else if (pos1 != string::npos && (pos2 != string::npos || pos3 != string::npos)) {
    int reg_num = atoi(op1str.substr(pos1+1, op1str.length()-1).c_str());
    return make_pair(ADDR, reg_num);
  }

  pos1 = op1str.find("GP");
  pos2 = op1str.find("FP");
  if (pos1 != string::npos)
    return make_pair(POINTER, 0);
  else if (pos2 != string::npos)
    return make_pair(POINTER, 1);

  return make_pair(CONSTANT, atoi(op1str) );
}

pair<OpType, int> get_2op(string instr) {
  int pos1, pos2, pos3;
  string op1str;
  pos2 = instr.length();
  pos1 = instr.find_last_of(" ");
  op1str = instr.substr(pos1+1, pos2-pos1-1);
  // check if it is a reg
  pos1 = op1str.find('(');
  if (pos1 != string::npos) {
    int reg_num = atoi(op1str.substr(pos1+1, op1str.length()-1).c_str());
    return make_pair(REG, reg_num);
  }

  // check if it is a branch target
  pos1 = op1str.find('[');
  if (pos1 != string::npos) {
    int reg_num = atoi(op1str.substr(pos1+1, op1str.length()-1).c_str());
    return make_pair(NONE, reg_num);
  }

  // check if it is a var
  pos1 = op1str.find("#");
  pos2 = op1str.find("_base");
  pos3 = op1str.find("_offset");
  if (pos1 != string::npos && pos2 == string::npos && pos3 == string::npos) {
    int reg_num = atoi(op1str.substr(pos1+1, op1str.length()-1).c_str());
    return make_pair(VAR, reg_num);
  }
  else if (pos1 != string::npos && (pos2 != string::npos && pos3 != string::npos)) {
    int reg_num = atoi(op1str.substr(pos1+1, op1str.length()-1).c_str());
    return make_pair(ADDR, reg_num);
  }

  pos1 = op1str.find("GP");
  pos2 = op1str.find("FP");
  if (pos1 != string::npos)
    return make_pair(POINTER, 0);
  else if (pos2 != string::npos)
    return make_pair(POINTER, 1);

  return make_pair(CONSTANT, atoi(op1str) );
}

bool newfunc_reached() {
  string s;
  int found1, found2, found3;

  //peek line
  streampos sp = cin.tellg();
  ios::iostate st = cin.rdstate();
  getline(cin, s);

  found1 = s.find("nop");
  if (found1 != string::npos) {
    getline(cin, s);
    return 1;
  }

  cin.clear();
  cin.setstate(st);
  cin.seekg(sp);

  found1 = s.find("enter");
  found2 = s.find("nop");
  found3 = s.find("entrypc");

  if (found1 == string::npos && found2 == string::npos && found3 == string::npos)
    return 0;
  else
    return 1;
}

// return 0 if reach the end of basic block
// return 1 if there are instructions following
bool Instr::populate(string temp, bool& main) {
  int found;
  int opcode_count = 0;
  bool instr_follow;

  use.clear();
  def.clear();
  opcode_num = -1;

  // get instruction
  instr = temp;
  // check if it is entrypc
  found = instr.find("entrypc");
  if (found != string::npos) {
    main = 1;
    getline(cin, instr);
  }

  // get intruction number
  num = instr_num(instr);

  // check if this is the last instr in the bb
  instr_follow = (br_target.find(num+1) == br_target.end());

  // populate def and use
  // type 0, 1 def + 2 use
  for(int i=0; i<DEF_REG0_SIZE; i++) {
    found = instr.find(def_reg0[i]);
    if (found != std::string::npos) {
      def.push_back(make_pair(REG, num));
      use.push_back(get_1op(instr));
      pair<OpType, int> t = get_2op(instr);

      opcode = def_reg0[i];
      opcode_num = opcode_count;
      if (t.first < use.front().first)
        use.push_front(t);
      else if (t.first == use.front().first && t.second < use.front().second)
        use.push_front(t);
      else
        use.push_back(t);

      return instr_follow;
    }
    opcode_count++;
  }
  // type 1, 1 def + 2 use
  for(int i=0; i<DEF_REG1_SIZE; i++) {
    found = instr.find(def_reg1[i]);
    if (found != std::string::npos) {
      def.push_back(make_pair(REG, num));
      use.push_back(get_1op(instr));
      use.push_back(get_2op(instr));

      opcode = def_reg1[i];
      opcode_num = opcode_count;

      return instr_follow;
    }
    opcode_count++;
  }
  // type 2, 1 def + 1 use
  for(int i=0; i<DEF_REG2_SIZE; i++) {
    found = instr.find(def_reg2[i]);
    if (found != std::string::npos) {
      def.push_back(make_pair(REG, num));
      use.push_back(get_2op(instr));

      if (i == 0) {
        opcode = def_reg2[i];
        opcode_num = opcode_count;
      }
      return instr_follow;
    }
    opcode_count++;
  }
  // type 3, 0 def + 2 use
  for(int i=0; i<DEF_REG3_SIZE; i++) {
    found = instr.find(def_reg3[i]);
    if (found != std::string::npos) {
      use.push_back(get_1op(instr));
      use.push_back(get_2op(instr));
      return instr_follow;
    }
  }
  // type 5, 0 def + 1 use
  for(int i=0; i<DEF_REG5_SIZE; i++) {
    found = instr.find(def_reg5[i]);
    if (found != std::string::npos) {
      use.push_back(get_1op(instr));
      def.push_back(get_2op(instr));
      return instr_follow;
    }
  }
  // type 6, 0 def + 1 use
  for(int i=0; i<DEF_REG6_SIZE; i++) {
    found = instr.find(def_reg6[i]);
    if (found != std::string::npos) {
      use.push_back(get_2op(instr));
      return instr_follow;
    }
  }
  // type 4, 0 def + 1 use
  for(int i=0; i<DEF_REG4_SIZE; i++) {
    found = instr.find(def_reg4[i]);
    if (found != std::string::npos) {
      use.push_back(get_1op(instr));
    }
  }
  // check end of bb
  for(int i=0; i<BB_END_SIZE; i++) {
    found = instr.find(bb_end[i]);
    if (found != std::string::npos)
      return 0;
  }
  return instr_follow;
}

// return 0 if reach the end of the function
// return 1 if there are other bb following
bool BasicBlock::populate() {
  string temp;
  bool ret=1;
  main = main_next;
  main_next = 0;

  live_list.clear();
  children.clear();
  children_p.clear();
  parent_p.clear();
  branch_target = -1;

  // get a basic block
  while(ret && !cin.eof()) {
    getline(cin, temp);
    instr.push_back(new Instr);
    ret = instr.back()->populate(temp, main);
  }

  // update basic block number
  num = instr.front()->num;

  // check branch target and populate children
  string last_instr = instr.back()->instr;
  int last_instr_num = instr.back()->num;
  int found, found1;
  // call
  found = last_instr.find("call");
  if (found != string::npos) {
    branch_target = -1;
    children.insert(last_instr_num+1);
    // check if reach the end of a function
    if (newfunc_reached())
      return 0;
    return 1;
  }
  // br
  found = last_instr.find("br");
  if (found != string::npos) {
    branch_target = get_2op(last_instr).second;
    children.insert(branch_target);
    // check if reach the end of a function
    if (newfunc_reached())
      return 0;
    return 1;
  }
  // blbc and blbs
  found = last_instr.find("blbc");
  found1 = last_instr.find("blbs");
  if (found != string::npos || found1 != string::npos) {
    branch_target = get_2op(last_instr).second;
    children.insert(branch_target);
    children.insert(last_instr_num+1);
    // check if reach the end of a function
    if (newfunc_reached())
      return 0;
    return 1;
  }

  // check if reach the end of a function
  if (newfunc_reached())
    return 0;

  // bb end because of branch target in next bb
  children.insert(last_instr_num+1);

  return 1;

}

BasicBlock* Function::get_bb(int num) {
  for(int i=0; i<bb.size(); i++) {
    if (bb[i]->num == num)
      return bb[i];
  }
  assert(0 && "BB not found");
  return NULL;
}

void Function::populate() {
  bool ret;
  dead_var_offset.clear();

  // populate each basic block
  do {
    bb.push_back(new BasicBlock);
    ret = bb.back()->populate();
  } while(ret);

  // connect pointers
  for(int i=0; i<bb.size(); i++) {
    for(set<int>::iterator it = bb[i]->children.begin();
        it != bb[i]->children.end(); it++) {
      BasicBlock* b = get_bb(*it);
      bb[i]->children_p.insert(b);
      b->parent_p.insert(bb[i]);

      int child_num = b->num;
      pair<int, int> temp_pair = make_pair(bb[i]->num, child_num);
      edge[temp_pair] = new Edge;
      edge[temp_pair]->parent = bb[i];
      edge[temp_pair]->child = b;
      edge[temp_pair]->EARLIEST.clear();
      edge[temp_pair]->LATER.clear();
      edge[temp_pair]->INSERT.clear();
    }
  }

  return;
}

void Function::print_CFG() {
  cout<<"Function: "<<bb[0]->num<<endl;

  cout<<"Basic blocks:";
  for(int i=0; i<bb.size(); i++)
    cout<<" "<<bb[i]->num;

  cout<<endl<<"CFG:"<<endl;
  for(int i=0; i<bb.size(); i++) {
    cout<<bb[i]->num<<" ->";
    for(set<BasicBlock*>::iterator j=bb[i]->children_p.begin();
        j != bb[i]->children_p.end(); j++) {
      cout<<" "<<(*j)->num;
    }
    cout<<endl;
  }
  return;
}

void Function::print_instr() {
  string prefix = "    instr ";
  for(int i=0; i<bb.size(); i++) {
    if (bb[i]->main)
      cout<<prefix<<bb[i]->num-1<<": entrypc\n";

    for(list<Instr*>::iterator j=bb[i]->instr.begin(); j!=bb[i]->instr.end(); j++) {
      cout<<(*j)->instr<<"\n";
    }
  }

  return;
}


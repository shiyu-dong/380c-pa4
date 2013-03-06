#include <assert.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include "dce.h"
#include <stdio.h>

inline string itoa(int i) {
  stringstream ss;
  ss << i;
  return ss.str();
}

string MakeMove(int instr_num, int use, int def) {
  string t;
  t = "    instr ";
  t.append(itoa(instr_num));
  t.append(": move (");
  t.append(itoa(use));
  t.append(") (");
  t.append(itoa(def));
  t.append(")");
  return t;
}

void printSet(set<pair<OpType, int> > s) {
  for(set<pair<OpType, int> >::iterator i = s.begin();
      i != s.end(); i++) {
    cout<<i->first<<" "<<i->second<<endl;
  }
  cout<<endl;
}

void BasicBlock::compute_DEE() {
}

void BasicBlock::compute_KILL() {
  set<Exp> exps;
  for(list<Instr*>::iterator i = instr.begin();
      i != instr.end(); i++) {
    // insert this operand to KILL if it is defined by this instruction.
    if ((*i)->def.size() != 0 && ((*i)->def.front().first == REG || (*i)->def.front().first == VAR) ) {
      KILL.insert((*i)->def.front());
    }

    // check if the expression has appeared before in this BB
    if ((*i)->opcode_num != -1) {
      Exp t((*i)->opcode_num, (*i)->num, (*i)->use);

      set<Exp>::iterator it = exps.find(t);
      if (it == exps.end()) {
        exps.insert(t);
      }
      else {
        (*i)->use.clear();
        (*i)->use.push_back(std::make_pair(REG, it->instr_num));
        (*i)->def.clear();
        (*i)->def.push_back(std::make_pair(REG, (*i)->num));
        (*i)->opcode_num = 11;
        (*i)->opcode = "move";
        (*i)->instr.clear();
        (*i)->instr += MakeMove((*i)->num, it->instr_num, (*i)->num);
      }
    }
  }

  // debug
  cout<<"BB: "<<num<<endl;
  printSet(KILL);
  cout<<endl;
}

void Function::compute_KILL() {
  for(int i=0; i<bb.size(); i++) 
    bb[i]->compute_KILL();
}

void Function::compute_DEE() {
  for(int i=0; i<bb.size(); i++) 
    bb[i]->compute_DEE();
}

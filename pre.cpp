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

void printSet(set<Exp> s) {
  for(set<Exp>::iterator i = s.begin(); i != s.end(); i++) {
    cout<<"Exp: "<<i->instr_num<<endl;
  }
}


void printSet(set<pair<OpType, int> > s) {
  for(set<pair<OpType, int> >::iterator i = s.begin();
      i != s.end(); i++) {
    cout<<i->first<<" "<<i->second<<endl;
  }
}

bool SetEqual(set<Exp>* s1, set<Exp>* s2) {
  if(s1->size() != s2->size())
    return 0;
  for(set<Exp>::iterator i = s1->begin();
      i != s1->end(); i++) {
    if (s2->find(*i) == s2->end())
      return 0;
  }
  return 1;
}

void BasicBlock::compute_UEE() {
  set<Exp> exps;
  for(list<Instr*>::iterator i = instr.begin();
      i != instr.end(); i++) {
    // insert this operand to KILL if it is defined by this instruction.
    if ((*i)->def.size() != 0 && ((*i)->def.front().first == REG || (*i)->def.front().first == VAR) ) {
      KILL_t.insert((*i)->def.front());
    }

    // if the expression needs to be processed for PRE
    if ((*i)->opcode_num != -1 && (*i)->opcode_num < PRE_OPCODE_RANGE) {
      Exp t((*i)->opcode_num, (*i)->num, (*i)->use);

      // local redundancy
      // check if the expression has appeared before in this BB
      set<Exp>::iterator it;
      it = exps.find(t);
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

      // check if the expression uses any kills
      bool is_UEE = 1;
      for(list<pair<OpType, int> >::iterator j = (*i)->use.begin();
          j != (*i)->use.end(); j++) {
        if (KILL_t.find(*j) != KILL_t.end()) {
          is_UEE = 0;
          break;
        }
      }
      if (is_UEE) {
        UEE.insert(t);
      }
    }
  }

  // debug
  //cout<<"KILL_t: \n";
  //printSet(KILL_t);
  //cout<<"UEE BB: "<<num<<endl;
  //printSet(UEE);
  //cout<<endl;
}

void Function::compute_UEE() {
  for(int i=0; i<bb.size(); i++) 
    bb[i]->compute_UEE();
}

void BasicBlock::compute_DEE() {
  set<pair<OpType, int> > kill_dee;

  list<Instr*>::iterator i = instr.end();
  while( i != instr.begin()) {
    i--;

    // check if this instruction kills any variable
    if ((*i)->def.size() != 0 && ((*i)->def.front().first == REG || (*i)->def.front().first == VAR) ) {
      kill_dee.insert((*i)->def.front());
    }

    // if the expression needs to be processed for PRE
    if ((*i)->opcode_num != -1 && (*i)->opcode_num < PRE_OPCODE_RANGE) {
      Exp t((*i)->opcode_num, (*i)->num, (*i)->use);

      // check if the expression uses any kills
      bool is_DEE = 1;
      for(list<pair<OpType, int> >::iterator j = (*i)->use.begin();
          j != (*i)->use.end(); j++) {
        if (kill_dee.find(*j) != kill_dee.end()) {
          is_DEE = 0;
          break;
        }
      }
      if (is_DEE) {
        DEE.insert(t);
      }
    }
  }

  // debug
  //cout<<"DEE BB: "<<num<<endl;
  //printSet(DEE);
}

void Function::compute_DEE() {
  for(int i=0; i<bb.size(); i++) 
    bb[i]->compute_DEE();
}


void BasicBlock::compute_KILL(set<Exp>* base) {
  for(set<pair<OpType, int> >::iterator i = KILL_t.begin();
      i != KILL_t.end(); i++) {
    for(set<Exp>::iterator j = base->begin();
        j != base->end(); j++) {
      for(list<pair<OpType, int> >::const_iterator k = j->use.begin();
          k != j->use.end(); k++) {
        if (*k == *i) {
          KILL.insert(*j);
        }
      }
    }
  }

  // debug
  //cout<<"KILL BB: "<<num<<endl;
  //printSet(KILL);
}

void Function::compute_KILL() {
  for(int i=0; i<bb.size(); i++) 
    bb[i]->compute_KILL(&base);
}

void Function::compute_base() {
  for(int i=0; i<bb.size(); i++) {
    for(list<Instr*>::iterator j = bb[i]->instr.begin();
        j != bb[i]->instr.end(); j++) {
      if ((*j)->opcode_num != -1 && (*j)->opcode_num < PRE_OPCODE_RANGE) {
        Exp t((*j)->opcode_num, (*j)->num, (*j)->use);
        base.insert(t);
      }
    }
  }

  // debug
  //cout<<"base: \n";
  //printSet(base);
}

void Function::PRE_init() {
  for(int i=0; i<bb.size(); i++) {
    // initi of start and exit blocks
    // are taken care of in corresponding functions
    bb[i]->AVAIL_OUT = base;
    bb[i]->ANT_OUT = base;
    if (bb[i]->parent_p.size() != 0) {
      bb[i]->LATER_IN = base;
    }
  }
}

void Function::compute_ANT() {
  bool stable = 0;
  while(!stable) {
    stable = 1;

    // backward analysis
    for(int i=bb.size()-1; i >= 0; i--) {
      bb[i]->ANT_IN.clear();

      // Intersection of all children
      if (bb[i]->children_p.size() != 0) {
        set<BasicBlock*>::iterator j = bb[i]->children_p.begin();
        bb[i]->ANT_IN = (*j)->ANT_OUT;
        j++;
        while(j != bb[i]->children_p.end()) {
          bb[i]->ANT_IN = Intersect(& (*j)->ANT_OUT, &bb[i]->ANT_IN);
          j++;
        }
      }

      // Intersect (Not(KILL), temp)
      set<Exp> temp2 = Not(&bb[i]->KILL);
      temp2 = Intersect(&bb[i]->ANT_IN, &temp2);

      // Or (temp, UEE)
      temp2 = Union(&temp2, &bb[i]->UEE);

      // check if ANT stables
      if (! SetEqual(& temp2, & bb[i]->ANT_OUT)) {
        bb[i]->ANT_OUT = temp2;
        stable = 0;
      }
    }
  }

  // debug
  //cout<<"ANT func: "<<bb[0]->num<<"\n";
  //for(int i=0; i<bb.size(); i++) {
  //  cout<<"BB num: "<<bb[i]->num<<endl;
  //  printSet(bb[i]->ANT_OUT);
  //}
}

void Function::compute_AVAIL() {
  bool stable = 0;
  while(!stable) {
    stable = 1;

    // forward analysis
    for(int i=0; i<bb.size(); i++) {
      bb[i]->AVAIL_IN.clear();

      // intersection of all children
      if (bb[i]->parent_p.size() != 0) {
        set<BasicBlock*>::iterator j = bb[i]->parent_p.begin();
        bb[i]->AVAIL_IN = (*j)->AVAIL_OUT;
        j++;
        while(j != bb[i]->parent_p.end()) {
          bb[i]->AVAIL_IN = Intersect(& (*j)->AVAIL_OUT, & bb[i]->AVAIL_IN);
          j++;
        }
      }

      // Intersect (Not(Kill), temp)
      set<Exp> temp2 = Not(&bb[i]->KILL);
      temp2 = Intersect(&bb[i]->AVAIL_IN, &temp2);

      // Or (temp, DEE)
      temp2 = Union(&temp2, &bb[i]->DEE);

      // check if AVAIL stables
      if (! SetEqual(& temp2, & bb[i]->AVAIL_OUT)) {
        bb[i]->AVAIL_OUT = temp2;
        stable = 0;
      }
    }
  }

  // debug
  //cout<<"AVAIL func: "<<bb[0]->num<<"\n";
  //for(int i=0; i<bb.size(); i++) {
  //  cout<<"BB num: "<<bb[i]->num<<endl;
  //  printSet(bb[i]->AVAIL_OUT);
  //}
}

void Function::compute_EARLIEST() {
  // for each edge
  for(map<pair<int, int>, Edge*>::iterator i = edge.begin();
      i != edge.end(); i++) {
    Edge* t = i->second;

    set<Exp> temp = Not(& t->parent->AVAIL_OUT);
    t->EARLIEST = Intersect(& t->child->ANT_IN, &temp);

    if (t->parent->parent_p.size() != 0) {
      set<Exp> temp2 = Not(& t->parent->ANT_OUT);
      temp2 = Union(& t->parent->KILL, & temp2);
      t->EARLIEST = Intersect(& t->EARLIEST, & temp2);
    }
  }
}

void Function::compute_LATER() {
  bool stable = 0;
  while(!stable) {
    stable = 1;

    // update LATER for all edges
    for(map<pair<int, int>, Edge* >::iterator i = edge.begin();
        i != edge.end(); i++) {
      BasicBlock* parent = i->second->parent;
      set<Exp> temp = Not(& parent->UEE);
      temp = Intersect(& parent->LATER_IN, & temp);
      temp = Union(& i->second->EARLIEST, &temp);

      if (!SetEqual(& temp, & i->second->LATER)) {
        stable = 0;
        i->second->LATER = temp;
      }
    }

    // update LATER_IN for all bb
    for(int i=0; i<bb.size(); i++) {
      set<Exp> temp;
      temp.clear();

      if (bb[i]->parent_p.size() != 0) {
        set<BasicBlock*>::iterator j = bb[i]->parent_p.begin();

        Edge* e = edge[make_pair((*j)->num, bb[i]->num)];
        temp = e->LATER;
        j++;
        while (j != bb[i]->parent_p.end()) {
          e = edge[make_pair((*j)->num, bb[i]->num)];
          Intersect(& temp, & e->LATER);
          j++;
        }
      }

      if (!SetEqual(& temp, & bb[i]->LATER_IN)) {
        stable = 0;
        bb[i]->LATER_IN = temp;
      }
    }
  }
}

void Function::compute_INSERT() {
  for(map<pair<int, int>, Edge*>::iterator i = edge.begin();
      i != edge.end(); i++) {
    set<Exp> temp = Not(& i->second->child->LATER_IN);
    i->second->INSERT = Intersect(& i->second->LATER, & temp);
  }
}

void Function::compute_DELETE() {
  for(int i=0; i<bb.size(); i++) {
    set<Exp> temp = Not(& bb[i]->LATER_IN);
    bb[i]->DELETE = Intersect(& bb[i]->UEE, & temp);
  }
}

set<Exp> Function::Intersect(const set<Exp>* s1, const set<Exp>* s2) {
  set<Exp> s3;
  for(set<Exp>::iterator i = s1->begin(); i != s1->end(); i++) {
    if (s2->find(*i) != s2->end()) {
      s3.insert(*i);
    }
  }

  return s3;
}

set<Exp> Function::Union(const set<Exp>* s1, const set<Exp>* s2) {
  set<Exp> s3;
  for(set<Exp>::iterator i = s1->begin(); i != s1->end(); i++) {
      s3.insert(*i);
  }
  for(set<Exp>::iterator i = s2->begin(); i != s2->end(); i++) {
      s3.insert(*i);
  }
  return s3;
}

set<Exp> Function::Not(const set<Exp>* s1) {
  set<Exp> s2;
  for(set<Exp>::iterator i = base.begin(); i != base.end(); i++) {
    if (s1->find(*i) == s1->end()) {
      s2.insert(*i);
    }
  }
  return s2;
}


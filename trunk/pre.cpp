#include <string>
#include <assert.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include "dce.h"
#include <stdio.h>

extern string opcode[];
int new_instr_num;

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
    cout<<"Exp: "<<i->instr_num<<" "<<i->exp<<endl;
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
      Exp t((*i)->opcode_num, (*i)->num, (*i)->use, (*i)->instr);

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
      Exp t((*i)->opcode_num, (*i)->num, (*i)->use, (*i)->instr);

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
        Exp t((*j)->opcode_num, (*j)->num, (*j)->use, (*j)->instr);
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
      bb[i]->AVAIL_IN = base;
    }
    else {
      bb[i]->LATER_IN.clear();
      bb[i]->AVAIL_IN.clear();
    }

    if (bb[i]->children_p.size() != 0) {
      bb[i]->ANT_IN = base;
    }
    else {
      bb[i]->ANT_IN.clear();
    }
  }

  for(map<pair<int, int>, Edge*>::iterator i = edge.begin();
      i != edge.end(); i++) {
    i->second->LATER = base;
  }
}

void Function::compute_ANT() {
  bool stable = 0;
  while(!stable) {
    stable = 1;

    // backward analysis
    for(int i=bb.size()-1; i >= 0; i--) {
      //bb[i]->ANT_IN.clear();
      set<Exp> ANT_OUT_t;
      ANT_OUT_t.clear();

      // Intersection of all children
      if (bb[i]->children_p.size() != 0) {
        set<BasicBlock*>::iterator j = bb[i]->children_p.begin();
        ANT_OUT_t = (*j)->ANT_IN;
        j++;
        while(j != bb[i]->children_p.end()) {
          ANT_OUT_t = Intersect(& (*j)->ANT_IN, & ANT_OUT_t);
          j++;
        }
      }

      if (!SetEqual(& ANT_OUT_t, & bb[i]->ANT_OUT)) {
        bb[i]->ANT_OUT = ANT_OUT_t;
        stable = 0;
      }

      // Intersect (Not(KILL), temp)
      set<Exp> temp2 = Not(&bb[i]->KILL);
      temp2 = Intersect(&bb[i]->ANT_OUT, &temp2);

      // Or (temp, UEE)
      temp2 = Union(&temp2, &bb[i]->UEE);

      // check if ANT stables
      if (! SetEqual(& temp2, & bb[i]->ANT_IN)) {
        bb[i]->ANT_IN = temp2;
        stable = 0;
      }
    }
  }

  // debug
  //cout<<"ANT_OUT func: "<<bb[0]->num<<"\n";
  //for(int i=0; i<bb.size(); i++) {
  //  cout<<"BB num: "<<bb[i]->num<<endl;
  //  printSet(bb[i]->ANT_OUT);
  //}
  //cout<<"ANT_IN func: "<<bb[0]->num<<"\n";
  //for(int i=0; i<bb.size(); i++) {
  //  cout<<"BB num: "<<bb[i]->num<<endl;
  //  printSet(bb[i]->ANT_IN);
  //}
}

void Function::compute_AVAIL() {
  bool stable = 0;
  while(!stable) {
    stable = 1;

    // forward analysis
    for(int i=0; i<bb.size(); i++) {
      //bb[i]->AVAIL_IN.clear();
      set<Exp> AVAIL_IN_t;
      AVAIL_IN_t.clear();

      // intersection of all children
      if (bb[i]->parent_p.size() != 0) {
        set<BasicBlock*>::iterator j = bb[i]->parent_p.begin();
        AVAIL_IN_t = (*j)->AVAIL_OUT;
        j++;
        while(j != bb[i]->parent_p.end()) {
          AVAIL_IN_t = Intersect(& (*j)->AVAIL_OUT, & AVAIL_IN_t);
          j++;
        }
      }

      if (!SetEqual(& AVAIL_IN_t, & bb[i]->AVAIL_IN)) {
          bb[i]->AVAIL_IN = AVAIL_IN_t;
          stable = 0;
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
  //cout<<"AVAIL_IN func: "<<bb[0]->num<<"\n";
  //for(int i=0; i<bb.size(); i++) {
  //  cout<<"BB num: "<<bb[i]->num<<endl;
  //  printSet(bb[i]->AVAIL_IN);
  //}
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

  //debug
  //for(map<pair<int, int>, Edge* >::iterator i = edge.begin();
  //    i != edge.end(); i++) {
  //    cout << "EARLIEST edge: " << i->second->parent->num << " -> " <<
  //      i->second->child->num << endl;
  //    printSet(i->second->EARLIEST);
  //}
  //cout << endl;
}

void Function::compute_LATER() {
  bool stable = 0;
  
  while (!stable) {
    stable = 1;
    // update LATER for all edges
    for(map<pair<int, int>, Edge* >::iterator i = edge.begin();
        i != edge.end(); i++) {
      //update LATER_IN for parent
      BasicBlock* parent = i->second->parent;

      set<Exp> temp;
      temp.clear();

      if (parent->parent_p.size() != 0) {
        set<BasicBlock*>::iterator j = parent->parent_p.begin();
        pair<int, int> key = make_pair((*j)->num, parent->num);
        temp = edge[key]->LATER;
        j++;
        while(j != parent->parent_p.end()) {
          key = make_pair((*j)->num, parent->num);
          temp = Intersect(& temp, & edge[key]->LATER);
          j++;
        }
      }

      if (!SetEqual(& temp, & parent->LATER_IN)) {
        parent->LATER_IN = temp;
        stable = 0;
      }

      temp.clear();
      temp = Not(& parent->UEE);
      temp = Intersect(& parent->LATER_IN, & temp);
      temp = Union(& i->second->EARLIEST, & temp);

      if (!SetEqual(& temp, & i->second->LATER)) {
        i->second->LATER = temp;
        stable = 0;
      }

    }
  }

  //debug
  //cout<<"LATER_IN func: "<<bb[0]->num<<"\n";
  //for(int i=0; i<bb.size(); i++) {
  //  cout<<"BB num: "<<bb[i]->num<<endl;
  //  printSet(bb[i]->LATER_IN);
  //}
  //cout << endl;
  //for(map<pair<int, int>, Edge* >::iterator i = edge.begin();
  //    i != edge.end(); i++) {
  //    cout << "LATER edge: " << i->second->parent->num << " -> " <<
  //      i->second->child->num << endl;
  //    printSet(i->second->LATER);
  //}
  //cout << endl;
}

void Function::compute_INSERT() {
  for(map<pair<int, int>, Edge*>::iterator i = edge.begin();
      i != edge.end(); i++) {
    set<Exp> temp = Not(& i->second->child->LATER_IN);
    i->second->INSERT = Intersect(& i->second->LATER, & temp);
  }

  //debug
  for(map<pair<int, int>, Edge* >::iterator i = edge.begin();
      i != edge.end(); i++) {
      cout << "INSERT edge: " << i->second->parent->num << " -> " <<
        i->second->child->num << endl;
      printSet(i->second->INSERT);
  }
  cout << endl;
}

void Function::compute_DELETE() {
  for(int i=0; i<bb.size(); i++) {
    if (bb[i]->parent_p.size() > 0) {
      set<Exp> temp = Not(& bb[i]->LATER_IN);
      bb[i]->DELETE = Intersect(& bb[i]->UEE, & temp);
    }
  }

  //debug
  cout<<"DELETE func: "<<bb[0]->num<<"\n";
  for(int i=0; i<bb.size(); i++) {
    cout<<"BB num: "<<bb[i]->num<<endl;
    printSet(bb[i]->DELETE);
  }
  cout << endl;
}

void Function::rewrite() {
  // remember number of new bb that I inserted
  map<pair<int, int>, BasicBlock*> new_bb;
  // start new instruction number backwards
  new_instr_num = 65535;
  int new_bb_num = 62000;

  // for each edge
  for(map<pair<int, int>, Edge*>::iterator i=edge.begin();
      i != edge.end(); i++) {

    // for each insert in the edge
    for(set<Exp>::iterator j = i->second->INSERT.begin();
        j != i->second->INSERT.end(); j++) {

      // create new instruction
      Instr* new_instr = new Instr;
      new_instr->num = new_instr_num;
      new_instr->use = j->use;
      new_instr->def.push_back(make_pair(REG, new_instr->num));
      new_instr->opcode_num = j->opcode_num;
      new_instr->opcode = opcode[j->opcode_num];
      new_instr->instr = "    instr ";
      new_instr->instr.append(itoa(new_instr_num));
      new_instr->instr.append(": ");
      new_instr->instr.append(j->exp);
      new_instr_num = new_instr_num-2;

      // just one successor
      if (i->second->parent->children_p.size() == 1) {
        BasicBlock* parent = i->second->parent;

        // check if last instruction is a branch
        int last_instr = parent->instr.back()->opcode_num;
        if (last_instr == 16 || last_instr == 19 || last_instr == 20) {
          // if last instr is a branch,
          // need to insert it to second last pos
          list<Instr*>::iterator it = parent->instr.end();
          it--;
          parent->instr.insert(it, new_instr);
        }
        else {
          // insert the instruction to the end, or the last second position
          // in parent basic block
          parent->instr.push_back(new_instr);
        }
      }

      // just one predecessor
      else if (i->second->child->parent_p.size() == 1) {
        BasicBlock* child = i->second->child;
        int old_instr_num;

        // add a branch with the instruction number
        // as the same as the number of the 1st instruction originally
        // add new instruction after the nop
        // In this way, no need to change branch dest of parent

        // change the first instruction's number
        Instr* this_instr = child->instr.front();
        old_instr_num = this_instr->num;
        this_instr->num = new_instr_num+1;
        int pos = this_instr->instr.find(':');
        string orig_exp = this_instr->instr.substr(pos);
        this_instr->instr.clear();
        this_instr->instr = "    instr ";
        this_instr->instr.append(itoa(new_instr_num+1));
        this_instr->instr.append(orig_exp);

        // always insert this instruction at the very beginning
        child->instr.push_front(new_instr);

        // insert a nop at the beginning
        Instr* nop_instr = new Instr;
        nop_instr->num = old_instr_num;
        nop_instr->instr = "    instr ";
        nop_instr->instr.append(itoa(old_instr_num));
        nop_instr->instr.append(": nop");
        nop_instr->opcode_num = 21;
        nop_instr->opcode = "nop";
        child->instr.push_front(nop_instr);
      }
      
      // insert instruction into a newly created bb
      else if (new_bb.find(make_pair(i->first.first, i->first.second)) != new_bb.end()) {
        BasicBlock* bb = new_bb[make_pair(i->first.first, i->first.second)];
        // always insert this instruction at the very beginning
        bb->instr.push_back(new_instr);
      }

      // more than one successors and predecessors
      else {
        BasicBlock* this_bb = new BasicBlock;
        this_bb->num = new_bb_num;
        new_bb_num--;
        this_bb->main = 0;
        // always insert this instruction at the very beginning
        this_bb->instr.push_front(new_instr);
        this_bb->num = new_instr->num;

        // TODO: do we still need nop here???
        Instr* nop_instr = new Instr;
        nop_instr->num = this_bb->num;
        nop_instr->instr = "    instr ";
        nop_instr->instr.append(itoa(this_bb->num));
        nop_instr->instr.append(": nop");
        nop_instr->opcode_num = 21;
        nop_instr->opcode = "nop";
        this_bb->instr.push_front(nop_instr);

        // reconnect at the end of this function
        // add to new_bb
        new_bb[make_pair(i->first.first, i->first.second)] = this_bb;
      }

    } // end of each insert in an edge
  } // end of all edges

  // reconnect the graph
  // children, children_p, parent_p
  for(map<pair<int, int>, BasicBlock*>::iterator i = new_bb.begin();
      i != new_bb.end(); i++) {
    int parent_num = i->first.first;
    int child_num = i->first.second;
    BasicBlock* parent = get_bb(parent_num);
    BasicBlock* child = get_bb(child_num);
    BasicBlock* this_bb = i->second;

    // reconnect pointers
    parent->children_p.erase(child);
    parent->children_p.insert(this_bb);
    parent->children.erase(child_num);
    child->parent_p.erase(parent);
    child->parent_p.insert(this_bb);
    this_bb->children.insert(child_num);
    this_bb->children_p.insert(child);
    this_bb->parent_p.insert(parent);

    if (parent->branch_target == child_num) {
      // edge is a branch taken
      // change the branch target of the last instruction of parent
      parent->branch_target = this_bb->num;
      string* instr_str = &(parent->instr.back()->instr);
      int pos = instr_str->rfind('[');
      *instr_str = instr_str->substr(0, pos+1);
      instr_str->append(itoa(this_bb->num));
      instr_str->append("]");

      // add unconditional branch at the end to the new bb
      Instr* this_instr = new Instr;
      this_instr->instr = "    instr ";
      this_instr->instr.append(itoa(new_instr_num));
      this_instr->instr.append(": br [");
      this_instr->instr.append(itoa(child->instr.front()->num));// branch to the actual instruction number
      this_instr->instr.append("]");
      this_instr->opcode_num = 16;
      this_instr->opcode = "br";
      this_bb->instr.push_back(this_instr);
      new_instr_num--;
      // insert new bb before the children
      vector<BasicBlock*>::iterator j = bb.begin();
      while(*j != child)
        j++;
      bb.insert(j, this_bb);
    }
    else {
      // edge is instruction order
      // insert new bb after the old parent
      vector<BasicBlock*>::iterator j = bb.begin();
      while(*j != parent)
        j++;
      bb.insert(j, this_bb);
    }
  } // end of reconnect

  //debug
  //cout<<"@@@"<<endl;
  print_CFG();
  //cout<<"@@@"<<endl;

  // for each delete exp in each bb,
  // find cooresponding instruction
  for(int i=0; i<bb.size(); i++) {
    for(set<Exp>::iterator j = bb[i]->DELETE.begin();
        j != bb[i]->DELETE.end(); j++) {
      // find cooresponding instruction
      // for the delete
      set<BasicBlock*> visited;
      visited.clear();
      int ref_num = find_ref(bb[i], *j, visited);

      assert(ref_num != -1);

      // find the instruction to be deleted,
      // change it to move
      for(list<Instr*>::iterator k = bb[i]->instr.begin();
          k != bb[i]->instr.end(); k++) {
        if ((*k)->opcode_num < PRE_OPCODE_RANGE && (*k)->opcode_num != -1) {
          Exp t((*k)->opcode_num, (*k)->num, (*k)->use, (*k)->opcode);
          if (t == *j) {
            // change it to move
            (*k)->use.clear();
            (*k)->use.push_back(std::make_pair(REG, ref_num));
            (*k)->def.clear();
            (*k)->def.push_back(std::make_pair(REG, (*k)->num));
            (*k)->opcode_num = 11;
            (*k)->opcode = "move";
            (*k)->instr.clear();
            (*k)->instr += MakeMove((*k)->num, ref_num, (*k)->num);
          }
        }
      }

      // search up in cfg to make sure
      // every path from start to this point has cooresponding expression calculated
      visited.clear();
      fix_up(bb[i], *j, ref_num, visited);

    } // end of one delete expression
  } // end of delete exp in each bb
}

void Function::fix_up(BasicBlock*& this_bb, const Exp& exp, int ref_num, set<BasicBlock*>& visited) {
  visited.insert(this_bb);
  int parent_index, this_index;

  for(int i=0; i<bb.size(); i++) {
    if (this_bb == bb[i]) {
      this_index = i;
    }
  }

  for(set<BasicBlock*>::iterator it = this_bb->parent_p.begin();
      it != this_bb->parent_p.end(); it++) {

    for(int i=0; i<bb.size(); i++) {
      if (*it == bb[i]) {
        parent_index = i;
      }
    }

    if (parent_index < this_index)
      fix_up_helper(*it, exp, ref_num, visited);
  }
}

void Function::fix_up_helper(BasicBlock* const& this_bb, const Exp& exp, int ref_num, set<BasicBlock*>& visited) {
  visited.insert(this_bb);

  // for each instruction in this bb
  for(list<Instr*>::iterator it = this_bb->instr.begin();
      it != this_bb->instr.end(); it++) {
    // if this bb has the exact same express 
    // thus has the same ref_num
    if ((*it)->num == ref_num) {
      return;
    }

    if ((*it)->opcode_num >= PRE_OPCODE_RANGE && (*it)->opcode_num != -1)
      continue;
    
    Exp t((*it)->opcode_num, (*it)->num, (*it)->use, (*it)->instr);
    // if this bb use the same expression
    // but doesn't have the same ref num
    if (t == exp) {
      int that_num = (*it)->num;

      Instr* new_move = new Instr;
      new_move->num = new_instr_num;
      new_move->use.push_back(make_pair(REG, that_num));
      new_move->def.push_back(make_pair(REG, ref_num));
      new_move->opcode_num = 11;
      new_move->opcode = "move";
      new_move->instr.clear();
      new_move->instr += MakeMove(new_instr_num, that_num, ref_num);

      new_instr_num--;

      // insert the newly created instructions to the bottom of the bb
      int last_instr = this_bb->instr.back()->opcode_num;
      // check if last instruction is a branch
      if (last_instr == 16 || last_instr == 19 || last_instr == 20) {
        // if last instr is a branch,
        // need to insert it to second last pos
        list<Instr*>::iterator it = this_bb->instr.end();
        it--;
        this_bb->instr.insert(it, new_move);
      }
      else {
        // insert at the bottom of the bb
        list<Instr*>::iterator it = this_bb->instr.end();
        this_bb->instr.insert(it, new_move);
      }
      return;
    }
  }

  // if this bb kills exp
  if (this_bb->KILL.find(exp) != this_bb->KILL.end() ) {
    // calculate exp at the end of this bb
    Instr* new_instr = new Instr;
    new_instr->num = new_instr_num;
    new_instr->use = exp.use;
    new_instr->def.push_back(make_pair(REG, new_instr_num));
    new_instr->instr.clear();
    new_instr->instr.append("    instr ");
    new_instr->instr.append(itoa(new_instr_num));
    new_instr->instr.append(": ");
    new_instr->instr.append(exp.exp);
    new_instr->opcode_num = exp.opcode_num;
    new_instr->opcode = opcode[new_instr->opcode_num];

    new_instr_num--;

    // move the new result to REG ref_num
    Instr* new_move = new Instr;
    new_move->num = new_instr_num;
    new_move->use.push_back(make_pair(REG, new_instr_num+1));
    new_move->def.push_back(make_pair(REG, ref_num));
    new_move->opcode_num = 11;
    new_move->opcode = "move";
    new_move->instr.clear();
    new_move->instr += MakeMove(new_instr_num, new_instr_num+1, ref_num);

    new_instr_num--;

    // insert the newly created instructions to the bottom of the bb
    int last_instr = this_bb->instr.back()->opcode_num;
    // check if last instruction is a branch
    if (last_instr == 16 || last_instr == 19 || last_instr == 20) {
      // if last instr is a branch,
      // need to insert it to second last pos
      list<Instr*>::iterator it = this_bb->instr.end();
      it--;
      this_bb->instr.insert(it, new_instr);

      it = this_bb->instr.end();
      it--;
      this_bb->instr.insert(it, new_move);
    }
    else {
      // insert at the bottom of the bb
      list<Instr*>::iterator it = this_bb->instr.end();
      this_bb->instr.insert(it, new_instr);

      it = this_bb->instr.end();
      this_bb->instr.insert(it, new_move);
    }
    return;
  }

  // if reach start, fail
  if (this_bb->parent_p.size() == 0) {
    assert("fix_up reach start" && 0);
  }
  else {
    int parent_index, this_index;
    for(int ii = 0; ii<bb.size(); ii++) {
      if (this_bb == bb[ii]) {
        this_index = ii;
      }
    }

    for(set<BasicBlock*>::iterator it = this_bb->parent_p.begin();
        it != this_bb->parent_p.end(); it++) {

      for(int ii = 0; ii<bb.size(); ii++) {
        if (*it == bb[ii]) {
          parent_index = ii;
        }
      }

      if (visited.find(*it) == visited.end() && parent_index < this_index) {
        fix_up_helper(*it, exp, ref_num, visited);
      }
    }
  }
}

int Function::find_ref(BasicBlock*& this_bb, const Exp& exp, set<BasicBlock*>& visited) {
  int ret;

  visited.insert(this_bb);

  for(set<BasicBlock*>::iterator it = this_bb->parent_p.begin();
      it != this_bb->parent_p.end(); it++) {
    ret = find_ref_helper(*it, exp, visited);
    if (ret != -1)
      return ret;
  }

  return -1;
}

int Function::find_ref_helper(BasicBlock* const& this_bb, const Exp& exp, set<BasicBlock*>& visited) {
  visited.insert(this_bb);

  // go through each instruction of the bb
  for(list<Instr*>::iterator it = this_bb->instr.begin();
      it != this_bb->instr.end(); it++) {
    if ((*it)->opcode_num < PRE_OPCODE_RANGE && (*it)->opcode_num != -1) {
      Exp t((*it)->opcode_num, (*it)->num, (*it)->use, (*it)->opcode);
      if (t == exp) {
        return (*it)->num;
      }
    }
  }

  // exp not found in this bb
  // check all its parents
  if (this_bb->parent_p.size() == 0) {
    return -1;
  }
  for(set<BasicBlock*>::iterator it = this_bb->parent_p.begin();
      it != this_bb->parent_p.end(); it++) {
    if (visited.find(*it) == visited.end()) {
      int ret = find_ref_helper(*it, exp, visited);
      if (ret != -1)
        return ret;
    }
  }

  return -1;
}

void Function::eliminate(BasicBlock* child, BasicBlock* starter, Exp exp, int insert_num, set<BasicBlock*>& bb_visited) {

  //debug
  // cout<<"eliminate testing bb: "<<child->num<<" \n";
  // update bb_visited
  bb_visited.insert(child);

  if (child->KILL.find(exp) != child->KILL.end()) {
    return;
  }

  // if child_bb->delete has the exp
  if (child->DELETE.find(exp) != child->DELETE.end()) {
    // find the one instruction uses the same exp
    for(list<Instr*>::iterator it = child->instr.begin();
        it != child->instr.end(); it++) {
      if ((*it)->opcode_num < PRE_OPCODE_RANGE && (*it)->opcode_num != -1) {
        Exp t((*it)->opcode_num, (*it)->num, (*it)->use, (*it)->opcode);
        if (t == exp) {
          // change it to move
          (*it)->use.clear();
          (*it)->use.push_back(std::make_pair(REG, insert_num));
          (*it)->def.clear();
          (*it)->def.push_back(std::make_pair(REG, (*it)->num));
          (*it)->opcode_num = 11;
          (*it)->opcode = "move";
          (*it)->instr.clear();
          (*it)->instr += MakeMove((*it)->num, insert_num, (*it)->num);

          // for parents that are not in bb_visited
          //for(set<BasicBlock*>::iterator j = child->parent_p.begin();
          //    j != child->parent_p.end(); j++) {
          //  if (bb_visited.find(*j) == bb_visited.end()) {
          //    fixup(*j, starter, exp, insert_num, bb_visited);
          //  }
          //}
          //fixup(child, starter, exp, insert_num, bb_visited);
        }
      }
    }
  }

  // if child has no children
  if (child->children_p.size() == 0) {
    return;
  }

  else {
    // for each child
    for(set<BasicBlock*>::iterator i = child->children_p.begin();
        i != child->children_p.end(); i++) {
      if (bb_visited.find(*i) == bb_visited.end() ) {
        // eliminate grand children if it is not visited
        eliminate(*i, starter, exp, insert_num, bb_visited);
      }
    }
    return;
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


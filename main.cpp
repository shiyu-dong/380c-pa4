#include "dce.h"
#include <iostream>
#include <vector>
#include <assert.h>
using namespace std;

int main(int argc, char* argv[]) {
  // argv[2] optimize
  // argv[3] print format

  int j=0;
  vector<Function*> functions;
  assert(argc == 3);

  // get all branch targets
  string temp;
  int pos1, pos2;

  while(getline(cin, temp)) {
    pos1 = temp.find("br");
    if (pos1 != string::npos) {
      br_target.insert(get_2op(temp).second);
      continue;
    }

    pos1 = temp.find("blbc");
    pos2 = temp.find("blbs");
    if (pos1 != string::npos || pos2 != string::npos) {
      br_target.insert(get_2op(temp).second);
      continue;
    }
  }

  // rewind cin
  cin.clear();
  cin.seekg(0);
  // get the first nop
  getline(cin, temp);

  // build CFG
  while(!cin.eof()) {
    functions.push_back(new Function);
    functions[j]->populate();
    j++;
  }

  // compute PRE
  if(!strcmp(argv[1], "-opt=pre")) {
    for(int i=0; i<functions.size(); i++) {
      functions[i]->compute_KILL();
    }
  }

  // compute liveness and dce
  else if(!strcmp(argv[1], "-opt=dce")) {
    for(int i=0; i<functions.size(); i++) {
      functions[i]->dce();
    }
  }

  if(!strcmp(argv[2], "-backend=3addr")) {

    string prefix = "    instr ";
    cout<<prefix<<"1"<<": nop\n";

    for(int i=0; i<functions.size(); i++)
      functions[i]->print_instr();

    int last_instr_num = functions.back()->bb.back()->instr.back()->num;
    cout<<prefix<<++last_instr_num<<": nop\n";

    return 0;
  }
  else {
    for(int i=0; i<functions.size(); i++)
      functions[i]->print_CFG();
    return 0;
  }

  return 0;
}

import sys, string, re, Queue


arith = ['sub', 'div', 'mod', 'cmple', 'add', 'mul', 'cmpeq', 'cmplt']
operators = ['-', '/', '%', '<=', '+', '*', '==', '<']

arith1 = ['neg']

local_size = 0;

# get operand
def getOperand(t, sline, access_local):
#GP
  if sline[t] == 'GP':
    print '(char*)global',
    return t+1
#FP
  elif sline[t] == 'FP':
    if access_local:
      print '(char*)&local[' + str(local_size/8 -1) + ']',
    else:
      print '(char*)param',
    return t+1
#constant
  elif sline[t].isdigit():
    print sline[t],
    return t+1
#address offsets and field offsets
  elif sline[t].endswith('_base') or sline[t].endswith('_offset'):
    if sline[t+1][0] == '-':
      print '(' + str(int(sline[t+1])+8) + ')',
      return -(t+2)
    else:
      print str(int(sline[t+1])-8),
      return t+2
#register name
  elif sline[t][0] == '(':
    print 'r' + sline[t].strip('()'),
    return t+1
#code label
  elif sline[t][0] == '[':
    print 'instr_' + sline[t].strip('[]'),
    return t+1
#local variables
  else:
    if sline[t+1][0] == '-':
      print 'local[' + str((local_size-int(sline[t+1].strip('-')))/8) + ']',
    else:
      print 'param[' + str(int(sline[t+1])/8-1) + ']',
    return t+2


# get next operand start
def getStart(t, sline):
#GP
  if sline[t] == 'GP':
    return t+1
#FP
  elif sline[t] == 'FP':
    return t+1
#constant
  elif sline[t].isdigit():
    return t+1
#address offsets and field offsets
  elif sline[t].endswith('base') or sline[t].endswith('_offsets'):
    if sline[t+1][0] == '-':
      return -(t+2)
    else:
      return t+2
#register name
  elif sline[t][0] == '(':
    return t+1
#code label
  elif sline[t][0] == '[':
    return t+1
#local variables
  else:
    return t+2


#-----------------  Main  -----------------#
#if len(sys.argv) != 2:
#  print "please specify input file name"
#  sys.exit(0)
#
#ifile = open(sys.argv[1], 'r')

#parameters
params = Queue.LifoQueue()
params_n = 0

parsing_main = 0

# Print out header of the file
print '#include <stdio.h>\n\
#include <stdlib.h>\n\
#include <string.h>\n\
#define WriteLine() printf("\\n");\n\
#define WriteLong(x) printf(" %lld", x);\n\
#define ReadLong(a) if (fscanf(stdin, "%lld", &a) != 1) a = 0;\n\
#define long long long\n\n'

print 'long global[4096];\n'

# parse the file line by line
#for line in ifile:
for line in sys.stdin:
  sline = re.split(': | |#', line.rstrip('\n').lstrip(' '))

  if sline[2] == 'nop':
    continue

#print label for next instruction
  if sline[2] != 'enter' and sline[2] != 'entrypc':
    print 'instr_' + sline[1] + ':;\n\t',

#function start
  if sline[2] == 'enter':
    assert int(sline[3]) % 8 == 0, 'operand not divisible by 8';
    if not parsing_main:
      print 'void func_' + sline[1] + '(long* param) {\n',
    else:
      print 'void main() {\n',
    if (sline[3] != '0'):
      print 'long local[' + str(int(sline[3])/8) + '];\n',
    local_size = int(sline[3]);
    parsing_main = 0

#main start
  if sline[2] == 'entrypc':
    parsing_main = 1

#function return
  elif sline[2] == 'ret':
    print 'return;\n}\n',

#arithmatic
#  elif sline[2] in arith:
#    print 'long r' + sline[1] + ' =',
##    t = getOperand(3, sline, 0)
##    print operators[arith.index(sline[2])],
##    if (t < 0):
##      getOperand(-t, sline, 1)
##    else:
##      getOperand(t, sline, 0)
#    t = getStart(3, sline)
#    if (t < 0):
#      getOperand(-t, sline, 1)
#    else:
#      getOperand(t, sline, 0)
#    print operators[arith.index(sline[2])],
#    getOperand(3, sline, 0)
#    print ';\n',

  elif sline[2] in arith:
    print 'long r' + sline[1] + ' =',
    t = getOperand(3, sline, 0)
    print operators[arith.index(sline[2])],
    if (t < 0):
      getOperand(-t, sline, 1)
    else:
      getOperand(t, sline, 0)
    print ';\n',

  elif sline[2] in arith1:
    print 'long r' + sline[1] + ' =',
    t = getOperand(3, sline, 0)
    print ' * (-1);\n',

#branch
  elif sline[2] == 'br':
    print 'goto ',
    getOperand(3, sline, 0)
    print ';\n',
  elif sline[2] == 'blbs':
    print 'if (',
    t = getOperand(3, sline, 0)
    print '!= 0) goto',
    getOperand(t, sline, 0)
    print ';\n',
  elif sline[2] == 'blbc':
    print 'if (',
    t = getOperand(3, sline, 0)
    print '== 0) goto',
    getOperand(t, sline, 0)
    print ';\n',

#data movement
  elif sline[2] == 'load':
    print 'long r' + sline[1] + ' = *(long*)',
    getOperand(3, sline, 0)
    print ';\n',
  elif sline[2] == 'move':
    t = getStart(3, sline);
    getOperand(t, sline, 0)
    print ' = ',
    getOperand(3, sline, 0)
    print ';\n',
  elif sline[2] == 'store':
    print '*(long*)',
    t = getStart(3, sline)
    getOperand(t, sline, 0)
    print ' =',
    getOperand(3, sline, 0)
    print ';\n',

#I/O
  elif sline[2] == 'write':
    print 'WriteLong(',
    getOperand(3, sline, 0)
    print ');\n',
  elif sline[2] == 'wrl':
    print 'WriteLine();\n',
  elif sline[2] == 'read':
    #TODO: read didn't appear in all any tests.. need to be tested
    print 'long r' + sline[1] + ';\n\t',
    print 'ReadLong( r' + sline[1],
    print ');\n',

#Parameter and call
  elif sline[2] == 'param':
    print 'long r' + sline[1] + ' = ',
    getOperand(3, sline, 0)
    print ';//input parameter\n',
    params.put(sline[1])
    params_n += 1
  elif sline[2] == 'call':
    param_name = 'param_' + sline[1]
    print 'long* ' + param_name + ' = (long*)malloc(sizeof(long)*' + str(params_n+1) + ');\n',
    params_n = 0;
    while not params.empty():
      tt = params.get();
      print 'memcpy(' + param_name + '+' + str(params_n+1) + ', &r' + tt + ', sizeof(long));\n',
      params_n += 1
    params_n = 0
    print 'func_' + sline[3].strip('[]') + '(' + param_name + ');\n',
    print 'free (' + str(param_name) + ');\n',


sys.exit(0)

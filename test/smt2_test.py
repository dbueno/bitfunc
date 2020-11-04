import bitfunc.parse_smt2 as parse_smt2
import gzip

# with gzip.open('/Users/denbuen/work/SAT/gulwani-pldi08/fig6.phx.smt2.gz') as fd:
#   contents = fd.read()
# with open('/Users/denbuen/work/bitfunc/test/vars.smt2', 'r') as fd:
#   contents = fd.read()
with open('/Users/denbuen/work/SAT/gulwani-pldi08/fig6.phx.smt2', 'r') as fd:
  contents = fd.read()
x=parse_smt2.parseString(contents)

for a in x.asserts:
  with open('dot.out', 'w') as fd:
    fd.write(a.as_dot())
print str(x)

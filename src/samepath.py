# Copyright 2012 Sandia Corporation. Under the terms of Contract
# DE-AC04-94AL85000, there is a non-exclusive license for use of this work by or
# on behalf of the U.S. Government. Export of this program may require a license
# from the United States Government.
import os.path, sys

p1 = os.path.realpath(sys.argv[1])
p2 = os.path.realpath(sys.argv[2])

if p1 == p2:
  s = 1
else:
  s = 0

sys.exit(s)

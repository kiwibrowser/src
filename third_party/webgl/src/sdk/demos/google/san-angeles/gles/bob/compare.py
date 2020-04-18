#!/usr/bin/env python

us = [float(f) for f in file('mine.txt')]
them = [float(f) for f in file('them.txt')]

assert len(us) == len(them)

for i, (a, b) in enumerate(zip(us, them)):
  diff = a - b
  if diff > 2./65536:
    vert, coord = divmod(i, 3)
    round, vert = divmod(vert, 6)
    print "%d:%d:%d" % (round, vert, coord), i, diff, a, b


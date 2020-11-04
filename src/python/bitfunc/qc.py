# Copyright (c) 2009-2011, Dan Bravender <dan.bravender@gmail.com>

# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.

# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

import random
import os
import functools

def bools():
  """
  Endlessly yields random booleans
  """
  yield False
  yield True
  while True:
    if random.randint(0, 1) == 0:
      yield False
    else:
      yield True

def integers(low=0, high=100):
    '''Endlessly yields random integers between (inclusively) low and high.
       Yields low then high first to test boundary conditions.
    '''
    yield low
    yield high
    while True:
        yield random.randint(low, high)

def floats(low=0.0, high=100.0):
    '''Endlessly yields random floats between (inclusively) low and high.
       Yields low then high first to test boundary conditions.
    '''
    yield low
    yield high
    while True:
        yield random.uniform(low, high)

def lists(items=integers(), size=(0, 100)):
    '''Endlessly yields random lists varying in size between size[0]
       and size[1]. Yields a list of the low size and the high size
       first to test boundary conditions.
    '''
    yield [items.next() for _ in xrange(size[0])]
    yield [items.next() for _ in xrange(size[1])]
    while True:
        yield [items.next() for _ in xrange(random.randint(size[0], size[1]))]

def tuples(items=integers(), size=(0, 100)):
    '''Endlessly yields random tuples varying in size between size[0]
       and size[1]. Yields a tuple of the low size and the high size
       first to test boundary conditions.
    '''
    yield tuple([items.next() for _ in xrange(size[0])])
    yield tuple([items.next() for _ in xrange(size[1])])
    while True:
        yield tuple([items.next() for _ in xrange(random.randint(size[0], size[1]))])

def key_value_generator(keys=integers(), values=integers()):
    while True:
        yield [keys.next(), values.next()]

def dicts(key_values=key_value_generator(), size=(0, 100)):
    while True:
        x = {}
        for _ in xrange(random.randint(size[0], size[1])):
            item, value = key_values.next()
            while item in x:
                item, value = key_values.next()
            x.update({item: value})
        yield x

def unicodes(size=(0, 100), minunicode=0, maxunicode=255):
    for r in (size[0], size[1]):
        yield unicode('').join(unichr(random.randint(minunicode, maxunicode)) \
                for _ in xrange(r))
    while True:
        yield unicode('').join(unichr(random.randint(minunicode, maxunicode)) \
                for _ in xrange(random.randint(size[0], size[1])))

def oneof(generators):
  """
  given a list of generators, endlessly yields elements each one chosen from a
  randomly chosen generator
  """
  while True:
    yield random.choice(generators).next()

characters = functools.partial(unicodes, size=(1, 1))

def objects(_object_class, _fields={}, *init_args, **init_kwargs):
    ''' Endlessly yields objects of given class, with fields specified
        by given dictionary. Uses given constructor arguments while creating
        each object.
    '''
    while True:
        ctor_args = [arg.next() for arg in init_args]
        ctor_kwargs = (dict((k, v.next()) for k, v in init_kwargs.iteritems()))
        obj = _object_class(*ctor_args, **ctor_kwargs)
        for k, v in _fields.iteritems():
            setattr(obj, k, v.next())
        yield obj

def forall(tries=100, **kwargs):
    def wrap(f):
        @functools.wraps(f)
        def wrapped(*inargs, **inkwargs):
            for _ in xrange(tries):
                random_kwargs = (dict((name, gen.next())
                                 for (name, gen) in kwargs.iteritems()))
                if forall.verbose or os.environ.has_key('QC_VERBOSE'):
                    from pprint import pprint
                    pprint(random_kwargs)
                random_kwargs.update(**inkwargs)
                f(*inargs, **random_kwargs)
        return wrapped
    return wrap
forall.verbose = False # if enabled will print out the random test cases

__all__ = ['integers', 'floats', 'lists', 'tuples', 'unicodes', 'characters', 'objects', 'forall']

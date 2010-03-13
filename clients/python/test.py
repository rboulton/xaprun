#!/usr/bin/env python

import xaprun
import unittest

def test():
    c = xaprun.LocalConnection()
    def cb(result):
        print repr(result)
    c.send(c.GET, 'version', '', cb)
    c.send(c.GET, 'version', '', cb)
    c.check(1.0)
    c.check(1.0)

if __name__ == '__main__':
    test()

#!/usr/bin/env python

import unittest
import xaprun

class TestConnections(unittest.TestCase):
    """Test that we can open various types of connection.

    """
    def test_local_connection(self):
        c = xaprun.LocalConnection()
        r = []
        def cb(result):
            r.append(result)
        c.send(c.GET, 'version', '', cb)
        c.send(c.GET, 'version', '', cb)
        self.assertEqual(r, [])
        c.check(1.0)
        self.assertEqual(r, [{'msg': '0.1', 'ok': 1}])
        r = []
        c.check(1.0)
        self.assertEqual(r, [{'msg': '0.1', 'ok': 1}])

    def test_sendwait(self):
        c = xaprun.LocalConnection()
        self.assertEqual(c.sendwait(c.GET, 'version', ''),
                         {'msg': '0.1', 'ok': 1})

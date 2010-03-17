# Copyright (c) 2010 Richard Boulton
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.

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

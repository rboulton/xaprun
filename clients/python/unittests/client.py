#!/usr/bin/env python

import unittest
import xaprun

class TestClient(unittest.TestCase):
    """Test basic operation of a client.

    """
    def test_client(self):
        c = xaprun.Client()
        self.assertEqual(c.version(), '0.1')

    def test_insert(self):
        c = xaprun.Client()
        db = c.db('test1')
        docid = db.insert({'text': 'hello world'})
        self.assertNotEqual(docid, '')

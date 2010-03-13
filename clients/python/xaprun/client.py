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
"""client.py: Client for xaprun

"""

import connection
from errors import ConnectionError

def check(response):
    """Check that a response is a dict, and has 'ok': 0.

    Raise an exception if not.

    """
    if not isinstance(response, dict):
        raise ConnectionError("Response is not a dict")
    if response.get('ok', 0) != 1:
        msg = response.get('msg', 'Unspecified error in server')
        raise ConnectionError(msg)
    return response

def item_from_response(response, key):
    """Get a single item from a response.

    """
    response = check(response)
    try:
        return response[key]
    except KeyError:
        raise ConnectionError("Expected key '%s' missing from response" % key)

class Client(object):
    def __init__(self, conn=None):
        if conn is None:
            conn = connection.LocalConnection()
        self.conn = conn

    def version(self, timeout=None):
        r = self.conn.sendwait(self.conn.GET, 'version', '', timeout)
        return item_from_response(r, 'msg')
        

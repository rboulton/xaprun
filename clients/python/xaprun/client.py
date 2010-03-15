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
import urllib
from utils import json

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

def quote(val):
    """Percent-quote a value, using no special safe characters.

    """
    return urllib.quote(val, safe='')

jsonseps = (',', ':')
def jsondumps(obj):
    return json.dumps(obj, separators=jsonseps)

class Client(object):
    def __init__(self, conn=None, timeout=None):
        """Create a client.

        - `conn` must be a valid connection to xaprun.

        - `timeout` is the timeout, in seconds, used for all accesses to
          xaprun.  This may be set to None to wait indefinitely.

        """
        if conn is None:
            conn = connection.LocalConnection()
        self.conn = conn
        self.timeout = timeout

    def _get(self, target):
        return check(self.conn.sendwait(self.conn.GET, target, '',
                                        self.timeout))

    def _put(self, target, payload):
        return check(self.conn.sendwait(self.conn.PUT, target, payload,
                                        self.timeout))

    def _putj(self, target, payload):
        payload = jsondumps(payload)
        return self._put(target, jsondumps(payload))

    def _post(self, target, payload):
        return check(self.conn.sendwait(self.conn.POST, target, payload,
                                        self.timeout))

    def _postj(self, target, payload):
        return self._post(target, jsondumps(payload))

    def _delete(self, target):
        return check(self.conn.sendwait(self.conn.DELETE, target, '',
                                        self.timeout))

    def version(self):
        r = self._get('version')
        return item_from_response(r, 'msg')

    def db(self, dbname):
        return Database(self, dbname)

class Database(object):
    def __init__(self, client, name):
        self.client = client
        self.name = name
        self.qname = quote(name)

    def get_schema(self):
        """Get the current schema of the database.

        Returns a Schema object.

        """
        r = self.client._get(self.qname + "/_schema")
        return Schema(item_from_response(r, 'schema'))

    def set_schema(self, schema):
        r = self.client._putj(self.qname + "/_schema", schema.schema)

    def insert(self, doc, docid='', type="default"):
        """Insert a document.

        If the docid is supplied, any existing document with the same docid
        will be replaced by the document.

        Returns the document id.

        Note that documents may take some time to become visible after this
        call.

        """
        if docid:
            target = self.qname + '/' + quote(type) + '/'
            r = self.client._postj(target, doc)
        else:
            target = self.qname + '/' + quote(type) + '/' + docid
            r = self.client._putj(target, doc)
        return item_from_response(r, 'docid')

    def delete(self, docid, type="default"):
        """Delete a document.

        If the document didn't exist, this has no effect, and does not return
        an error.

        Returns None.

        """
        target = self.qname + '/' + quote(type) + '/' + docid
        check(self.client._delete(target))

    def get(self, docid):
        """Get the document with the specified docid.

        If no such document is found, returns an empty document body.

        """
        target = self.qname + '/' + quote(type) + '/' + docid
        return item_from_response(self.client._get(target), 'doc')

class Schema(object):
    def __init__(self, schema):
        """Initialise from a dict of schema properties.

        """
        self.schema = schema

    def set_default_handler(self, action='text'):
        """Set the handler to use on a field with no configuration.

        """
        self.schema['default_action'] = action

    def get_default_handler(self):
        return self.schema['default_action']

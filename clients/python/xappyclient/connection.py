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
"""connection.py: A connection to xappy-server.

"""

import os
import select
import subprocess
import time

class ConnectionError(Exception):
    """Base class of all errors associated with a connection.

    """

class Connection(object):
    GET = 'G'
    POST = 'P'
    PUT = 'U'
    DELETE = 'D'

    def __init__(self):
        # IDs of messages we're waiting for a response from.
        self.pending = {}

        # The next ID to allocate.
        self.next_id = 0

        # Data which has been read from the connection but not yet processed.
        self.read_buf = ''

        # None if we're not currently reading a message, otherwise the length
        # of the message we're currently reading.
        self.read_msg_len = None

        # Subclasses set this to True when closed.
        self.closed = False

    def __del__(self):
        self.close()

    def send(self, method, target, payload, callback):
        """Send a message.

        The method should be one of Connection.GET, Connection.POST,
        Connection.PUT or Connection.DELETE.

        The target should be url quoted (eg, with urllib.quote), and must not
        contain any spaces.

        """
        if self.closed:
            callback({'ok': 0, 'msg': 'Connection closed'})
            return
        assert method in 'GPUD' and len(method) == 1
        assert ' ' not in target
        msgid = str(self.next_id)
        msg = msgid + " " + method + target + " " + payload
        self.pending[msgid] = callback
        self.next_id += 1
        self._write(str(len(msg)) + " " + msg)

    def check(self, timeout=0.0):
        """Check for a response from the server.

        This will return after the timeout is reached, or after a response is
        received.

        It will call the callback associated with the response, if one if
        found.

        `timeout` is the time to wait for responses, in seconds.

        """
        endtime = time.time() + timeout
        looped = False
        need_more_data = False
        while True:
            if looped:
                # Check the timeout on subsequent passes round the loop
                if time.time() >= endtime:
                    return
            else:
                looped = True

            if need_more_data:
                self.read_buf += self._read(1024, endtime)
                need_more_data = False

            if self.read_msg_len is None:
                # Skip over any initial whitespace
                i = 0
                while i < len(self.read_buf) and self.read_buf[i] == ' ':
                    i += 1
                if i != 0:
                    self.read_buf = self.read_buf[i:]
                i = 0

                # Read the length of a response
                i = self.read_buf.find(' ')
                if i == -1:
                    need_more_data = True
                    continue

                while i < len(self.read_buf) and self.read_buf[i] != ' ':
                    i += 1
                if i == len(self.read_buf):
                    need_more_data = True
                    continue
                assert self.read_buf[i] == ' '

                try:
                    self.read_msg_len = int(self.read_buf[:i])
                except ValueError:
                    raise ConnectionError("Invalid value read")
                self.read_buf = self.read_buf[i + 1:]

            if len(self.read_buf) >= self.read_msg_len:
                self._handle_message(self.read_buf[:self.read_msg_len])
                self.read_buf = self.read_buf[self.read_msg_len:]
                self.read_msg_len = None
                return
            else:
                need_more_data = True

    def _handle_message(self, buf):
        i = buf.find(' ')
        if i == -1:
            raise ConnectionError("Invalid response message - no message id")
        msgid = buf[:i]
        buf = buf[i + 1:]
        cb = self.pending.get(msgid, None)
        if cb is None:
            print msgid, self.pending
            raise ConnectionError("Response for unknown message id")
        del self.pending[msgid]
        try:
            response = ''
            if len(buf) > 0:
                if buf[0] == 'S':
                    response = {'ok': 1, 'msg': buf[1:]}
                elif buf[0] == 'J':
                    response = json.loads(buf[1:])
                else:
                    response = {'ok': 0,
                        'msg': "Unknown response type code (%r)" % buf[0]}
            cb(response)
        except:
            cb()

    def close(self):
        """Close the connection to the server.

        """
        self._close()
        self._cleanup()

    def _cleanup(self):
        for callback in self.pending.itervalues():
            callback({'ok': 0, 'msg': 'Connection closed'})
        self.pending = {}
        self.closed = True

    def _write(self, data):
        """Write some data to the server.

        Note - this may block, if the server hasn't been keeping up with input.

        """
        raise NotImplementedError

    def _read(self, maxbytes, endtime):
        """Read some bytes from the server.

        This must return as soon as any bytes are read, or the endtime is
        reached.  It returns the bytes read (an empty string if no bytes were
        read).

        `endtime` is the time to return at, if no responses were returned.

        """
        raise NotImplementedError

    def _close(self):
        raise NotImplementedError

class LocalConnection(Connection):
    def __init__(self):
        Connection.__init__(self)
        cmd = ['./xappy-server', '--stdio']
        try:
            self.process = subprocess.Popen(cmd,
                                            bufsize=0,
                                            stdin=subprocess.PIPE,
                                            stdout=subprocess.PIPE,
                                            close_fds=True)
        except OSError, e:
            raise ConnectionError("Couldn't start local instance of "
                                  "xappy-server: %s" % str(e))
        self.write_fd = self.process.stdin.fileno()
        self.read_fd = self.process.stdout.fileno()

    def _close(self):
        if hasattr(self, 'process'):
            if self.process is not None:
                self.process.stdin.close()
                self.process.stdout.close()
                self.process.wait()
                self.process = None

    def _write(self, data):
        os.write(self.write_fd, data)

    def _read(self, maxbytes, endtime):
        timeout = endtime - time.time()
        if timeout < 0:
            timeout = 0
        ready_rfds = select.select([self.read_fd], [], [], timeout)[0]
        if ready_rfds == [self.read_fd]:
            return os.read(self.read_fd, maxbytes)
        return ''

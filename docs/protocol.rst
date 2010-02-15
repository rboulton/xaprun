Messages sent to the server are in the following format:

 - [0-9]+ the decimal length of the following message, in bytes.
 - a single space character.
 - the body of the message, of length exactly as given in the initial length.

Whitespace (including \n and \r) between messages is ignored.

Message bodies consist of the following:

 - [^ ]+ a message identifier, urlquoted, to be returned with responses.
 - a single space character.
 - the target of the message, urlquoted.
 - optionally:
   - a single space character.
   - the payload of the message.

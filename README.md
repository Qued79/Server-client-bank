# Server-client-bank

Work in progress...

This program is a multi-processed multi-threaded server/client bank implementation.


Some notes:
  The server side has no input as it is designed as a persistent server that is either up and down.
    In the future, I plan on adding a server side I/O for controlling the server, but for now it just outputs the current
    state of the bank (each of the accounts and their balances). The data structure is in shared memory and persists if
    the server crashes. 
    The polling is done very simply by passing the word 'thump' back and forth between server and client. I am sure that
      it could be made more secure, but I don't know that it really needs to be (since if they were to pass thump it
      would be ignored, and if they could block the thump signal they could conceivably block all signals so it wouldn't
      really change anything if the word passed was more complicated). Any break in this stream is considered a disconnect
      and the client closes while the server terminates the session. The only hangup is that the client is still in the
      session (its flag is set to 1 instead of 0), that data is in a mutex protected data structure, and the disconnect
      is implemented as a SIGALRM handler. Since I can't change the mutex inside of a handler, I've decided to fo the safe
      route for now and just leave the account 'in session'. I'm working around this with a semaphore in the handler, and
      select instead of read [the read block is preventing the semaphore check] but haven't fully worked it out yet.
    The style is currently awful and will be fixed as I update the code.

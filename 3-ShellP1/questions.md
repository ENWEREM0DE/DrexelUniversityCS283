1. How does the remote client determine when a command's output is fully received from the server, and what techniques can be used to handle partial reads or ensure complete message transmission?

_I think the remote client determines when command output is complete by looking for the EOF character (0x04) that the server sends. In my implementation, I use a loop to keep reading from the socket until I see this special character, which tells me the server is done sending. I think this helps handle partial reads since TCP might split up the data into multiple packets._

2. This week's lecture on TCP explains that it is a reliable stream protocol rather than a message-oriented one. Since TCP does not preserve message boundaries, how should a networked shell protocol define and detect the beginning and end of a command sent over a TCP connection? What challenges arise if this is not handled correctly?

_The key challenge is knowing when a message starts and ends since TCP just sees everything as a stream of bytes. In my shell protocol, I use null-terminated strings for commands going to the server and an EOF character for responses coming back. I think without these markers, we could have issues like commands getting mixed together or partial commands being executed._

3. Describe the general differences between stateful and stateless protocols.

_Stateful protocols maintain information about previous interactions - like keeping track of a login session. Stateless protocols, treat each request independently without remembering previous ones. I think stateful is more complex but can be more efficient since you don't need to resend information._

4. Our lecture this week stated that UDP is "unreliable". If that is the case, why would we ever use it?

_UDP is useful when speed is more important than reliability - like in video streaming or gaming. I think it's better to occasionally miss a frame than to pause the stream waiting for retransmission. UDP is also simpler and has less overhead since it doesn't need to track packet delivery._

5. What interface/abstraction is provided by the operating system to enable applications to use network communications?

_The operating system provides sockets as the main interface for network communication. In my code, I use socket functions like socket(), bind(), listen(), accept(), and connect() to handle network operations. I think sockets provide a consistent way to handle both local and network communication._
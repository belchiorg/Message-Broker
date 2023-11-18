# _Message Broker_

The second exercise of the project aims to build a simple message publishing and subscription system, where messages are stored in the file system [TecnicoFS](https://github.com/belchiorg/simple-fs).
The system will have an autonomous server process to which different client processes can connect to publish or receive messages in a given message storage box.

## 1. System Architecture

The system consists of the server (_mbroker_) and various publishers (_publishers_), subscribers (_subscribers_), and managers (_managers_).

### Message Boxes

A fundamental concept of the system is message boxes.
Each box can have one publisher and multiple subscribers.
The _publisher_ places messages in the box, and various _subscribers_ read the messages from the box.
Each box is supported on the server by a file in TFS.
For this reason, the life cycle of a box is distinct from the life cycle of the _publisher_ who publishes messages there.
Moreover, it is possible for a box to have multiple _publishers_ throughout its existence, although only one at a time.

Box creation and removal operations are managed by the _manager_.
Additionally, the _manager_ allows listing the existing boxes in the _mbroker_.

### 1.1. Server

The server incorporates TecnicoFS and is an autonomous process, initialized as follows:

```sh
$ mbroker <pipename> <max_sessions>
```

The server creates a _named pipe_ whose name (_pipename_) is indicated in the above argument.
It is through this _named pipe_, created by the server, that client processes can connect to register.

Any client process can connect to the server's _named pipe_ and send a message requesting the start of a session.
A **session** consists of having a client's _named pipe_, where the client sends messages (if it is a publisher) or where the client receives messages (if it is a subscriber).
A given client assumes only one of the two roles, i.e., it is either exclusively a publisher and only sends information to the server, or it is exclusively a subscriber (or manager) and only receives information.

The session's _named pipe_ must be created in advance by the client.
In the registration message, the client sends the name of the _named pipe_ to use during the session.

A session remains open until one of the following situations occurs:

1. A client (publisher or subscriber) closes its _named pipe_, implicitly signaling the end of the session;
2. The box is removed by the manager.

The server accepts a maximum number of simultaneous sessions, defined by the `max_sessions` argument.

In the following subsections, we describe the client-server protocol in more detail, i.e., the content of the request and response messages exchanged between clients and the server.

#### 1.1.1. Server Architecture

The server must have a thread to manage the registration _named pipe_ and launch `max_sessions` threads to process sessions.
When a new registration request arrives, it should be sent to an available thread, which will process it for the necessary time.
To manage these requests, avoiding threads from being in active wait, the _main thread_ and the _worker threads_ cooperate using a **producer-consumer queue**, according to the interface provided in the `producer-consumer.h` file.
Thus, when a new registration request arrives, it is placed in the queue, and as soon as a thread becomes available, it will consume and process that request.

The server's architecture is summarized in the following figure:

![](img/architecture_proj2.png)

- The _mbroker_ uses TFS to store messages from the boxes;
- The _main thread_ receives requests through the _register pipe_ and places them in a producer-consumer queue;
- The _worker threads_ execute client requests, dedicating themselves to serving one client at a time;
- They cooperate with the _main thread_ through a producer-consumer queue, avoiding active waiting.

### 1.2. Publisher

A publisher is a process launched as follows:

```sh
pub <register_pipe> <pipe_name> <box_name>
```

Once launched, the _publisher_ requests to start a session on the _mbroker_ server, indicating the message box to which it intends to write messages.
If the connection is accepted (it can be rejected if there is already a _publisher_ connected to the box, for example), it starts receiving messages from `stdin` and then publishes them.
A **message** corresponds to a line from `stdin`, truncated to a given maximum value and delimited by a `\0`, like a C string.
The message should not include a final `\n`.

If the _publisher_ receives an EOF (End Of File, for example, with a Ctrl-D), it should end the session by closing the _named pipe_.

The session's _named pipe_ is automatically chosen by the _publisher_ to ensure there are no conflicts with other concurrent clients.
The _named pipe_ must be removed from the file system after the end of the session.

### 1.3. Subscriber

A subscriber is a process launched as follows:

```sh
sub <register_pipe> <pipe_name> <box_name>
```

Once launched, the _subscriber_:

1. Connects to _mbroker_, indicating which message box it wants to subscribe to;
2. Collects the messages already stored there and prints them one by one to `stdout`, delimited by `\n`;
3. Listens for new messages;
4. Prints new messages when they are written to the _named pipe_ for which it has an open session.

To terminate the _subscriber_, it must handle `SIGINT` properly (i.e., Ctrl-C), closing the session and printing to `stdout` the number of messages received during the session.

The session's _named pipe_ is automatically chosen by the _subscriber_ to ensure there are no conflicts with other concurrent clients.
The _named pipe_ must be removed from the file system after the end of the session.

### 1.4. Manager

A manager is a process launched in one of the following ways:

```sh
manager <register_pipe> <pipe_name> create <box_name>
manager <register_pipe> <pipe_name> remove <box_name>
manager <register_pipe> <pipe_name> list
```

Once launched, the _manager_:

1. Sends the request to _mbroker_;
2. Receives the response in the _named pipe_ created by the manager itself;
3. Prints the response and terminates.

The session's _named pipe_ is automatically chosen by the _manager_ to ensure there are no conflicts with other concurrent clients.
The _named pipe_ must be removed from the file system before the _manager_ terminates.

### 1.5. Execution Examples

A first **example** considers the **sequential** operation of clients:

1. A manager creates the `bla` box;
2. A publisher connects to the same box, writes 3 messages, and disconnects;
3. A subscriber connects to the same box and starts receiving messages;
4. Receives all three, one at a time, and then waits for more messages.

In a second, more interesting **example**, there will be **concurrency** between clients:

1. A publisher connects;
2. Meanwhile, a subscriber for the same box also connects;
3. The publisher places

 messages in the box, and these are immediately delivered to the subscriber, remaining registered in the file;
4. Another subscriber connects to the same box and starts receiving all messages from the beginning of its subscription;
5. Now, when the publisher writes a new message, both subscribers receive the message directly.

## 2. Protocol

To moderate the interaction between the server and clients, a protocol is established, defining how messages are serialized, i.e., how they are arranged in a byte buffer.
This type of protocol is sometimes referred to as a _wire protocol_, alluding to the data that effectively circulates in the transmission medium, which in this case, will be the _named pipes_.

The content of each message should follow the following format, where:

- The symbol `|` denotes the concatenation of elements in a message;
- All request messages are initiated by a code that identifies the requested operation (`OP_CODE`);
- The strings carrying the names of _named pipes_ are of fixed size, indicated in the message.
  In the case of names of smaller size, additional characters should be filled with `\0`.

### 2.1. Registration

The server's _named pipe_, which only receives registrations from new clients, must receive messages of the following type:

Publisher registration request:

```
[ code = 1 (uint8_t) ] | [ client_named_pipe_path (char[256]) ] | [ box_name (char[32]) ]
```

Subscriber registration request:

```
[ code = 2 (uint8_t) ] | [ client_named_pipe_path (char[256]) ] | [ box_name (char[32]) ]
```

Box creation request:

```
[ code = 3 (uint8_t) ] | [ client_named_pipe_path (char[256]) ] | [ box_name (char[32]) ]
```

Response to the box creation request:

```
[ code = 4 (uint8_t) ] | [ return_code (int32_t) ] | [ error message (char[1024]) ]
```

The return code should be `0` if the box was successfully created and `-1` in case of an error.
In case of an error, the error message is sent (otherwise, it is simply initialized with `\0`).

Box removal request:

```
[ code = 5 (uint8_t) ] | [ client_named_pipe_path (char[256]) ] | [ box_name (char[32]) ]
```

Response to the box removal request:

```
[ code = 6 (uint8_t) ] | [ return_code (int32_t) ] | [ error message (char[1024]) ]
```

Box listing request:

```
[ code = 7 (uint8_t) ] | [ client_named_pipe_path (char[256]) ]
```

The response to the box listing comes in several messages, of the following type:

```
[ code = 8 (uint8_t) ] | [ last (uint8_t) ] | [ box_name (char[32]) ] | [ box_size (uint64_t) ] | [ n_publishers (uint64_t) ] | [ n_subscribers (uint64_t) ]
```

The `last` byte is `1` if this is the last box in the listing and `0` otherwise.
`box_size` is the size (in _bytes_) of the box, with `n_publisher` (`0` or `1`) indicating whether a _publisher_ is connected to the box at that moment, and `n_subscriber` the number of subscribers to the box at that moment.

### 2.2 Publisher

The _publisher_ sends messages to the server of the following type:

```
[ code = 9 (uint8_t) ] | [ message (char[1024]) ]
```

### 2.3 Subscriber

The server sends messages to the _subscriber_ of the following type:

```
[ code = 10 (uint8_t) ] | [ message (char[1024]) ]
```

## 3. Implementation Requirements

### 3.1. Client Handling

When the server starts, it launches a set of `S` tasks (thread pool) waiting for registration requests to process, which they will receive through the producer-consumer queue.
The _main thread_ manages the registration _named pipe_ and places the registration requests in the producer-consumer queue.
When a thread ends a session, it waits for a new session to process.

### 3.2 Storage Boxes

Messages received by the server should be placed in a box.
In practice, a box corresponds to a file in TecnicoFS.
The file must be created when the box is created by the manager and deleted when the box is removed.
All received messages are written to the end of the file, separated by `\0`.

In summary, messages accumulate in the boxes.
When a subscriber connects to a box, the corresponding file is opened, and the messages begin to be read from the beginning (even if the same subscriber or another has received them before).
Subsequent messages generated by the _publisher_ of a box should also be delivered to the subscribers of the box.
This functionality should be implemented using **condition variables** to avoid active waits.

### 3.3 Message Formatting

To standardize the output of various commands (to `stdout`), the format in which they should be printed is provided.

### 3.4 Producer-Consumer Queue

The producer-consumer queue is the most complex synchronization structure in the project.
Therefore, this component will be evaluated in isolation (i.e., there will be tests that use only the interface described in `producer-consumer.h`) to ensure its correctness.
As such, the interface of `producer-consumer.h` should not be changed.

Apart from that, groups are free to change the base code as they see fit.

#### Subscriber Messages

```c
fprintf(stdout, "%s\n", message);
```

#### Box Listing

Each line of the box listing should be printed as follows:

```c
fprintf(stdout, "%s %zu %zu %zu\n", box_name, box_size, n_publishers, n_subscribers);
```

Boxes should be sorted in alphabetical order, and it is not guaranteed that the server will send them in that order (i.e., the client must sort the boxes before printing them).

### 3.5 Active Waiting

In the project, active waiting mechanisms should never be used.

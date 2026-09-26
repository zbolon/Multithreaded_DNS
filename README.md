# Multithreaded DNS Resolver

A multithreaded DNS resolver written in C that uses a **producer-consumer architecture**, a **bounded circular queue**, and **POSIX threads (`pthreads`)** to concurrently process hostname lookups.

The program accepts multiple input files containing hostnames, distributes the files among requester threads, and uses resolver threads to perform DNS lookups concurrently. Thread synchronization is handled with mutexes and condition variables to safely coordinate access to shared data.

## Features

* Concurrent hostname processing using POSIX threads
* Producer-consumer architecture
* Bounded circular queue for communication between threads
* Configurable number of requester and resolver threads
* Thread-safe access to shared log files
* Thread-safe distribution of input files among requester threads
* DNS resolution using `getaddrinfo()`
* Reports unresolved hostnames
* Per-thread processing statistics and execution times
* Supports up to 10 requester threads, 10 resolver threads, and 100 input files

## How It Works

The program is divided into two types of worker threads:

### Requester Threads

Requester threads act as **producers**.

Each requester:

1. Claims an unprocessed input file using a shared file index.
2. Reads hostnames from the file.
3. Places each hostname into the shared circular queue.
4. Records the hostname in the requester log.
5. Continues until all input files have been processed.

A mutex protects the shared file index so that multiple requester threads cannot process the same file.

### Resolver Threads

Resolver threads act as **consumers**.

Each resolver:

1. Waits for a hostname to become available in the shared queue.
2. Removes a hostname from the queue.
3. Performs a DNS lookup using `getaddrinfo()`.
4. Writes the hostname and resulting IP address to the resolver log.
5. Records `NOT_RESOLVED` if the lookup fails.
6. Continues until all requester threads have finished and the queue is empty.

## Producer-Consumer Queue

The requester and resolver threads communicate through a shared **circular queue with a capacity of 8 hostnames**.

The queue uses:

* A mutex to protect queue state
* A `not_full` condition variable for producers
* A `not_empty` condition variable for consumers
* `head`, `tail`, and `count` values to manage the circular buffer

When the queue is full, requester threads wait until space becomes available.

When the queue is empty, resolver threads wait until a requester adds another hostname.

Once all requester threads have finished, resolver threads can determine that no additional work will be produced and safely terminate when the queue becomes empty.

## Synchronization

Several shared resources require synchronization:

| Shared Resource  | Synchronization             |
| ---------------- | --------------------------- |
| Circular queue   | Mutex + condition variables |
| Input file index | Mutex                       |
| Requester log    | Mutex                       |
| Resolver log     | Mutex                       |
| Standard output  | Mutex                       |
| Standard error   | Mutex                       |

This prevents race conditions when multiple threads access the same resource concurrently.

## Building

### Requirements

* GCC
* POSIX threads
* GNU Make
* Linux/macOS environment with pthread support

### Compile

Clone the repository and build the program:

```bash
git clone https://github.com/zbolon/Multithreaded_DNS.git
cd Multithreaded_DNS
make
```

This produces the executable:

```text
multi-lookup
```

To remove compiled object files and the executable:

```bash
make clean
```

## Usage

```text
./multi-lookup <requester threads> <resolver threads> <requester log> <resolver log> <input files...>
```

### Arguments

| Argument            | Description                                 |
| ------------------- | ------------------------------------------- |
| `requester threads` | Number of producer/requester threads        |
| `resolver threads`  | Number of consumer/resolver threads         |
| `requester log`     | File where processed hostnames are recorded |
| `resolver log`      | File where DNS results are recorded         |
| `input files`       | One or more files containing hostnames      |

### Example

```bash
./multi-lookup 4 4 requester.log resolver.log input/names1.txt input/names2.txt
```

Multiple input files can be supplied:

```bash
./multi-lookup 4 6 requester.log resolver.log input/*.txt
```

The program supports a maximum of:

* **10 requester threads**
* **10 resolver threads**
* **100 input files**

## Output

### Requester Log

The requester log records the hostnames read from the input files.

```text
example.com
google.com
github.com
```

### Resolver Log

The resolver log contains the hostname and its resolved IP address:

```text
example.com, 93.184.216.34
google.com, 142.250.72.14
github.com, 140.82.112.4
```

If a hostname cannot be resolved:

```text
example.invalid, NOT_RESOLVED
```

### Thread Statistics

The program also reports how much work each thread completed and how long it took:

```text
Thread <12345> serviced 3 files in 0.012345 seconds
Thread <12346> resolved 24 hosts in 0.018732 seconds
```

## Project Structure

```text
Multithreaded_DNS/
├── input/
│   └── Input hostname files
├── array.c
├── array.h
├── multi-lookup.c
├── multi-lookup.h
├── util.c
├── util.h
├── Makefile
└── README.md
```

### `multi-lookup.c`

Contains the main program and the requester/resolver thread implementations.

* Creates requester and resolver threads
* Distributes input files among requester threads
* Coordinates thread execution
* Performs DNS lookups
* Writes logging and performance information

### `array.c`

Implements the shared circular queue.

* Queue initialization
* Adding hostnames
* Removing hostnames
* Thread synchronization
* Queue cleanup

### `array.h`

Defines the shared queue data structure, including the queue state, mutexes, condition variables, and synchronization state.

### `multi-lookup.h`

Defines constants and the data structures used to pass information to requester and resolver threads.

### `util.c` / `util.h`

Provides the DNS lookup utility used by the resolver threads.

### `Makefile`

Automates compilation and cleanup using GCC and the pthread library.

## Technologies

* **C**
* **POSIX Threads (`pthreads`)**
* **Mutexes**
* **Condition Variables**
* **Producer-Consumer Pattern**
* **Circular Queue**
* **DNS / `getaddrinfo()`**
* **GCC**
* **GNU Make**

## Concepts Demonstrated

This project focuses on several systems programming and operating systems concepts:

* Multithreading
* Thread synchronization
* Race-condition prevention
* Mutual exclusion
* Condition variables
* Producer-consumer systems
* Shared memory
* Circular buffers
* Concurrent file I/O
* DNS resolution
* Resource management
* Thread lifecycle management

## Project Background

This project was originally developed as an assignment for a Design and Analysis of Operating Systems course.

The core implementation was developed from scratch, with the exception of the provided `util.c` and `util.h` DNS utility files.

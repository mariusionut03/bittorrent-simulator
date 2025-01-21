# Bittorrent Protocol Simulator

## Overview
This project simulates the Bittorrent protocol using MPI (Message Passing Interface) for communication between peers or with the tracker. The simulation includes all the functionalities for the tracker, files integrity checks, swarm updates and so on. The downloads/uploads of file segments are simulated using two signals: ACK and NAK.

## Components
- **tracker.cpp**: Handles some of the tracker's operations, receiving the initial data about the files and sending the updated swarms when requested from peers. 
- **client.cpp**: Handles the client's operations, including reading files, sending initial data to the tracker, requesting file swarms, and writing output files.
- **bittorrent.cpp**: Contains the main logic for the simulation, including the creation of download and upload threads for each peer, communication logic.
- **bittorrent.h**: Header file defining constants, data structures, and global variables used across the project.

## Features
- **File Initialization**: Clients read input files and send initial data to the tracker. These are the hashes representing each segment of the file.
- **File Requesting**: Clients request file swarms from the tracker and download missing segments from other peers.
- **File Uploading**: Clients respond to file segment requests from other peers. They should send the file segment if they have it, but in this simulation they just send a signal, ACK/NAK. Receiving NAK requests the segment to another peer.
- **Multi-threading**: Uses pthreads for handling concurrent download and upload operations.
- **MPI Communication**: Utilizes MPI for message passing between clients and the tracker.
- **Dynamic swarm update**: A peer can upload a file he partially owns, being added to the swarm when requesting it initially.

## How to Run
1. **Install MPI**: Ensure that MPI is installed on your system.
2. **Compile the Code**: Use `mpic++` to compile the source files.
   ```sh
   mpic++ -o bittorrent bittorrent.cpp -lpthread
   ```
   or use the included Makefile:
   ```sh
   make build
   ```
3. **Add file informations in tests/input**: Create your own input files or choose one from the tests directory. Check "Example" below for file structure.
4. **Run the Simulation**: Use mpirun to run the compiled executable with the desired number of processes.
   ```sh
   mpirun --oversubscribe -np <number_of_processes> ./bittorrent
   ```
   <number_of_processes> should be the number of input files + 1 (the last one being the tracker).

   Example: We have 3 files (in1.txt, in2.txt, in3.txt) representing 3 clients. We need 3 clients + 1 tracker (4 processes). Run:
   ```sh
   mpirun --oversubscribe -np 4 ./bittorrent
   ```
5. **Check the tests/output folder**: Make sure all the files received the files as requested (After a peer successfully receives all the ACKs for a file, all the hashes are written in order inside a client\<R\>_\<file_name\> file).

6. **Remove input/output files and executable**:
    ```sh
    make clean
    make clear-tests
    ```

## Example
1. Input Files: Place input files in the input directory, named as in<rank>.txt where <rank> is the rank of the client.

    Format:
    ```sh
    number_owned_files
    name_owned_file_1 number_segments_owned_file_1
    segment1_owned_file_1
    segment2_owned_file_1
    ...
    name_owned_file_2 number_segments_owned_file_2
    segment1_owned_file_2
    segment2_owned_file_2
    ...
    number_requested_files
    name_requested_file_1
    name_requested_file_2
    ...
    ```


2. Output Files: Output files will be generated in the output directory, named as client\<rank\>_\<filename\>.

    Format:
    ```sh
    file_hash_1
    file_hash_2
    ...
    file_hash_N
    ```

## Acknowledgements

The tests for this project are created by UPB's Moby Labs Team.

## License

This project is licensed under the MIT License.

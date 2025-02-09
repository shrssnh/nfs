We are assuming that storage server is not managed manually by the user when it is running.

ERROR CODES:

Assumption: Storage server needs to be initialized with a root directory where all the data for that server will be
stored. It will not be created automatically to avoid excessive control over the file system.


- 001: No root directory found on a storage server
- 002: Multiple root directories found
- 003: Unable to send data over the network
- 004: Issues with polling for clients
- 005: Error receiving request, asking to resend
- 006: Invalid Request
- 008: Memory exceeded


Functionality:
Read: READ <Path>
Write: WRITE <Path>
Copy: COPY <Src> <Dest>
Create: CREATE <Path> <0|1 (0 for FIle, 1 for directory)>
Delete: DELETE <Path>
List: LIST <Path>
Get Additional Information: GETINFO <Path>

Things that have been implemented:
Full intialization
Multithreading
Multiple Clients
LRU Caching (multithreaded)
Multiple clients can read files but file writing is atomic
Error Handling 
Bookkeeping



Signing posts on the internet is like wisiting the 



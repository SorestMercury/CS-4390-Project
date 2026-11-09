4390 Group Project
Members: Archer, Christopher, Dylan, Samara

To do:

* Implement 7 functions in the tracker

  * xtrct\_fname
  * tokenize\_createmsg- Dylan
  * tokenize\_updatemsg- Dylan
  

* Peer

In progress:

* Peer Downloading

Completed:

* Download and upload C files
* declared previously undeclared headers

* Peer

  * create tracker- Archer
  * update tracker- Archer
  * User input and output - Archer
  * Stop program - Archer
  * REQ LIST- Christopher
  * GET- Christopher

* Tracker

  * handle\_list\_req - Samara
  * handle\_createtracker\_req - Dylan
  * handle\_updatetracker\_req - Dylan

* messages

Work Log:

* Group meeting to meet and discuss the project (10/23/26)
* Dylan uploaded starter C files (10/28/26)
* Group meeting to work on code and discuss future progress (10/30/26)
* Dylan set up initial messaging between client and server (11/03/26)
* Dylan fully implemented handling the createtracker and updatetracker commands on the server side (11/06/26)
* Christopher implemented the GET command on the client/server (11/7/26)
* Samara implemented the LIST command on the client/server (11/7/26)
* Archer implemented the shell-style interface where the user can manually enter commands to run (11/8/26)
  * Replacing hard-coded test commands with actual user input
  * Added some helpful features (like automatic md5 calculation) to server createtracker/updatetracker requests
* Dylan implemented the HOST and DOWNLOAD functionalities that enable peers to download files (specified byte ranges) from each other (11/8/26)
  * commands are currently entered manually. Later, createtracker/updateracker will automatically run a HOST command from the client for whatever byte range they have. GET will automatically call DOWNLOAD once a tracker file has been detected inside of cache/.

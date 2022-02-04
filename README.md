# SessionExecutioner

  ## SessionExecutioner is a project that allow you to run executables as the active user in the local computer.

  ## Usage
  
  * SessionExecutioner.exe <PathToFile>
  
  ## Notes
  
  * Should be run as SYSTEM.
  * Run with "TaskScheduler" and set "Trigger" to "At Log On" to run processes on every logged on user to the local computer.

  ## How does it works?
  
  * Iterate over existing sessions to find the active session.
  * Iterate over existing processes to find the process "explorer.exe" that belongs to the current active session.
  * Open the process and duplicate its Token.
  * Use the function CreateProcessAsUserW to run the process as the current active session.
  * Enjoy :)




<h1 align="center">Unix-Shell</h1>
<h3 align="center">Oprating Systems Course</h3>

<h2> Description: </h2>
<h3>Smash - Small Shell </h3>
<h3> 
  A unix shell (bash-like), which behaves like a real Linux shell. <br>
  This shell implements only a limited subset of Linux shell commands. <br>
  For all other commands, it sends them to the original shell(bash). </h3>
<h3>Below is the list of implemented commands:</h3>
<h4>
  <ul>
    <li>Shell Commands: "chprompt", "showpid", "pwd", "cd", "jobs", "kill", "fg", "bg", "quit", "tail", "touch" and "timeout".</li>
    <li>Pipes and IO Redirection: Supports ">", ">>", "|" and "|&".</li>
    <li>Handling signals: Supports "Ctrl + C" and "Ctrl + Z", for killing/stopping a process.</li>
    <li>Exit The Shell: "quit" command.</li>
</h4>

<h2>Download:</h2>

    git clone https://github.com/oradbarel/Unix-Shell.git
    
<h2> Build:</h2>
Once downloaded, do the following:

    cd Unix-Shell/src
    make smash
or you run `make smash` from your `src` folder.

<h2> Run:</h2>

  cd Unix-Shell/release
  ./smash
  
or you run `./smash` from your `release` folder.


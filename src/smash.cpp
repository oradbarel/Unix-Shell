#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include "signals.h"

//Constants:


//Code:

int main(int argc, char* argv[]) {
    if(signal(SIGTSTP , ctrlZHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if(signal(SIGINT , ctrlCHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }
    struct sigaction act = { 0 };
    act.sa_flags = SA_RESTART;
    act.sa_sigaction = reinterpret_cast<void (*)(int, siginfo_t *, void *)>(&alarmHandler);
    if (sigaction(SIGALRM, &act, NULL) == -1) {
        perror("smash error: failed to set alarm handler");
        exit(EXIT_FAILURE);
    }

    //TODO: setup sig alarm handler

    SmallShell& smash = SmallShell::getInstance();

    while(true) {
        std::cout << smash.getPrompt();
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        smash.executeCommand(cmd_line.c_str());
    }
    return 0;
}
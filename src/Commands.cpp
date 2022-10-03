#include "Commands.h"
#include <unistd.h>
#include <string.h>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include <time.h>
#include <utime.h>
#include <assert.h>
#include <algorithm>
#include <climits>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>


using namespace std;

#if 0
#define FUNC_ENTRY() \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT() \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

// Constants:

#define SMASH_DEFAULT_PROMPT   "smash> "
#define SMASH_MAX_PATH         2048
#define SMASH_BIG_PATH         128
#define SMASH_MAX_ARGS         21
#define DEFAULT_LINES_FOR_TAIL 10
#define SMASH_BASH_PATH        "/bin/bash"
#define SMASH_C_ARG            "-c"
#define TIME_SEPERATOR         ":"
#define YEARS_TO_SUBSTRACT     1900
#define MONTHS_TO_SUBSTRACT    1
#define DASH                   "-"
#define REDIRECTION_CHAR       ">"
#define PIPE_CHAR              "|"
#define STDERR_PIPE_PREFIX     "&"

#define CD        "cd"
#define OPEN      "open"
#define FG        "fg"
#define BG        "bg"
#define FORK      "fork"
#define KILL      "kill"
#define CHDIR     "chdir"
#define KILL      "kill"
#define FORK      "fork"
#define EXECV     "execv"
#define DUP       "dup"
#define CLOSE     "close"
#define READ      "read"
#define WRITE     "write"
#define PIPE      "pipe"
#define TAIL      "tail"
#define TOUCH     "touch"
#define UTIME     "utime"
#define MKTIME    "mktime"


#define SMASH_PRINT_WITH_PERROR(ERR, CMD)                       perror(ERR(CMD));
#define SMASH_PRINT_ERROR(ERR, CMD)                             cerr << ERR(CMD) << endl;
#define SMASH_PRINT_ERROR_JOB_ID(ERR_START, ERR_END, CMD, ID)   cerr <<  ERR_START(CMD) << ID << ERR_END << endl;
#define SMASH_PRINT_COMMAND_WITH_PID(CMD, PID)                  cout << CMD << " : " << PID << endl;
#define SMASH_SYSCALL_FAILED_ERROR(CMD)                         "smash error: " CMD " failed"
#define SMASH_INVALID_ARGS_ERROR(CMD)                           "smash error: " CMD ": invalid arguments"
#define SMASH_JOBS_LIST_EMPTY_ERROR(CMD)                        "smash error: " CMD ": jobs list is empty"
#define SMASH_NO_STOPPED_JOBS_IN_LIST(CMD)                      "smash error: " CMD ": there is no stopped jobs to resume"
#define SMASH_JOB_ID_DOES_NOT_EXISTS_ERROR_START(CMD)           "smash error: " CMD ": job-id "
#define SMASH_JOB_ID_DOES_NOT_EXISTS_ERROR_END                  " does not exist"
#define SMASH_JOB_JOB_ALREADY_IN_BG_ERROR_START(CMD)            "smash error: " CMD ": job-id "
#define SMASH_JOB_JOB_ALREADY_IN_BG_ERROR__END                  " is already running in the background"

// Code:

//--------- Course Staff Functions:

const std::string WHITESPACE = " \n\r\t\f\v";

string _ltrim(const std::string &s)
{
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string &s)
{
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string &s)
{
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char *cmd_line, char **args)
{
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for (std::string s; iss >> s;)
    {
        args[i] = (char *)malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
}

bool _isBackgroundComamnd(const char *cmd_line)
{
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

bool _isBackgroundComamnd(const string& cmd_line)
{
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char *cmd_line)
{
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos)
    {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&')
    {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

void _removeBackgroundSign(string& cmd_line)
{
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos){
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&'){
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

//--------- End of Course Staff Functions.


//--------- Here Start My Functions:

/**
 * removeFirstWord: Removes the first word from a given string and returns the remaining, with no spaces at the beginning and at the end of it.
 * If there are no more words(string is empty or has one word only), returns "".
 * @param line The given string
 * @return A string, as described.
 * @throw std::invalid_argument, if line points to NULL.
 */
string removeFirstWord(const char *line)
{
    if (!line)
    {
        throw std::invalid_argument("NULL argument");
    }
    string new_line = _trim(string(line));
    string first_word = new_line.substr(0, new_line.find_first_of(" \n"));
    return _trim(new_line.substr(first_word.length()));
}

/**
 * removeFirstWord: Removes the first word from a given string and returns the remaining, with no spaces at the beginning and at the end of it.
 * If there are no more words(string is empty or has one word only), returns "".
 * @param line The given string
 * @return A string, as described.
 * @throw std::invalid_argument, if line points to NULL.
 */
string removeFirstWord(const string& line)
{
    return removeFirstWord(line.c_str());
}

/**
 * getFirstWord: Gets the first word of a given string, with no spaces at the beginning and at the end of it.
 * If (string is empty, returns "".
 * @param line The given string
 * @return A string, as described.
 * @throw std::invalid_argument, if line points to NULL.
 */
string getFirstWord(const char *line)
{
    if (!line)
    {
        throw std::invalid_argument("NULL argument");
    }
    string cmd_s = _trim(string(line));
    return cmd_s.substr(0, cmd_s.find_first_of(" \n"));
}

/**
 * getFirstWord: Gets the first word of a given string, with no spaces at the beginning and at the end of it.
 * If (string is empty, returns "".
 * @param line The given string
 * @return A string, as described.
 * @throw std::invalid_argument, if line points to NULL.
 */
string getFirstWord(const string& line)
{
    return getFirstWord(line.c_str());
}

/**
 * splitStringByCharacter: Splits to 2 parts, by a given character
 *
 * @return A vector of those two parts of the strings. It also removes the spaces at the beginnig and the end of the parts.
 * @throw std::invalid_argument, if `cmd` argument or `ch` argument is empty.
 */
vector<string> splitCommandByCharacter(const string& cmd, const string& ch)
{
    if (ch.empty() || cmd.empty())
    {
        throw std::invalid_argument("INVALID ARGUMENT");
    }
    vector<string> vec;
    if(cmd.find_first_of(ch) == string::npos){
        string only_part = cmd;
        vec.push_back(cmd);
        return vec;
    }
    int index = cmd.find_first_of(ch);
    vec.push_back(_trim(cmd.substr(0, index)));// first part, without the ch.
    vec.push_back(_trim(cmd.substr(index+1))); // second part, without the ch.
    return vec;
}

/**
 * argToInt - Make a string to integer, if posible.
 * @param arg A non-empty string contains a number(and maybe a '-' sign).
 * @return An integer
 *@throw std::invalid_argument, in case of wrong argument(some other characters or an empty string).
 */
int argToInt(const string& number)
{
    string arg = number;
    //Check if empty:
    if(arg.empty()){
        throw std::invalid_argument("EMPTY STRING");
    }
    //Check if negative:
    bool negative = false;
    if(arg.substr(0,1).compare("-") == 0){
        negative = true;
        arg = arg.substr(1);
    }
    //Check again if empty:
    if(arg.length() == 0){
        throw std::invalid_argument("EMPTY STRING");
    }
    int num = 0;
    for (char c: arg){
        if (c >= '0' && c <= '9') {
            num = num * 10 + (c - '0');
        }
        else {
            throw std::invalid_argument("INVALID ARGUMENT");
        }
    }
    if(negative){
        num *= (-1);
    }
    return num;
}

/**
 * getTimeFromString
 * @param time_s A string in format: "ss:mm:hh:dd:mm:yyyy".
 * @return time vector in format:
 * seconds, minutes, hours, day, month and year
 */
vector<int> getTimeFromString(const string& time_s)
{
    vector<int> time_vec;
    string new_time_s = _trim(time_s);

    // exract with a loop, as long as we have more then 2 characters(if we reach "")
    while(!(new_time_s.empty())){
        int current = argToInt(splitCommandByCharacter(new_time_s, TIME_SEPERATOR)[0]);
        time_vec.push_back(current);
        if(new_time_s.find(TIME_SEPERATOR) == string::npos){
            break;
        }
        new_time_s = splitCommandByCharacter(new_time_s, TIME_SEPERATOR)[1];
    }
    return time_vec;
}


//-------- My implementation for classes in Commands.h:

//-------- SmallShell Class:

SmallShell::SmallShell() : prompt(SMASH_DEFAULT_PROMPT), old_pwd(""), jobs(), current_job(nullptr) {}

SmallShell::~SmallShell() {}

/**
 * CreateCommand: Creates and returns a pointer to Command class which matches the given command line (cmd_line)
 * @param cmd_line A const pointer to a string contains the command with the arguments.
 * @return A pointer to Command class with that command.
 */
Command *SmallShell::CreateCommand(const char *cmd_line)
{
    string cmd_s = _trim(string(cmd_line));

    // make a command class, accordingly:
    if (cmd_s.find(">") != string::npos)
    {
        return new RedirectionCommand(cmd_line);
    }
    else if (cmd_s.find("|") != string::npos)
    {
        return new PipeCommand(cmd_line);
    }
    // remove the '&', so we can read the first word correctly:
    if (cmd_s[cmd_s.length() - 1] == '&'){
        cmd_s = _trim(cmd_s.substr(0, cmd_s.length() - 1));
    }
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
    if (firstWord.compare("chprompt") == 0)
    {
        return new ChangePrompt(cmd_line);
    }
    else if (firstWord.compare("showpid") == 0)
    {
        return new ShowPidCommand(cmd_line);
    }
    else if (firstWord.compare("pwd") == 0)
    {
        return new GetCurrDirCommand(cmd_line);
    }
    else if (firstWord.compare("cd") == 0)
    {
        return new ChangeDirCommand(cmd_line);
    }
    else if (firstWord.compare("jobs") == 0)
    {
        return new JobsCommand(cmd_line, &(this->jobs));
    }
    else if (firstWord.compare("kill") == 0)
    {
        return new KillCommand(cmd_line, &(this->jobs));
    }
    else if (firstWord.compare("fg") == 0)
    {
        return new ForegroundCommand(cmd_line, &(this->jobs));
    }
    else if (firstWord.compare("bg") == 0)
    {
        return new BackgroundCommand(cmd_line, &(this->jobs));
    }
    else if(firstWord.compare("quit") == 0)
    {
        return new QuitCommand(cmd_line, &(this->jobs));
    }
    else if(firstWord.compare("tail") == 0)
    {
        return new TailCommand(cmd_line);
    }
    else if(firstWord.compare("touch") == 0)
    {
        return new TouchCommand(cmd_line);
    }
        // last resort - give command to bash itself:
    else{
        return new ExternalCommand(cmd_line);
    }
    return nullptr;
}

/**
 * executeCommand:
 * @param cmd_line A const pointer to a string contains the command with the arguments.
 */
void SmallShell::executeCommand(const char *cmd_line)
{
    if (_trim(string(cmd_line)).empty()){// skip empty commands.
        return;
    }
    string updatedCommand;
    string timed = getFirstWord(cmd_line);
    bool is_timed = false;
    int duration = 0;
    if (timed.compare("timeout") == 0)
    {
        is_timed = true;
        updatedCommand = removeFirstWord(cmd_line);
        duration = stoi(getFirstWord(updatedCommand));
        updatedCommand = removeFirstWord(updatedCommand.c_str());
        cmd_line = updatedCommand.c_str();
    }

    this->jobs.removeFinishedJobs();
    Command *cmd = CreateCommand(cmd_line);
    if (is_timed && cmd)
    {
        alarm(duration);
        cmd->is_timed = is_timed;
        cmd->duration = duration;
    }

    cmd->execute();
    delete cmd;
}

std::string SmallShell::getPrompt() const
{
    return this->prompt;
}

void SmallShell::setPrompt(const string &new_prompt)
{
    this->prompt = new_prompt;
}

std::string SmallShell::getOldPwd() const
{
    return this->old_pwd;
}

void SmallShell::setOldPwd(const std::string &prev_pwd)
{
    this->old_pwd = prev_pwd;
}



//-------- JobEntry Class:

JobsList::JobEntry::JobEntry(const int& id, const bool& isStopped, const std::string& cmd, const pid_t& pid, const bool& is_timed, const int& duration) :
        job_id(id), is_stopped(isStopped), complete_command(cmd), job_pid(pid), is_timed(is_timed), duration(duration)
{
    time_t time1;
    this->insert_time = time(&time1);
    if(time1 != this->insert_time){
        cout <<"time error" << endl;
    }
}

JobsList::JobEntry::JobEntry(const JobsList::JobEntry& other) :
        job_id(other.getId()), is_stopped(other.jobWasStopped()), complete_command(other.getCompleteCmd()), job_pid(other.getPid()), insert_time(other.getTime()) {}

JobsList::JobEntry& JobsList::JobEntry::operator=(const JobsList::JobEntry& other)
{
    if (this == &other)
    {
        return *this;
    }
    this->job_id = other.getId();
    this->is_stopped = other.jobWasStopped();
    this->complete_command = other.getCompleteCmd();
    this->job_pid = other.getPid();
    this->insert_time = other.getTime();

    this->is_timed = other.is_timed;
    this->duration = other.duration;

    return *this;

}

int JobsList::JobEntry::getId() const
{
    return this->job_id;
}

bool JobsList::JobEntry::jobWasStopped() const
{
    return this->is_stopped;
}

std::string JobsList::JobEntry::getCompleteCmd() const
{
    return this->complete_command;
}

pid_t JobsList::JobEntry::getPid() const
{
    return this->job_pid;
}

time_t JobsList::JobEntry::getTime() const
{
    return this->insert_time;
}
bool JobsList::JobEntry::getIsTimed()
{
    return this->is_timed;
}

int JobsList::JobEntry::getDuration()
{
    return this->duration;
}

void JobsList::JobEntry::setJobAsStopped()
{
    this->is_stopped = true;
}

void JobsList::JobEntry::setJobAsResumed()
{
    this->is_stopped = false;
}

bool JobsList::JobEntry::operator>(const JobsList::JobEntry& other)
{
    return this->getId() > other.getId();
}

bool JobsList::JobEntry::operator<(const JobsList::JobEntry& other)
{
    return this->getId() < other.getId();
}

bool JobsList::JobEntry::operator==(const JobsList::JobEntry& other)
{
    return this->getId() == other.getId();
}

bool JobsList::JobEntry::operator!=(const JobsList::JobEntry& other)
{
    return this->getId() != other.getId();
}


//-------- JobList Class:

JobsList::JobsList() : jobs() {}

JobsList::JobsList(const JobsList& other)
{
    std::vector<std::shared_ptr<JobsList::JobEntry>> new_list;
    for(const auto& current_job: other.jobs){
        shared_ptr<JobEntry> new_job(nullptr);
        try{
            new_job = current_job;
        }
        catch(const std::bad_alloc& e){
            cout << e.what() << endl;
        }
        new_list.push_back(new_job);
    }
    this->jobs = new_list;
}

JobsList& JobsList::operator=(const JobsList& other)
{
    if (this == &other)
    {
        return *this;
    }
    this->jobs.clear();
    for(const auto& current_job: this->jobs){
        shared_ptr<JobEntry> new_job(nullptr);
        try{
            new_job = current_job;
        }
        catch(const std::bad_alloc& e){
            cout << e.what() << endl;
        }
        this->jobs.push_back(new_job);
    }
    return *this;
}

JobsList::~JobsList() {}

void JobsList::addJob(Command *cmd, const pid_t& pid, bool isStopped)
{
    this->removeFinishedJobs();
    int job_id = this->getMaxJobId() + 1;
    //make a JobEntery:
    shared_ptr<JobEntry> new_job(nullptr);
    try{
        new_job = make_shared<JobEntry>(job_id, isStopped, cmd->getInitialCmdLine(), pid, cmd->is_timed, cmd->duration);
    }
    catch(const std::bad_alloc& e){
        cout << e.what() << endl;
    }
    this->jobs.push_back(new_job);
}

bool comapareJobs(const shared_ptr<JobsList::JobEntry> job1, const shared_ptr<JobsList::JobEntry> job2)
{
    return (job1<job2);
}

void JobsList::printJobsList()
{
    std::sort(jobs.begin(), jobs.end(), JobsList::JobIsBigger());
    for(const auto& job: this->jobs){
        string jobToPrint = job->getCompleteCmd();
        time_t current_time;
        time(&current_time);
        time_t elapsed_time = difftime(current_time, job->getTime());

        if (job->getIsTimed())
        {
            jobToPrint = "timeout " + std::to_string(job->getDuration()) + " " + job->getCompleteCmd();
        }

        cout << "[" <<  job->getId() << "] " << jobToPrint << " : " << job->getPid() << " " << elapsed_time << " secs";
        if(job->jobWasStopped()){
            cout << " (stopped)";
        }
        cout << endl;
    }
}

void JobsList::killAllJobs(const bool& kill_jobs)
{
    if (kill_jobs){
        for(auto& job: this->jobs){
            cout << job->getPid() << ": " << job->getCompleteCmd() << endl;
            if (kill(job->getPid(), SIGKILL) == -1){
                //SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR, KILL);
                SMASH_PRINT_ERROR(SMASH_INVALID_ARGS_ERROR, KILL);
                return;
            }
        }
    }
    this->jobs.clear();
}

void JobsList::removeFinishedJobs()
{
    for (auto iter = jobs.begin(); iter != jobs.end();) {
        pid_t result = waitpid((*iter)->getPid(), NULL, WNOHANG);
        if (result > 0 || kill((*iter)->getPid(), 0) == -1){
            iter = jobs.erase(iter);
        }
        else{
            iter++;
        }
    }
}

shared_ptr<JobsList::JobEntry> JobsList::getJobById(const int& jobId) const
{
    if(jobId <= 0){
        return std::shared_ptr<JobsList::JobEntry>(nullptr);
    }
    for(const auto& job: this->jobs){
        if(job->getId() == jobId){
            return job;
        }
    }
    return std::shared_ptr<JobsList::JobEntry>(nullptr);
}

shared_ptr<JobsList::JobEntry> JobsList::getJobByPid(const int& jobPid) const
{
    if(jobPid <= 0){
        return std::shared_ptr<JobsList::JobEntry>(nullptr);
    }
    for(const auto& job: this->jobs){
        if(job->getPid() == jobPid){
            return job;
        }
    }
    return std::shared_ptr<JobsList::JobEntry>(nullptr);
}

std::vector<std::shared_ptr<JobsList::JobEntry>> JobsList::getAllJobs() const
{
    return jobs;
}


void JobsList::removeJobById(const int& jobId)
{
    if(jobId <= 0){
        return;
    }
    for (auto iter = jobs.begin(); iter != jobs.end();) {
        if ((*iter)->getId() == jobId){
            iter = jobs.erase(iter);
        }
        else{
            iter++;
        }
    }
}

shared_ptr<JobsList::JobEntry> JobsList::getLastJob(int *lastJobId) const
{
    std::shared_ptr<JobsList::JobEntry> last_job(nullptr);
    time_t current_time, last_job_time = LONG_MAX;
    time(&current_time);

    for(const auto& job: this->jobs){
        time_t elapsed_time = abs(difftime(current_time, job->getTime()));
        if(elapsed_time < last_job_time){
            last_job_time = elapsed_time;
            *lastJobId = job->getId();
            last_job = job;
        }
    }
    return last_job;
}

//TODO:
/*shared_ptr<JobsList::JobEntry> JobsList::getLastStoppedJob(int *jobId) const
{

}*/

/**
 * getMaxJobId
 * @return A positive job-id if exists. 0, if jobs list is empty.
 */
int JobsList::getMaxJobId() const
{
    int max_id = 0;
    for(const auto& job: this->jobs){
        max_id = max(max_id, job->getId());
    }
    return max_id;
}

/**
 * getMaxStoppedJobId
 * @return A positive job-id if exists. 0, if jobs list does not contain any stopped jobs.
 */
int JobsList::getMaxStoppedJobId() const
{
    int max_id = 0;
    for(const auto& job: this->jobs){
        if(job->jobWasStopped()){
            max_id = max(max_id, job->getId());
        }
    }
    return max_id;
}


int JobsList::getJobsNum() const
{
    return this->jobs.size();
}

//-------- JobIsBigger Class:

bool JobsList::JobIsBigger::operator()(const std::shared_ptr<JobsList::JobEntry>&job1, const std::shared_ptr<JobsList::JobEntry>& job2)
{
    return ( (*job1 < *job2) && !(*job2 < *job1) );
}


//-------- Command Class:

Command::Command(const char *cmd_line) : initial_cmd(cmd_line), internal_cmd_line(cmd_line), command_is_bg(false)
{
    /*if (internal_cmd_line[internal_cmd_line.length() - 1] == '&'){
      this->command_is_bg = true;
      internal_cmd_line = _trim(internal_cmd_line.substr(0, internal_cmd_line.length()-1));
    }*/
    if (_isBackgroundComamnd(internal_cmd_line)){
        this->command_is_bg = true;
        _removeBackgroundSign(internal_cmd_line);
    }
}

bool Command::isBgCommand() const
{
    return this->command_is_bg;
}

void Command::setAsBgCommand(const bool& is_bg)
{
    this->command_is_bg = is_bg;
}

std::string Command::getInitialCmdLine() const
{
    return this->initial_cmd;
}


//-------- PipeCommand Class:

PipeCommand::PipeCommand(const char *cmd_line) : Command(cmd_line), stderr_pipe(false)
{
    vector<string> cmd_vec = splitCommandByCharacter(this->internal_cmd_line, PIPE_CHAR);
    _removeBackgroundSign(cmd_vec[0]);
    this->cmd1 = cmd_vec[0];
    if ((cmd_vec[1].length() > 0) &&  cmd_vec[1].substr(0,1).compare(STDERR_PIPE_PREFIX) == 0){
        this->stderr_pipe = true;
        cmd_vec[1] = cmd_vec[1].substr(1);
    }
    this->cmd2 = cmd_vec[1];
}

void PipeCommand::execute()
{
    if(this->cmd1.empty() || this->cmd2.empty()){
        return;
    }
    SmallShell& smash = SmallShell::getInstance();

    // create pipe:
    int my_pipe[2];// my_pipe[0] <- READ.  my_pipe[1] <- WRITE.
    if (pipe(my_pipe) == -1){
        SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR, PIPE);
        return;
    }

    // fork process:
    int new_pid = fork();

    //$$$ syscall error:
    if (new_pid < 0){
        SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR, FORK);
        return;
    }

        //$$$ father's code:
    else if (new_pid > 0){
        if(close(my_pipe[0]) == -1){
            SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR, CLOSE);
            return;
        }
        // stderr or stdout:
        int old_fd = this->stderrToStdin() ? 2 : 1;
        // make a copy of old stdout/stderr fd:
        int old_stdout = dup(old_fd);
        if (old_stdout == -1){
            SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR, DUP);
            return;
        }
        // make file descriptor 1 or 2, refers to the same open file description as the descriptor `my_pipe[1]`:
        if (dup2(my_pipe[1], old_fd) == -1) {
            SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR, DUP);
            close(old_stdout);
            return;
        }
        //since we have done that, close the file description whice refers by `my_pipe[1]`:
        if (close(my_pipe[1]) == -1) {
            SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR, CLOSE);
            close(old_stdout);
            return;
        }
        smash.executeCommand(this->cmd1.c_str());
        // make file descriptor 1(or 2) refers again to the stdout(or stderr):
        if (dup2(old_stdout, old_fd) == -1){
            SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR, DUP);
            close(old_stdout);
            return;
        }
        // close the file description which refers by `old_stdout`:
        if (close(old_stdout) == -1){
            SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR, CLOSE);
            return;
        }
        waitpid(new_pid, NULL, WUNTRACED);
    }

        //$$$  son's code:
    else{
        if(close(my_pipe[1]) == -1){
            SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR, CLOSE);
            exit(0);
        }

        // make a copy of old stdin fd:
        int old_stdin = dup(0);
        if (old_stdin == -1){
            SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR, DUP);
            exit(0);
        }
        // make file descriptor 0, refers to the same open file description as the descriptor `my_pipe[0]`:
        if (dup2(my_pipe[0], 0) == -1) {
            SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR, DUP);
            close(old_stdin);
            exit(0);
        }

        //since we have done that, close the file description whice refers by `my_pipe[0]`:
        if (close(my_pipe[0]) == -1) {
            SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR, CLOSE);
            close(old_stdin);
            exit(0);
        }

        smash.executeCommand(this->cmd2.c_str());

        // make file descriptor 0 refers again to the stdin:
        if (dup2(old_stdin, 0) == -1){
            SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR, DUP);
            close(old_stdin);
            exit(0);
        }
        // close the file description which refers by `old_stdout`:
        if (close(old_stdin) == -1){
            SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR, CLOSE);
            exit(0);
        }
        exit(0);
    }
}


bool PipeCommand::stderrToStdin() const
{
    return this->stderr_pipe;
}

//-------- RedirectionCommand class:

RedirectionCommand::RedirectionCommand(const char *cmd_line) : Command(cmd_line), append_to_file(false)
{
    vector<string> cmd_vec = splitCommandByCharacter(this->internal_cmd_line, REDIRECTION_CHAR);
    _removeBackgroundSign(cmd_vec[0]);
    this->cmd_part = cmd_vec[0];
    if ((cmd_vec[1].length() > 0) &&  cmd_vec[1].substr(0,1).compare(REDIRECTION_CHAR) == 0){
        this->append_to_file = true;
        cmd_vec[1] = cmd_vec[1].substr(1);
    }
    this->file_part = cmd_vec[1];
}

void RedirectionCommand::execute()
{
    if (this->cmd_part.empty() || this->file_part.empty()){
        return;
    }
    // make a copy of old stdout fd:
    int old_stdout = dup(1);
    if (old_stdout == -1){
        SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR, DUP);
        return;
    }
    // open the file:
    int fd = 0;
    if (this->appendToFile()){
        fd = open(this->file_part.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0777);
    }
    else{
        fd = open(this->file_part.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0777);
    }
    if (fd == -1){
        SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR, OPEN);
        close(old_stdout);
        return;
    }
    // make file descriptor 1 refers to the same open file description as the descriptor `fd`:
    if (dup2(fd,1) == -1) {
        SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR, DUP);
        close(old_stdout);
        return;
    }
    //since we have done that, close the file description whice refers by `fd`:
    if (close(fd) == -1) {
        SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR, CLOSE);
        close(old_stdout);
        return;
    }
    //now we execute the command:
    SmallShell::getInstance().executeCommand(this->cmd_part.c_str());
    // make file descriptor 1 refers again to the stdout:
    if (dup2(old_stdout, 1) == -1){
        SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR, DUP);
        close(old_stdout);
        return;
    }
    // close the file description which refers by `old_stdout`:
    if (close(old_stdout) == -1){
        SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR, CLOSE);
        return;
    }
}

bool RedirectionCommand::appendToFile() const
{
    return this->append_to_file;
}


//-------- BuiltInCommand Class:

BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command(cmd_line)
{
    this->command_is_bg = false;
}


//-------- ChangePrompt Class:

ChangePrompt::ChangePrompt(const char *cmd_line) : BuiltInCommand(cmd_line)
{
    // remove the first word of the line:
    string argument = removeFirstWord(this->internal_cmd_line);
    // get the second word, which is the first one of agrument:
    argument = getFirstWord(argument);
    if (argument.length() > 0)
    {
        prompt_name = argument + "> ";
    }
    else
    {
        prompt_name = SMASH_DEFAULT_PROMPT;
    }
}

void ChangePrompt::execute()
{
    SmallShell &smash = SmallShell::getInstance();
    smash.setPrompt(this->prompt_name);
}


//-------- ShowPidCommand Class:

ShowPidCommand::ShowPidCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ShowPidCommand::execute()
{
    cout << "smash pid is " << getpid() << endl;
}


//-------- GetCurrDirCommand Class:

GetCurrDirCommand::GetCurrDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void GetCurrDirCommand::execute()
{
    char buf[SMASH_MAX_PATH + 1] = "";
    if (getcwd(buf, SMASH_MAX_PATH))
    {
        cout << buf << endl;
    }
    else
    {
        cout << "pwd error" << endl;
    }
}


//-------- ChangeDirCommand Class:

ChangeDirCommand::ChangeDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line), is_valid_command(false), path_name("")
{
    // remove the first word of the line:
    string argument = removeFirstWord(this->internal_cmd_line);
    // check the remaining(Excluding the first two words):
    if (removeFirstWord(argument).length() > 0)
    {
        this->path_name = "cd: too many arguments";
        return;
    }
    // get the second word, which is the first one of agrument:
    argument = getFirstWord(argument);
    if (argument.compare("-") == 0)
    {
        SmallShell &smash = SmallShell::getInstance();
        // in case that cd command has not yet been called:
        if (smash.getOldPwd().length() <= 0)
        {
            this->path_name = "cd: OLDPWD not set";
            return;
        }
        this->path_name = smash.getOldPwd();
    }
    else
    {
        this->path_name = argument;
    }
    this->is_valid_command = true;
}

void ChangeDirCommand::execute()
{
    // save our directory in `buf` before we change it
    char buf[SMASH_MAX_PATH + 1] = "";
    if (!getcwd(buf, SMASH_MAX_PATH))
    {
        cerr << "pwd error" << endl;
    }
    if (this->is_valid_command)
    {
        // call the syscall. if succeded, save the old directory:
        if (chdir(this->path_name.c_str()) != -1)
        {
            SmallShell &smash = SmallShell::getInstance();
            smash.setOldPwd(string(buf));
        }
        else{
            SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR, CHDIR);
            return;
        }
    }
    else
    {
        cerr << "smash error: " << this->path_name << endl;
    }
}


//-------- JobsCommand Class:

JobsCommand::JobsCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line) {}

void JobsCommand::execute()
{
    SmallShell &smash = SmallShell::getInstance();
    smash.jobs.printJobsList();
}


//-------- KillCommand Class:

KillCommand::KillCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobs_list(jobs){}

void KillCommand::execute()
{
    //first, check if the command is valid
    string argument = removeFirstWord(this->internal_cmd_line); // command exluding the first word.
    string sig_num_s = getFirstWord(argument); // second word
    string job_id_s = removeFirstWord(argument); // command exluding first 2 words.
    string remainnig = removeFirstWord(job_id_s); // used to check if there are more then tow arguments.
    // in case of wrong number of arguments:
    if(sig_num_s.length() == 0 || job_id_s.length() == 0 || remainnig.length() > 0){
        SMASH_PRINT_ERROR(SMASH_INVALID_ARGS_ERROR, KILL);
        return;
    }
    //check sig_num ang job_num:
    string prefix = sig_num_s.substr(0,1);
    if(prefix.compare("-") != 0){
        SMASH_PRINT_ERROR(SMASH_INVALID_ARGS_ERROR, KILL);
        return;
    }
    int sig_num, job_id = 0;
    try{
        sig_num = argToInt(sig_num_s.substr(1));
        job_id = argToInt(job_id_s);
    }
    catch(const std::invalid_argument& e){
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }
    //check if a job with this id exists:
    shared_ptr<JobsList::JobEntry> job = jobs_list->getJobById(job_id);
    if(job == nullptr){
        cerr << "smash error: kill: job-id " << job_id << " does not exist" <<endl;
        return;
    }
    //send signal to process:
    if (kill(job->getPid(), sig_num) == -1)
    {
        SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR, KILL);
        return;
    }
    cout << "signal number " << sig_num << " was sent to pid " << job->getPid() << endl;
    jobs_list->removeJobById(job_id);
}


//-------- ForegroundCommand Class:

ForegroundCommand::ForegroundCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobs_list(jobs){}

void ForegroundCommand::execute()
{
    string arguments = removeFirstWord(this->internal_cmd_line);
    string job_id_s = getFirstWord(arguments); // if empty, try getting the max job-id.
    string remainnig = removeFirstWord(arguments); // supposed to be empty.
    int job_id = 0;
    //In case of no arguments:
    if (job_id_s.length() == 0){
        job_id = this->jobs_list->getMaxJobId();
        if (job_id == 0){
            SMASH_PRINT_ERROR(SMASH_JOBS_LIST_EMPTY_ERROR, FG);
            return;
        }
    }
        //In case of two or more arguments:
    else if (remainnig.length() > 0){
        SMASH_PRINT_ERROR(SMASH_INVALID_ARGS_ERROR, FG);
        return;
    }
        //In case of one arguments:
    else{
        try{
            job_id = argToInt(job_id_s);
        }
        catch (std::invalid_argument&){
            SMASH_PRINT_ERROR(SMASH_INVALID_ARGS_ERROR, FG);
            return;
        }
    }
    //check if a job with this id exists:
    shared_ptr<JobsList::JobEntry> job = this->jobs_list->getJobById(job_id);
    if (job == nullptr){
        SMASH_PRINT_ERROR_JOB_ID(SMASH_JOB_ID_DOES_NOT_EXISTS_ERROR_START, SMASH_JOB_ID_DOES_NOT_EXISTS_ERROR_END, FG, job_id);
        return;
    }
    // execute:
    SMASH_PRINT_COMMAND_WITH_PID(job->getCompleteCmd(),job->getPid());
    SmallShell& smash = SmallShell::getInstance();
    smash.current_job = job;
    if (kill(job->getPid(), SIGCONT) == -1){
        SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR, KILL);
    }
    int status;

    smash.current_job = job;
    waitpid(job->getPid(), &status, WUNTRACED);

    if (!WIFSTOPPED(status)) {
        jobs_list->removeJobById(job->getPid());
        smash.current_job = nullptr;
        return;
    }
    job->setJobAsStopped();
}


//-------- BackgroundCommand Class:

BackgroundCommand::BackgroundCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobs_list(jobs){}

void BackgroundCommand::execute()
{
    string arguments = removeFirstWord(this->internal_cmd_line);
    string job_id_s = getFirstWord(arguments); // if empty, try getting the max job-id.
    string remainnig = removeFirstWord(arguments); // supposed to be empty.
    int job_id = 0;
    //In case of no arguments:
    if (job_id_s.length() == 0){
        job_id = this->jobs_list->getMaxStoppedJobId();
        if (job_id == 0){
            SMASH_PRINT_ERROR(SMASH_NO_STOPPED_JOBS_IN_LIST, BG);
            return;
        }
    }
        //In case of two or more arguments:
    else if (remainnig.length() > 0){
        SMASH_PRINT_ERROR(SMASH_INVALID_ARGS_ERROR, BG);
        return;
    }
        //In case of one arguments:
    else{
        try{
            job_id = argToInt(job_id_s);
        }
        catch(std::invalid_argument&){
            SMASH_PRINT_ERROR(SMASH_INVALID_ARGS_ERROR, BG);
            return;
        }
    }
    //check if a job with this id exists:
    shared_ptr<JobsList::JobEntry> job = this->jobs_list->getJobById(job_id);
    if (job == nullptr){
        SMASH_PRINT_ERROR_JOB_ID(SMASH_JOB_ID_DOES_NOT_EXISTS_ERROR_START, SMASH_JOB_ID_DOES_NOT_EXISTS_ERROR_END, BG, job_id);
        return;
    }
    //check if that job was stopped:
    if (!job->jobWasStopped()){
        SMASH_PRINT_ERROR_JOB_ID(SMASH_JOB_JOB_ALREADY_IN_BG_ERROR_START,SMASH_JOB_JOB_ALREADY_IN_BG_ERROR__END, BG, job_id);
        return;
    }
    // execute:
    SMASH_PRINT_COMMAND_WITH_PID(job->getCompleteCmd(),job->getPid());
    if (kill(job->getPid(), SIGCONT) == -1){
        SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR, KILL);
    }
    job->setJobAsResumed();
}


//-------- Quit Class:

QuitCommand::QuitCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobs_list(jobs){}

void QuitCommand::execute()
{
    string argument = getFirstWord(removeFirstWord(this->internal_cmd_line));
    //in case kill wasn't specified, we just exit smash:
    if(argument.compare("kill") != 0){
        exit(0);
    }
    // kill jobs and print to stdin, before exiting:
    cout << "smash: sending SIGKILL signal to " << this->jobs_list->getJobsNum() << " jobs:" << endl;
    this->jobs_list->killAllJobs(true);
    exit(0);
}


//-------- TailCommand Class:

TailCommand::TailCommand(const char *cmd_line) : BuiltInCommand(cmd_line), args_vec()
{
    string args = removeFirstWord(this->internal_cmd_line);
    args_vec.push_back(getFirstWord(args));// 1st arg
    args = removeFirstWord(args);
    args_vec.push_back(getFirstWord(args));// 2nd arg
    args = removeFirstWord(args);
    args_vec.push_back(getFirstWord(args));// remaining
}

void TailCommand::execute()
{
    //in case of no arguments:
    if(this->args_vec[0].empty()){
        SMASH_PRINT_ERROR(SMASH_INVALID_ARGS_ERROR, TAIL);
        return;
    }
    // get args:
    int lines_num = 0;
    string file_name;
    string remaining;
    // if first arg is lines num:
    if(this->args_vec[0].substr(0,1).compare("-") == 0){
        try{
            lines_num = argToInt(this->args_vec[0].substr(1));// send argToInt() the number, without '-'.
            file_name = this->args_vec[1];
            remaining = this->args_vec[2];
        }
        catch(const std::invalid_argument&){
            // lines num was not given.
            SMASH_PRINT_ERROR(SMASH_INVALID_ARGS_ERROR, TAIL);
            return;
        }
    }
        // if first arg is file name:
    else{
        lines_num = 10;
        file_name = this->args_vec[0];
        remaining = this->args_vec[1];
    }
    //in case of too much arguments or a negative lines num:
    if(remaining.length() > 0 || lines_num < 0){
        SMASH_PRINT_ERROR(SMASH_INVALID_ARGS_ERROR, TAIL);
        return;
    }

    //execute:

    // open the file:
    int fd = open(file_name.c_str(), O_RDONLY);
    if (fd == -1){
        SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR, OPEN);
        return;
    }
    // loop for read and print, line by line:
    for (int i = 0; i < lines_num; i++)
    {
        char c;
        int res;
        //internal loop for read a line:
        while (1){
            res = read(fd, &c, 1);
            if (res == -1){
                SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR, READ);
                close(fd);
                return;
            }
            else if (res == 0){
                break;
            }

            // write this character:
            if(write(1, &c, 1) == -1){
                SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR, WRITE);
                close(fd);
                return;
            }
            if(c == '\n'){
                break;
            }

        }
        if(res == 0){
            break;
        }
    }

    if(close(fd) == -1) {
        SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR, CLOSE);
        return;
    }
}


//-------- TailCommand Class:

TouchCommand::TouchCommand(const char *cmd_line) : BuiltInCommand(cmd_line), args_vec()
{
    string args = removeFirstWord(this->internal_cmd_line);
    args_vec.push_back(getFirstWord(args));// 1st arg
    args = removeFirstWord(args);
    args_vec.push_back(getFirstWord(args));// 2nd arg
    args = removeFirstWord(args);
    args_vec.push_back(getFirstWord(args));// remaining
}


void TouchCommand::execute()
{
    // in case of wrong number of arguments:
    if (this->args_vec[0].empty() || this->args_vec[1].empty() || !(this->args_vec[2].empty())){
        SMASH_PRINT_ERROR(SMASH_INVALID_ARGS_ERROR, TOUCH);
        return;
    }

    vector<int> time_vec = getTimeFromString(this->args_vec[1]);
    if(time_vec.size() < 6){ // according to the pdf, should never happen.
        return;
    }
    //build the time struct:
    struct tm info;
    info.tm_sec = time_vec[0];
    info.tm_min = time_vec[1];
    info.tm_hour = time_vec[2];
    info.tm_mday = time_vec[3];
    info.tm_mon = time_vec[4] - MONTHS_TO_SUBSTRACT;
    info.tm_year = time_vec[5] - YEARS_TO_SUBSTRACT;
    info.tm_isdst = -1;

    // get the time, according to the time struct:
    time_t new_time = mktime(&info);
    if (new_time == -1){
        SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR, MKTIME);
        return;
    }

    //set the parameter utime() takes:
    struct utimbuf ubuf;
    ubuf.actime = new_time;
    ubuf.modtime = new_time;

    //call utime:
    if (utime(args_vec[0].c_str(), &ubuf) == -1) {
        SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR, UTIME);
        return;
    }
}


//-------- ExternalCommand Class:

ExternalCommand::ExternalCommand(const char *cmd_line) : Command(cmd_line){}

void ExternalCommand::execute()
{
    SmallShell &smash = SmallShell::getInstance();
    pid_t new_pid = fork();
    if (new_pid < 0){//syscall error:
        SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR, FORK);
    }
    else if (new_pid > 0){ //father's code:
        if(this->getInitialCmdLine().compare("") != 0){
            smash.jobs.addJob(this, new_pid, false);
        }
        if(!isBgCommand())
        {
            smash.current_job = smash.jobs.getJobByPid(new_pid);
            waitpid(new_pid, NULL, WUNTRACED);
            smash.current_job = nullptr;
        }
    }
    else{ // son's code:
        setpgrp();
        char cmd_args[SMASH_MAX_PATH+1];
        strcpy(cmd_args, this->internal_cmd_line.c_str());
        char bash_path[SMASH_BIG_PATH+1];
        strcpy(bash_path, SMASH_BASH_PATH);
        char c_arg[SMASH_BIG_PATH+1];
        strcpy(c_arg, SMASH_C_ARG);
        char *args[] = {bash_path, c_arg, cmd_args, NULL};
        execv(SMASH_BASH_PATH, args);
        SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR, EXECV);
        exit(0);
    }
}


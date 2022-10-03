#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <iostream>
#include <memory>
#include <vector>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

class Command
{
protected:

    std::string initial_cmd; //The command as the user inserted.
    std::string internal_cmd_line; //The command, exluding the '&', if exists.
    bool command_is_bg;
    void setAsBgCommand(const bool &is_bg);


public:
    Command(const char *cmd_line);
    virtual ~Command() = default; // I made it default for now
    virtual void execute() = 0;
    //For timed Commands
    bool is_timed = false;
    int duration;
    //virtual void prepare();
    //virtual void cleanup();
    bool isBgCommand() const;
    std::string getInitialCmdLine() const;

};

class BuiltInCommand : public Command
{
public:
    BuiltInCommand(const char *cmd_line);
    virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command
{
public:
    ExternalCommand(const char *cmd_line);
    virtual ~ExternalCommand() {}
    void execute() override;
};

class PipeCommand : public Command
{
private:
    bool stderr_pipe;
    std::string cmd1;
    std::string cmd2;
public:
    PipeCommand(const char *cmd_line);
    virtual ~PipeCommand() {}
    void execute() override;
    bool stderrToStdin() const;
};

class RedirectionCommand : public Command
{
private:
    bool append_to_file;
    std::string cmd_part;
    std::string file_part;
public:
    explicit RedirectionCommand(const char *cmd_line);
    virtual ~RedirectionCommand() {}
    void execute() override;
    bool appendToFile() const;
    // void prepare() override;
    // void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand
{
private:
    bool is_valid_command;
    std::string path_name;

public:
    ChangeDirCommand(const char *cmd_line);
    virtual ~ChangeDirCommand() {}
    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand
{
public:
    GetCurrDirCommand(const char *cmd_line);
    virtual ~GetCurrDirCommand() {}
    void execute() override;
};

class ShowPidCommand : public BuiltInCommand
{
public:
    ShowPidCommand(const char *cmd_line);
    virtual ~ShowPidCommand() {}
    void execute() override;
};

class JobsList;

class QuitCommand : public BuiltInCommand
{
private:
    JobsList* jobs_list;
public:
    QuitCommand(const char *cmd_line, JobsList *jobs);
    virtual ~QuitCommand() {}
    void execute() override;
};

class JobsList
{
public:
    class JobEntry
    {
    private:
        //each process has its unique id:
        int job_id;
        //indicates if the process was stopped py the user:
        bool is_stopped;
        //each process has its own cmd line, which will be printed:
        std::string complete_command;
        //each process has a pid:
        pid_t job_pid;
        //each process has its unique
        time_t insert_time;
        bool is_timed;
        int duration;
    public:
        JobEntry(const int& id, const bool& isStopped, const std::string& cmd, const pid_t& pid, const bool& is_timed, const int& duration);
        JobEntry(const JobEntry& other);
        JobEntry& operator=(const JobEntry& other);
        ~JobEntry() {};
        int getId() const;
        bool jobWasStopped() const;
        std::string getCompleteCmd() const;
        pid_t getPid() const;
        time_t getTime() const;
        bool getIsTimed();
        int getDuration();
        void setJobAsStopped();
        void setJobAsResumed();
        bool operator>(const JobEntry& other);
        bool operator<(const JobEntry& other);
        bool operator==(const JobEntry& other);
        bool operator!=(const JobEntry& other);

    };

private:
    std::vector<std::shared_ptr<JobEntry>> jobs;

public:
    JobsList();
    JobsList(const JobsList& other);
    JobsList& operator=(const JobsList& other);
    ~JobsList();
    void addJob(Command *cmd, const pid_t& pid, bool isStopped = false);
    void printJobsList();
    void killAllJobs(const bool& kill_jobs);
    void removeFinishedJobs();
    std::vector<std::shared_ptr<JobEntry>> getAllJobs() const;
    std::shared_ptr<JobEntry> getJobById(const int& jobId) const;
    std::shared_ptr<JobEntry> getJobByPid(const int& jobPid) const;
    void removeJobById(const int& jobId);
    std::shared_ptr<JobEntry> getLastJob(int *lastJobId) const;
    std::shared_ptr<JobEntry> getLastStoppedJob(int *jobId) const;
    int getMaxJobId() const;
    int getMaxStoppedJobId() const;
    int getJobsNum() const;
    class JobIsBigger
    {
    public:
        bool operator()(const std::shared_ptr<JobEntry>& job1, const std::shared_ptr<JobEntry>& job2);
    };

};

class JobsCommand : public BuiltInCommand
{
    // TODO: Add your data members
public:
    JobsCommand(const char *cmd_line, JobsList *jobs);
    virtual ~JobsCommand() {}
    void execute() override;
};

class KillCommand : public BuiltInCommand
{
private:
    JobsList* jobs_list;
public:
    KillCommand(const char *cmd_line, JobsList *jobs);
    virtual ~KillCommand() {}
    void execute() override;
};

/**
 * ForegroundCommand: Class for fg command.
 * This conmmand brings a stopped process or a process that runs in the background to the foreground.
 */
class ForegroundCommand : public BuiltInCommand
{
private:
    JobsList* jobs_list;
public:
    ForegroundCommand(const char *cmd_line, JobsList *jobs);
    virtual ~ForegroundCommand() {}
    void execute() override;
};


/**
 * BackgroundCommand: Class for bg command.
 * This  command resumes one of the stopped processes in the background.
 */
class BackgroundCommand : public BuiltInCommand
{
private:
    JobsList* jobs_list;
public:
    BackgroundCommand(const char *cmd_line, JobsList *jobs);
    virtual ~BackgroundCommand() {}
    void execute() override;
};

class TailCommand : public BuiltInCommand
{
private:
    std::vector<std::string> args_vec;
public:
    TailCommand(const char *cmd_line);
    virtual ~TailCommand() {}
    void execute() override;
};

class TouchCommand : public BuiltInCommand
{
private:
    std::vector<std::string> args_vec;
public:
    TouchCommand(const char *cmd_line);
    virtual ~TouchCommand() {}
    void execute() override;
};

class SmallShell
{
private:
    std::string prompt;
    std::string old_pwd;
    SmallShell();


public:
    JobsList jobs;
    std::shared_ptr<JobsList::JobEntry> current_job;
    Command *CreateCommand(const char *cmd_line);
    SmallShell(SmallShell const &) = delete;     // disable copy ctor
    void operator=(SmallShell const &) = delete; // disable = operator
    static SmallShell &getInstance()             // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }
    ~SmallShell();
    void executeCommand(const char *cmd_line);
    std::string getPrompt() const;
    void setPrompt(const std::string &new_prompt);
    std::string getOldPwd() const;
    void setOldPwd(const std::string &prev_pwd);
};

/**
 * ChangePrompt: Class for chprompt command - allow the user to change the prompt displayed by the smash.
 */
class ChangePrompt : public BuiltInCommand
{
private:
    std::string prompt_name;

public:
    ChangePrompt(const char *cmd_line);
    virtual ~ChangePrompt() {}
    void execute() override;
};

#endif // SMASH_COMMAND_H_

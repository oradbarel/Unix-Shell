#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num)
{
    cout << "smash: got ctrl-Z" << endl;
    SmallShell& smash = SmallShell::getInstance();
    if (smash.current_job)    //Checks if there is job running in the foreground
    {
        pid_t pid = smash.current_job->getPid();

        kill(pid, SIGSTOP);
        smash.current_job->setJobAsStopped();
        //shared_ptr<JobsList::JobEntry> check_job = smash.jobs.getJobByPid(pid);
        cout << "smash: process " << pid << " was stopped" << endl;

        smash.current_job = nullptr; ///--- changes of Orad ---///
    }
}

void ctrlCHandler(int sig_num)
{
    cout << "smash: got ctrl-C" << endl;
    SmallShell& smash = SmallShell::getInstance();
    if (smash.current_job)   //Checks if there is job running in the foreground
    {
        int job_id = smash.current_job->getId(); ///--- changes of Orad ---///
        pid_t pid = smash.current_job->getPid();

        kill(pid, SIGKILL);
        cout << "smash: process " << pid << " was killed" << endl;

        smash.current_job = nullptr; ///--- changes of Orad ---///
        smash.jobs.removeJobById(job_id); ///--- changes of Orad ---///
    }
}


void alarmHandler(int sig_num)
{
    cout << "smash: got an alarm" << endl;
    SmallShell& smash = SmallShell::getInstance();

    /*if (smash.current_job->getIsTimed())
    {
        time_t current_time;
        time(&current_time);
        time_t elapsed_time = difftime(current_time, smash.current_job->getTime());

        if (elapsed_time >= smash.current_job->getDuration())
        {
            cout<<"time over"<<endl;
        }

    }*/

    for(const auto& current_job: smash.jobs.getAllJobs())
    {
        if (current_job->getIsTimed())
        {
            time_t current_time;
            time(&current_time);
            time_t elapsed_time = difftime(current_time, current_job->getTime());

            if (elapsed_time >= current_job->getDuration())
            {
                kill(current_job->getPid(), SIGKILL);
                cout<<"smash: timeout " << std::to_string(current_job->getDuration()) << " " << current_job->getCompleteCmd() << " timed out!" <<endl;
                break;
            }

        }

    }
}



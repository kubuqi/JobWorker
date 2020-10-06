//
// Namespace JobServer.
// This namespace contains a set of functions to start, stop and query jobs.
//

#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/wait.h>

#include <fstream>
#include <streambuf>
#include <filesystem>

#include <string>
#include <map>
#include <vector>

#include <atomic>
#include <mutex>
#include <thread>

#include "JobServer.h"

namespace JobServer {
    // Job Table.
    // Only running jobs will be kept here.
    std::mutex g_jobs_mutex;
    std::map<uint64_t, Job> g_jobs;

    //
    // Start a job, return the assigned job ID.
    // 
    uint64_t Start(std::string cmdline) {
        LOG("Starting command line: %s\n", cmdline.c_str());

        // Initialize the job id counter. To make sure the job IDs are unique over time,
        // the higher 32 bit would be the epoch at process start, lower 32 bit for jobs.
        static std::atomic<uint64_t> jid_counter{ uint64_t(std::time(NULL)) << 32 };

        Job job;
        job.jid = jid_counter++;
        job.log = std::string(std::filesystem::current_path()) + "/log/" + std::to_string(job.jid);

        job.pid = fork();
        if (job.pid == -1) {
            perror("Failed to fork()\n");
            exit(1);
        } 
        
        if (job.pid == 0) {
            // Child process
            
            // Terminate when parent process is gone.
            prctl(PR_SET_PDEATHSIG, SIGTERM);

            // In case the log directory is not present
            std::filesystem::create_directory("log");

            // Create log file, redirect stdout and stderr to it.
            int fd = open(job.log.c_str(), O_WRONLY|O_CREAT, 0666);
            dup2(fd, 1);
            dup2(fd, 2);

            execl("/bin/sh", "sh", "-c", cmdline.c_str(), NULL);

            // Probably unreachable from now on
            close(fd); 
            return 0;
        }

        // Parent process.
        // Append this new job into the job table
        LOG("Sarted child process %d for cmd: %s with log file: %s\n",
              job.pid, cmdline.c_str(), job.log.c_str());
    
        std::lock_guard<std::mutex> lock(g_jobs_mutex);
        g_jobs[job.jid] = job;

        return job.jid;
    }

    //
    // Stop a running job
    //
    int Stop(uint64_t jid) {
        LOG("job %llu...\n", jid);

        std::lock_guard<std::mutex> lock(g_jobs_mutex);
        if (g_jobs.count(jid) == 0) {
            ERR("job %llu not running\n", jid);
            return -1;
        }

        // Found job and kill the process.
        // Removing of this job from the job table will be done when the process terminates
        return kill(g_jobs[jid].pid, SIGTERM);
    }

    //
    // Get the status of a job
    //
    JobStatus GetStatus(uint64_t jid) {
        LOG("job %llu...\n", jid);

        std::string log_file = std::string(std::filesystem::current_path()) + "/log/" + std::to_string(jid);
        if (!std::filesystem::exists(log_file)) {
            return NotFound;
        }

        std::lock_guard<std::mutex> lock(g_jobs_mutex);
        if (g_jobs.count(jid) != 0) {
            return Running;
        }

        return Finished;
    }

    //
    // Get the output log of a job
    //
    std::string GetLog(uint64_t jid) {
        LOG("job %llu...\n", jid);

        std::string log_file = std::string(std::filesystem::current_path()) + "/log/" + std::to_string(jid);
        if (!std::filesystem::exists(log_file)) {
            ERR("job %s not found\n", log_file);
            return "";
        }

        // Read the log and return in a string
        std::ifstream fs(log_file);
        return std::string(std::istreambuf_iterator<char>(fs),
                           std::istreambuf_iterator<char>());
    }

    //
    // Helper function to erase a job from job table with the Process ID of the job
    //
    void EraseJob(pid_t pid) {
        // Find the job ID of the terminating process, and erase it from job table.
        std::lock_guard<std::mutex> lock(g_jobs_mutex);
        uint64_t jid = 0;
        for (auto const& x : g_jobs) {
            if (x.second.pid == pid) {
                jid = x.first;
                break;
            }
        }
        assert(jid);        // Should not be 0 or the code is wrong
        g_jobs.erase(jid);
    }

    // Signal handler for SIGCHILD
    // This captures the terminations of child processes, and removes the jobs from the job table 
    void SigChildHandler(int signum)
    {
        // Defer the clean up to a thread to keep signal handler safe.
        // Fire & forget, this anonymous thread will terminate on its own.
        std::thread([](){
            pid_t pid;
            int   status = 0;
            while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
                LOG("Clean up thread: child process pid: %d, status change: %d\n", pid, status);
                JobServer::EraseJob(pid);
            }
        }).detach();
    }
};

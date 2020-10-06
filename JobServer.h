//
// Namespace JobServer.
// This namespace contains a set of functions to start, stop and query jobs.
//

// Simple log utility -- for simple usage only
#define LOG(Format, ...) fprintf(stdout, "%s()[%d]: " Format, __FUNCTION__, __LINE__, __VA_ARGS__)
#define ERR(Format, ...) fprintf(stderr, "%s()[%d]: " Format, __FUNCTION__, __LINE__, __VA_ARGS__)

namespace JobServer {
    enum JobStatus {
        Running,
        Finished,
        NotFound
    };

    struct Job {
        uint64_t    jid;    // Job ID
        pid_t       pid;    // Process ID that runs the job
        std::string log;    // output file path
    };

    //
    // Start a job, return the assigned job ID.
    // 
    uint64_t Start(std::string cmdline);

    //
    // Stop a running job
    //
    int Stop(uint64_t jid);

    //
    // Get the status of a job
    //
    JobStatus GetStatus(uint64_t jid);

    //
    // Get the output log of a job
    //
    std::string GetLog(uint64_t jid);

    //
    // Helper function to erase a job from job table with the Process ID of the job
    //
    void EraseJob(pid_t pid);

    //
    // Signal handler for SIGCHILD
    // This captures the terminations of child processes, and removes the jobs from the job table
    //
    void SigChildHandler(int signum);
}
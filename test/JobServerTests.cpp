#include "../JobServer.h"
// Prototype server for the Job Worker Challenge
//
// How to compile:
//      g++ -std=c++17 -lpthread JobServer.cpp -o server
//
// How to start:
//      ./server
//
// How to stop:
//      type any key to exit.

int main()
{
    printf("server starting..\n");

    // Monitor the termination of child processes by installing SIGCHILD handler
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sa.sa_handler = JobServer::SigChildHandler;
    sigaction(SIGCHLD, &sa, NULL);

    //
    // Testing cases below.
    //
    std::vector<uint64_t> jobIDs;

    // Long lasting jobs
    jobIDs.push_back(JobServer::Start("ping www.tesla.com"));
    jobIDs.push_back(JobServer::Start("ping www.google.ca"));

    // Ephemeral job
    jobIDs.push_back(JobServer::Start("uname -a"));

    // Invalid job
    jobIDs.push_back(JobServer::Start("mom I need help"));    

    getchar();
    
    // Testing GetStatus, GetLog, and Stop
    for (auto jid : jobIDs) {
        printf("Job result for %llu\n Status: %d\n %s\n", 
                jid, JobServer::GetStatus(jid), JobServer::GetLog(jid).c_str());
        JobServer::Stop(jid);
    }

    return 0;
}

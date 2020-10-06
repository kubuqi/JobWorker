//
// client of the Job Worker
//
// To run: ./client ping gravitational.com
// To stop: press return.
//
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include "job_api.grpc.pb.h"
#include "JobServer.h"      // Only for LOG() and ERR()

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using job_api::JobService;

class JobClient {
 public:
  JobClient(std::shared_ptr<Channel> channel)
      : stub_(JobService::NewStub(channel)) {}

  // Wrapper for using the Run service.
  uint64_t Run(const std::string& cmd) {
    job_api::StartRequest request;
    job_api::StartReply reply;
    ClientContext context;

    request.set_command(cmd);
    Status status = stub_->Start(&context, request, &reply);

    if (!status.ok()) {
      ERR("%d %s\n", status.error_code(), status.error_message().c_str());
      return 0;
    }
    return reply.job_id();
  }

  // Wrapper for using the Stop service.
  void Stop(uint64_t job_id) {
    job_api::StopRequest request;
    job_api::StopReply reply;
    ClientContext context;

    request.set_job_id(job_id);
    Status status = stub_->Stop(&context, request, &reply);

    if (!status.ok()) {
      ERR("%d %s\n", status.error_code(), status.error_message().c_str());
    }
  }


 private:
  std::unique_ptr<JobService::Stub> stub_;
};

int main(int argc, char** argv) {

  // TODO: command parsing
  std::string target_str =  "0.0.0.0:50051";

  // Build the command line that need to be run in the server
  std::string cmdline= "";
  for (int i=1; i<argc; i++) {
    cmdline.append(std::string(argv[i]).append(" "));
  }

  // Load certificates
  auto contents = [](const std::string& filename) -> std::string {
      std::ifstream fh(filename);
      std::stringstream buffer;
      buffer << fh.rdbuf();
      fh.close();
      return buffer.str();
  };
  auto ssl_options = grpc::SslCredentialsOptions();
  ssl_options.pem_cert_chain  = contents("../../cert/client-cert.pem");
  ssl_options.pem_private_key = contents("../../cert/client-key.pem");
  ssl_options.pem_root_certs  = contents("../../cert/ca-cert.pem");

  // Create client over secure channel.
  JobClient client(grpc::CreateChannel(target_str, grpc::SslCredentials(ssl_options)));


  auto job_id = client.Run(cmdline);
  
  LOG("Command %llu started\n", job_id);

  getchar();

  client.Stop(job_id);

  LOG("Command %llu stopped\n", job_id);


  return 0;
}

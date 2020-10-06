//
// server of the Job Worker. 
// To run: ./server
//
#include <memory>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include "job_api.grpc.pb.h"

#include "JobServer.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using job_api::JobService;
using job_api::StartRequest;
using job_api::StartReply;
using job_api::StopRequest;
using job_api::StopReply;

//
// RPC service handler
//
class JobServiceImpl final : public JobService::Service {
  // Start a job
  Status Start(ServerContext* context, const StartRequest* request,
                  StartReply* reply) override {
    LOG("%s\n", request->command().c_str());
    auto job_id = JobServer::Start(request->command());
    reply->set_job_id(job_id);
    return Status::OK;
  }

  // Stop a job
  Status Stop(ServerContext* context, const StopRequest* request,
                  StopReply* reply) override {
    LOG("%llu\n", request->job_id());
    JobServer::Stop(request->job_id());
    return Status::OK;
  }
};

int main(int argc, char** argv) {

  // JobServer need to be hooked up with SIGCHILD
  // in order to monitor the child process status.
  struct sigaction sa;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  sa.sa_handler = JobServer::SigChildHandler;
  sigaction(SIGCHLD, &sa, NULL);

  JobServiceImpl service;
  ServerBuilder builder;
  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();

  // A local lambda to load certificates
  auto contents = [](const std::string& filename) -> std::string {
      std::ifstream fh(filename);
      std::stringstream buffer;
      buffer << fh.rdbuf();
      fh.close();
      return buffer.str();
  };

  // TODO: read files from config or option input.
  grpc::SslServerCredentialsOptions::PemKeyCertPair pkcp;
  grpc::SslServerCredentialsOptions ssl_opts;

  pkcp.private_key = contents("../../cert/server-key.pem");
  pkcp.cert_chain = contents("../../cert/server-cert.pem");
  ssl_opts.pem_root_certs=contents("../../cert/ca-cert.pem");
  ssl_opts.pem_key_cert_pairs.push_back(pkcp);
  ssl_opts.client_certificate_request = GRPC_SSL_REQUEST_AND_REQUIRE_CLIENT_CERTIFICATE_AND_VERIFY;

  // Listen on the given address over TLS
  const std::string SERVER_ADDR("0.0.0.0:50051");
  builder.AddListeningPort(SERVER_ADDR, grpc::SslServerCredentials(ssl_opts));
  builder.RegisterService(&service);
  
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  LOG("Server listening on %s\n", SERVER_ADDR.c_str());

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();

  return 0;
}

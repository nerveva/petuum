// Copyright (c) 2014, Sailing Lab
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the <ORGANIZATION> nor the names of its contributors
// may be used to endorse or promote products derived from this software
// without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
#include <boost/shared_array.hpp>
#include <boost/program_options.hpp>
namespace boost_po = boost::program_options;

#include <string>
#include <comm_handler.hpp>
#include <stdio.h>
#include <glog/logging.h>

#define PUB_ID_ST 500

using namespace petuum;

int main(int argc, char *argv[]){
  boost_po::options_description options("Allowed options");
  std::string sip;
  std::string sport;
  int32_t id;
  int32_t sid;
  std::string pubport;

  options.add_options()
    ("id", boost_po::value<int32_t>(&id)->default_value(1), "node id")
    ("sid", boost_po::value<int32_t>(&sid)->default_value(0), "scheduler id")
    ("sip", boost_po::value<std::string>(&sip)->default_value("127.0.0.1"), "ip address")
    ("sport", boost_po::value<std::string>(&sport)->default_value("9999"), "port number")
    ("pubport", boost_po::value<std::string>(&pubport)->default_value("10000"), "publish port number");
  
  boost_po::variables_map options_map;
  boost_po::store(boost_po::parse_command_line(argc, argv, options), options_map);
  boost_po::notify(options_map);  
  google::InitGoogleLogging(argv[0]);
  ConfigParam config(id, false, "", "");

  CommHandler *comm;
  try{
    comm = new CommHandler(config);
  }catch(...){
    LOG(ERROR) << "failed to create comm";
    return -1;
  }
  
  zmq::context_t zmq_ctx(1);

  int suc = comm->Init(&zmq_ctx);
  if(suc == 0) LOG(INFO) << "comm_handler init succeeded";
  else{
    LOG(ERROR) << "comm_handler init failed";
    return -1;
  }

  suc = comm->ConnectTo(sip, sport, sid);
  if(suc < 0){
    LOG(ERROR) << "failed to connect to server";
  }else{
    LOG(INFO) << "successfully connected";
  }

  std::vector<int32_t> pubids(1);
  pubids[0] = PUB_ID_ST;
  suc = comm->SubscribeTo(sip, pubport, pubids);
  if(suc < 0){
    LOG(ERROR) << "failed to subscribe";
  }
  else{
    LOG(ERROR) << "successfully subscribed";
  }

  boost::shared_array<uint8_t> data;
  int32_t rid;

  suc = comm->Recv(rid, data);
  assert(suc > 0 && rid == sid);

  printf("Received msg : %d from %d\n", *((int32_t *) data.get()), sid);

  suc = comm->Send(sid, (uint8_t *) "HEREhello", 10);
  assert(suc > 0);

  suc = comm->Recv(rid, data);
  assert(suc > 0);

  printf("Received msg : %d from %d\n", *((int32_t *) data.get()), sid);
  LOG(INFO) << "MULTICAST MSG RECEIVED!!";

  suc = comm->Send(sid, (uint8_t *) "HEREhello", 10);
  assert(suc > 0);

  LOG(INFO) << "TEST NEARLY PASSED!! SHUTTING DOWN COMM THREAD!!";
  suc = comm->ShutDown();
  if(suc < 0) LOG(ERROR) << "failed to shut down comm handler";
  delete comm;
  LOG(INFO) << "TEST PASSED!! EXITING!!";
  return 0;
}

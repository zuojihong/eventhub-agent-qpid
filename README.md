# eventhub-agent-qpid
C-implementation to send batched messages to event hub asynchronously based on Qpid Proton-C 0.9.1

Overview

This agent can be used in gateways or devices to send batched messages to Azure Event Hub asynchronously. The agent is implemented in C language based on Apache Qpid Proton-C. This implementation is inspired by an example (https://github.com/Azure/azure-content/blob/master/includes/service-bus-event-hubs-get-started-send-c.md). However, the example only supports synchronous messaging. So, this project is intended to go one step further to make it more practical for real production use. 

The agent depends on two external libraries: qpid-proton and jansson. Apache Qpid Proton is a high-performance and lightweight messaging library based on AMQP protocol. Please refer to http://qpid.apache.org/releases/qpid-proton-0.9.1/ for more information. Jansson a C library for working with json data. You can refer to https://jansson.readthedocs.org/en/2.7/ for more information.

The agent includes following files:
- sender-async-batch.c
- tracker-array.c
- tracker-array.h
- gateway.conf

The agent has been tested on Ubuntu 14.04 LTS.


How to install and compile

Before downloading and compiling the source files, you need to install qpid-proton and jansson libraries. In addition, qpid-proton also depends on some third party libraries like openssl. So, you have to install the underlying libraries first. Below are the basic steps to follow:

1. Install underlying dependent libraries:

sudo apt-get install build-essential cmake uuid-dev openssl libssl-dev

2. Browse to http://qpid.apache.org/releases/qpid-proton-0.9.1/ and copy the appropriate mirror site URL. Download the library and extract it, e.g.,

wget http://mirror.bit.edu.cn/apache/qpid/proton/0.9.1/qpid-proton-0.9.1.tar.gz
tar xvfz qpid-proton-0.9.1.tar.gz

3. Build qpid-proton library:

cd qpid-proton-0.9.1-rc1
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr ..
make install

4. Browse to http://www.digip.org/jansson and copy the link URL for Jansson v2.7. Download the library and extract it:

wget http://www.digip.org/jansson/releases/jansson-2.7.tar.gz
tar xvfz jansson-2.7.tar.gz

5. Build Jansson library:

cd jansson-2.7
mkdir build
cd build
cmake ..
make
make install

6. Copy agent files, i.e., sender-async-batch.c, tracker-array.c, tracker-array.h and gateway.conf, to current directory.

7. Compile agent and run it:

gcc sender-async-batch.c tracker-array.c -o sender-async-batch -lqpid-proton -ljansson
./sender-async-batch


How to configure

Before running the agent, you need to configure by editing gateway.conf. To complete the configuration, you are required to provision an Event Hub in Azure and create a shared access policy for it. You can refer to https://azure.microsoft.com/documentation/articles/event-hubs-overview/ for more information on how to use Event Hub.

One thing that deserves particular attention is that the sasKey has to be URL-encoded, which is required by Qpid Proton.
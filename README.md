# DDS Mesos Plugin

An RMS for DDS to utilise mesos for launching dds-agents on demand.

# Compilation Instructions

To get and compile this plugin, please do the following:

```
git clone https://github.com/alisw/mesos-dds
cd mesos-dds
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=<Destination-Install-Directory>    \
         -DPROTOBUF_ROOT=<ProtoBuf-Install-Directory>              \
         -DMESOS_ROOT=<Mesos-Install-Directory>                    \
         -DBOOST_ROOT==<Boost-Install-Directory>                   \
         -DGLOG_ROOT=<GLOG-Install-Directory>			           \
         -DCPPRESTSDK_ROOT=<CPPRESTSDK-Install-Directory>	   	   \
         -DDDS_ROOT=<DDS-Install-Directory>
 make
 make install
```

# Server Usage

The server needs two parameters to run, the IP and Port of the running Mesos server and the IP and Port where to provide the REST Service. For instance, if the Mesos server is running on 192.168.134.131:5050 and the REST service should be run on 192.168.134.131:1234, then the server can be run using:

```
./dds-mesos-server -m 192.168.134.131:5050 -r 192.168.134.131:1234
```

To verify that the REST service is running, one can make an HTTP GET on http://192.168.134.131:1234/status to get a JSON reply representing the status of the service.

# Plugin Usage
## Insert into DDS
In order to let DDS know about the plugin, a folder with a link to the plugin should be made inside DDS' plugin folder. One should do the following:

```
cd <DDS-Install-Directory>/plugins
mkdir dds-submit-mesos
cd dds-submit-mesos
ln -s <Destination-Install-Directory>/bin/dds-submit-mesos .
```

Verify by executing 'dds-submit -l' and checking that 'mesos' is listed.

## Configuration File

A configuration file is required for the plugin to run. The format of each line of the configuration file should be as follows:
1. Mesos Master IP:Port
2. Number of agents to deploy
3. Docker image to use for the agents to run in
4. Folder inside the docker image where to copy the DDS Worker Package
5. Number of CPU Cores to utilise for each Agent
6. The size of memory to use for each agent (in MegaBytes)
7. The IP:Port of the Rest service api (the one given with the -r switch to the server)

An example configuration file (ignoring the bullet points) looks like the following:

- 192.168.134.131:5050
- 2
- ubuntu:14.04
- DDSEnvironment
- 1
- 512
- 192.168.134.131:1234

## Run
Finally, assuming a running DDS server, one can submit DDS agents using this plugin by doing:

```
dds-submit -r mesos -c <path-to-config-file>
```
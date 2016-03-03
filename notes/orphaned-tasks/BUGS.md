Orphaned Tasks
==========

How to reproduce
------------

Start DDS server, then set and activate first topology:

```bash
dds-server start -s
dds-submit -r localhost - n 2
dds-topology --set topology.xml
dds-topology --activate
```

Verify that topology is running and that the sleep processes exist using:

```bash
dds-info -l
ps -aux | grep sleep
```
At this point, one agent is assigned task1 and the other agent is assigned task2. There are also two sleep processes running as expected.

Quickly set and activate second topology, before task1 and task2 finish running (All tasks in this example take a few seconds longer than 2 minutes):

```bash
dds-topology --set topology2.xml
dds-topology --activate
```

Now if we check what happens using:

```bash
dds-info -l
ps -aux | grep sleep
```

We see that, one agent is assigned task3 and the other agent is assigned task4. However, this time there are four sleep processes running. This means that task1 and task2 were orphaned instead of terminated, and using dds, we cannot get back control of task1 and task2.

Eventually, all the sleep tasks will finish running and all gets back to normal.
=====
Terms
=====

.. figure:: figs/workload-terms.png
   :scale: 50 %

   A sample write workload illustrating the terms. This workload
   consists of two rounds, each with a work amount of 500 MB.


We define the following terms.

* **Performance index (PI)** is the property we want to measure. For
  instance, the throughput of a storage device is one PI, and its
  latency is another.

.. note::

   When multiple PIs are involved, Pilot applies the chosen analytical
   methods to each PI independently and combines the results. For
   instance, when calculating the work amount for the next benchmark
   round, Pilot computes the desired work amount for each PI and selects
   the largest.

* **Session** is the context for one measurement run. A session can measure
  multiple PIs and consists of multiple **rounds** of benchmark execution,
  where each round may have a different length.

* **Work amount** is the amount of work performed in one benchmark round.
  It is proportional to the duration of the workload. For instance, in a
  sequential write workload where we write 500 MB of data using 1 MB
  writes, the work amount of that round is 500.

* **Work unit** is the smallest unit of work from which an individual
  measurement can be extracted. In the example above, if the I/O size is
  1 MB, we can measure the time of each I/O syscall and compute the
  throughput of each individual operation. Here the work unit is 1 MB, and
  one round of 500 MB contains 500 work units.

  Not all workloads should be divided into units. Pilot expects work units
  to be reasonably homogeneous: reading 1 MB from different locations on a
  device is approximately homogeneous because performance differences are
  small and roughly normally distributed, but mixing sequential and random
  I/O is not homogeneous because the performance difference is large and
  non-random. In general, divide the workload into units only when you
  *expect* those units to have similar performance. Otherwise, use only
  readings (defined below).

* **Reading** is a measurement of a PI for an entire round. Each benchmark
  round produces one reading per PI at the end of the round. In the example
  above, if the PI is throughput, each round produces one throughput reading.

* **Unit reading (UR)** is a measurement of a PI for a single work unit. In
  the example above, one round of 500 MB produces 500 throughput unit
  readings — one per 1 MB write operation.

* **Work-per-second (WPS)** is the calculated rate at which the workload
  consumes work. This is usually the target PI for simple workloads where
  individual work units cannot be timed separately.

Some workloads cannot be meaningfully divided into homogeneous work units —
for example, booting a system or randomly reading files of different sizes.
These workloads produce only readings, not unit readings.

Installation on Linux
*********************

The original binary packages for Linux are no longer actively maintained.
**Building from source is the recommended installation method.**

See the :doc:`build` page for full instructions. The short version:

.. code-block:: bash

   # Ubuntu / Debian
   sudo apt-get install build-essential cmake libboost-all-dev git

   # Fedora / RHEL / CentOS
   sudo dnf install gcc-c++ cmake boost-devel git

   # Clone and build (headless, no TUI)
   git clone https://github.com/darrelllong/pilot-bench.git
   cd pilot-bench
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DWITH_TUI=OFF
   cmake --build build -j

   # The CLI binary is at:
   build/cli/bench

Historical Binary Packages
--------------------------

The original project provided pre-built RPM packages for RHEL/CentOS 7 and
generic x86-64 Linux tarballs. These are no longer updated and are listed
here for reference only.

Red Hat Enterprise Linux / CentOS 7
=====================================

.. code-block:: bash

   cd /tmp
   wget https://download.ascar.io/pub/repo/RPM-GPG-KEY-ASCAR-NIGHTLY
   rpm --import RPM-GPG-KEY-ASCAR-NIGHTLY
   rpm -ihv https://download.ascar.io/pub/repo/ascar-repo-el-nightly.rpm
   yum install pilot-bench

Generic x86-64 Linux
=====================

* Latest nightly build: https://download.ascar.io/pub/repo/linux-generic-x64/nightly/pilot-bench-nightly-latest-linux-x64.tar.gz

* GPG Signature: https://download.ascar.io/pub/repo/linux-generic-x64/nightly/pilot-bench-nightly-latest-linux-x64.tar.gz.asc

.. include:: signing-keys.rst

Setting up the Python Binding
==============================

If you built with ``-DWITH_PYTHON=ON``, add the path containing
``pilot_bench.so`` to your ``PYTHONPATH``:

.. code-block:: bash

   export PYTHONPATH="$PYTHONPATH:/path/to/pilot-bench/build/lib"

After that, import it in Python with ``import pilot_bench``.

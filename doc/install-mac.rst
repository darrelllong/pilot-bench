Installation on macOS
=====================

The original binary packages for macOS are no longer actively maintained.
**Building from source is the recommended installation method.**

See the :doc:`build` page for full instructions. The short version:

.. code-block:: bash

   # Install dependencies (Homebrew)
   brew install cmake boost

   # Clone and build (headless, no TUI)
   git clone https://github.com/darrelllong/pilot-bench.git
   cd pilot-bench
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DWITH_TUI=OFF
   cmake --build build -j

   # The CLI binary is at:
   build/cli/bench

Historical Binary Packages
--------------------------

The original project provided nightly x86-64 binary packages for macOS
10.11 (El Capitan). These are no longer updated and are not compatible with
Apple Silicon. They are listed here for reference only.

* Latest nightly build: https://download.ascar.io/pub/repo/mac/nightly/10.11-elcapitan/pilot-bench-nightly-latest-Darwin.tar.gz

* GPG Signature: https://download.ascar.io/pub/repo/mac/nightly/10.11-elcapitan/pilot-bench-nightly-latest-Darwin.tar.gz.asc

.. include:: signing-keys.rst

### MSK144 signal generator.

Testing utility. It produces MSK144 frames with desired signal and noise levels. Output - stdout.  
Two output formats: 
1 Audio – 16bit, mono, 12000 samples per second.
2 IQ – 8+8bit, 12000 samples per second.
The number of samples per second is tweakable. IQ stream can be sent to decoder or hardware signal producer like HackRF One device.

**How to compile:**

Prereqirements:

```shell
sudo apt-get install build-essential
sudo apt-get install cmake
sudo apt-get install libboost-dev
sudo apt-get install gfortran

```

The project utilizes original WSJT decoder functions written in Fortran. WSJTX repository linked as git submodule.  
After cloning this repository execute the following commands:
```shell
cd msk144gensim
git submodule init
git submodule update --progress
```

Commands to build:
```shell
mkdir build
cd build
cmake ..
cmake --build . 
```

Install system-wide if necessary:
```shell
sudo cmake --install .
```

Get brief help:
```shell
./msk144gensim --help
```

**Examples:**
How to feed data to HackRF One.
```shell
./msk144gensim --mode=2 --sample-rate=8000000 --on-frames=5 --off-frames=5 --signal-level=20 --noise-level=40 | hackrf_transfer -t - -f 144361500 -s 8000000
```

Feed audio stream directly to msk144 decoder.
```shell
./msk144gensim  | msk144decoder
```

---

*Acknowledgements to K1JT Joe Taylor and WSJT Development Group. The algorithms, source code, and protocol specifications for the mode MSK144, JT65, Q65 are Copyright © 2001-2021 by one or more of the following authors: Joseph Taylor, K1JT; Bill Somerville, G4WJS; Steven Franke, K9AN; Nico Palermo, IV3NWV; Greg Beam, KI7MT; Michael Black, W9MDB; Edson Pereira, PY2SDR; Philip Karn, KA9Q; and other members of the WSJT Development Group.*

---

Alexander, RA9YER.  
ra9yer@yahoo.com

# Omnet++ simulation files

This repository provides the instructions and files required to run the simulations and extract the results. We use [Omnet++ simulator](https://omnetpp.org/) and INET framework to run the simulations. [Omnet++ manual](https://doc.omnetpp.org/omnetpp/manual/) and its examples are good references for introduction to the simulator. We ran our simulations on Ubuntu machines (version: 18.04) so all the commands are for Ubuntu.To run the simulations, you should follow the following steps:

* Step 1: Install dependencies
* Step 2: Install Omnet++
* Step 3: Clone the repository
* Step 4: Build the project
* Step 5: Run the simulations and extract the results

### Step 1: Installing dependencies

To successfully install Omnet++ and run the simulations, execute the following commands:

```
sudo apt update
sudo apt-get install -y build-essential
sudo apt-get install -y flex bison
sudo apt-get install -y zlib1g-dev
sudo apt-get install -y libxml2-dev
sudo apt install -y sqlite3
sudo apt-get install -y libsqlite3-dev
sudo apt-get install -y zip unzip
```

Make sure you have python3 installed (if not, run ```sudo apt-get install -y python3```). If it is installed and accessible in "/usr/bin/python3" run the following: 

```
sudo rm -rf /usr/bin/python
sudo ln -s /usr/bin/python3 /usr/bin/python
sudo apt install -y python3-numpy
sudo apt-get install -y python3-matplotlib
```

### Step 2: Installing Omnet++

The complete instructions for installing Omnet++ can be found in [Omnet++ installation guide](https://doc.omnetpp.org/omnetpp/InstallGuide.pdf). **In case you already have the simulator installed, you can skip this step.** To install Omnet++ on an Ubuntu OS, you should run the following commands:

```
wget https://github.com/omnetpp/omnetpp/releases/download/omnetpp-5.6.2/omnetpp-5.6.2-src-linux.tgz
tar xvfz omnetpp-5.6.2-src-linux.tgz
cd omnetpp-5.6.2/
. setenv
```

For the next line we are assuming that you have extracted omnet in $HOME. If this is not true, replace $HOME with the path you have extracted the omnet files to.

```
echo "export PATH=$HOME/omnetpp-5.6.2/bin:\$PATH" >> ~/.bashrc
```

```
source ~/.bashrc
./configure WITH_QTENV=no WITH_OSG=no WITH_OSGEARTH=no
```

In case, the configure command printed out that "omnetpp-5.6.2/bin" is not added to your path, close and re-open your terminal and run the configure command again. If you are not using GUI, reboot your system using ```sudo reboot```. Make sure you get the **"Your PATH contains /opt/omnetpp-5.6.2/bin. Good!"** message in the configuration process before moving forward.

```
make
```

### Step 3: Cloning the repository

To clone the repository, run the following script:

```
git clone https://github.com/hopnets/practical_deflection.git
```

### Step 4: Building the project

We have provided a ```build.sh``` shell script to simplify this process. To build the project modules, run the following scripts:

```
cd conext-23/Omnet_Sims/
bash build.sh
```

### Step 5: Running the simulations and extracting the results

The config files for large scale simulations can be used for evaluating Simple Deflection, quantile-based Preemptive Deflection, distribution-based Preemptive Deflection, Vertigo, DIBS, ECMP, and AIFO while using TCP, DCTCP, Swift, and Bolt as the transport protocol. Every scenario with these configurations takes a few days to complete. To run the large scale simulations, first make sure that you are in the right directory ("conext-23/Omnet_Sims/dc_simulations/simulations/sims") and then use the following commands to download the distribution files depending on the simulations you want to run (**Make sure to execute only one of these commands as running both of them will overwrite some distribution files and you will face errors while running simulations**):

```
bash download_dist_files_LS.sh # Run this if you want to run leaf-spine simulations.
```
```
bash download_dist_files_FatTree.sh # Run this if you want to run fat-tree simulations.
```

After the distribution files are downloaded, you can use the provided bash scripts to run the large scale simulations for different incast arrival rates (dqps), flow sizes, and scales. Additionally, you can run the simulations for 100 Gbps link rates, fat-tree topology, and component analysis. The list of the provided bash scripts is as below:
* **Different arrival rates with 25%, 50%, and 75% background load and 10/40 Gbps links**
  * run_25_bg_dqps.sh (uses ```omnetpp_25_bg_dqps.ini```)
  * run_50_bg_dqps.sh (uses ```omnetpp_50_bg_dqps.ini```)
  * run_75_bg_dqps.sh (uses ```omnetpp_75_bg_dqps.ini```)
* **Different scales with 50% background load**
  * run_50_bg_dscale.sh (uses ```omnetpp_50_bg_dscale.ini```)
* **Different flow sizes with 50% background load**
  * run_50_bg_dfsize.sh (uses ```omnetpp_50_bg_dfsize.ini```)
* **Different arrival rates with 25%, 50%, and 75% background load and 100 Gbps links**
  * run_25_bg_dqps_100gbps.sh (uses ```omnetpp_25_bg_dqps_100gbps.ini```)
  * run_50_bg_dqps_100gbps.sh (uses ```omnetpp_50_bg_dqps_100gbps.ini```)
  * run_75_bg_dqps_100gbps.sh (uses ```omnetpp_75_bg_dqps_100gbps.ini```)
* **Fat-tree topology under different incast query arrival rates and 50% background load**
  * run_fattree.sh (uses ```omnetpp_fattree.ini```)
* **Preemptive Deflection parameter analysis**
  * run_50_bg_param_study.sh (uses ```omnetpp_50_bg_param_study.ini```)
* **Preemptive Deflection under various queue occupancy update frequencies**
  * run_50_bg_update_freq.sh (uses ```omnetpp_50_bg_dupdate_freq.ini```)
* **Probabilistic Preemptive Deflection**
  * run_50_bg_dqps_probpd.sh (uses ```omnetpp_50_bg_dqps.ini```)


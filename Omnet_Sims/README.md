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
cd practical_deflection/Omnet_Sims/
bash build.sh
```

### Small-scale simulations: Extra step for functionality evaluation

Every scenario with large-scale simulation configurations takes days or even weeks to complete. Accordingly, we are providing a small-scale sample with 1Gbps links for those interested in evaluating the functionality of the code in a short time. First make sure that you are in the right directory ("practical_deflection/Omnet_Sims") and then use the following commands to download the distribution files:

```
cd dc_simulations/simulations/sims
bash download_dist_files_LS_1Gbps.sh
./run_1G.sh 
```

The commands above, download the distribution files for the small-scale simulations and simulate the following scenarios:

* DCTCP + ECMP
* DCTCP + DIBS
* DCTCP + Sample Deflection
* DCTCP + Vertigo
* DCTCP + Quantile_PD
* DCTCP + Dist_PD

We run each technique under 55%, 65%, 75%, 85%, and 95% load. Every scenario is expected to take less than 20 minutes. After the simulations are over, our bash script automatically runs the Python code, prints the results, plots the figures, and saves them in the ./figs directory. The sample output printed should be as follows:

```
dctcp_ecmp:

Mean QCT, tail QCT
0.030712031845580915, 0.07201848251260001

Mean QCT, tail QCT
0.034307867671053476, 0.07510459499572997

Mean QCT, tail QCT
0.041627160594655314, 0.09706178067920997

Mean QCT, tail QCT
0.05073613592821989, 0.10064912956383959

Mean QCT, tail QCT
0.06924289941049247, 0.18201361699152008

----------------------------
dctcp_dibs:

Mean QCT, tail QCT
0.010906813705906512, 0.020969740215100015

Mean QCT, tail QCT
0.02169906125596, 0.04962611242223991

Mean QCT, tail QCT
0.03454644170485348, 0.08130108028845007

Mean QCT, tail QCT
0.05299565263838234, 0.11354337909428008

Mean QCT, tail QCT
0.09183142333665194, 0.31749914493809994

----------------------------
dctcp_sd:

Mean QCT, tail QCT
0.011535378420394319, 0.024274218487599985

Mean QCT, tail QCT
0.021900257798548, 0.04664913429574996

Mean QCT, tail QCT
0.035377280446028975, 0.08639408704508003

Mean QCT, tail QCT
0.053160416861183814, 0.11055537044034001

Mean QCT, tail QCT
0.091430342734979, 0.32988623696522

----------------------------
dctcp_vertigo:

Mean QCT, tail QCT
0.015313937901024383, 0.021617097697300004

Mean QCT, tail QCT
0.017490554553774667, 0.024438772586230006

Mean QCT, tail QCT
0.01825644406141095, 0.02575649940625

Mean QCT, tail QCT
0.01999479711049624, 0.03881809425646

Mean QCT, tail QCT
0.02476214915666788, 0.04861479448799999

----------------------------
dctcp_quantile_pd:

Mean QCT, tail QCT
0.016950521835534693, 0.028026551812679996

Mean QCT, tail QCT
0.023329626538532003, 0.04209370502909996

Mean QCT, tail QCT
0.030315652008150288, 0.05668532107170006

Mean QCT, tail QCT
0.03850137779787182, 0.08061230477756004

Mean QCT, tail QCT
0.05233781733900524, 0.0980787876352001

----------------------------
dctcp_dist_pd:

Mean QCT, tail QCT
0.017507239215281623, 0.026672066423519956

Mean QCT, tail QCT
0.023162280101660004, 0.04119669965275997

Mean QCT, tail QCT
0.030882932240730354, 0.053321890343440044

Mean QCT, tail QCT
0.0405086575778436, 0.08173681864623009

Mean QCT, tail QCT
0.05536194997497886, 0.10524610339752

```

### Step 5: Running the simulations and extracting the results

The config files for large-scale simulations can be used for evaluating Simple Deflection, quantile-based Preemptive Deflection, distribution-based Preemptive Deflection, Vertigo, DIBS, ECMP, and AIFO while using TCP, DCTCP, Swift, and Bolt as the transport protocol. To run the large-scale simulations, first make sure that you are in the right directory ("practical_deflection/Omnet_Sims/dc_simulations/simulations/sims") and then use the following commands to download the distribution files depending on the simulations you want to run (**Make sure to execute only one of these commands as running both of them will overwrite some distribution files and you will face errors while running simulations**):

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


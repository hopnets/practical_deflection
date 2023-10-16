#!/bin/bash


do_extract () {
    python3 ./extractor_shell_creator.py $1
    pushd ./results/
    bash extractor.sh
    popd
    sleep 5
}

rm -rf results

# create the directory to save extracted_results
bash dir_creator.sh


# TCP RUNS
echo "\n\n-------------------------------------------"
echo "Running TCP_ECMP"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c TCP_ECMP -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_50_bg_dqps.ini
do_extract tcp_ecmp
mkdir logs/tcp_ecmp_50_bg_dqps
cp results/*.out logs/tcp_ecmp_50_bg_dqps/

echo "\n\n-------------------------------------------"
echo "Running TCP_DIBS"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c TCP_DIBS -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_50_bg_dqps.ini
do_extract tcp_dibs
mkdir logs/tcp_dibs_50_bg_dqps
cp results/*.out logs/tcp_dibs_50_bg_dqps/

echo "\n\n-------------------------------------------"
echo "Running TCP_SD"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c TCP_SD -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_50_bg_dqps.ini
do_extract tcp_sd
mkdir logs/tcp_sd_50_bg_dqps
cp results/*.out logs/tcp_sd_50_bg_dqps/

echo "\n\n-------------------------------------------"
echo "Running TCP_VERTIGO"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c TCP_VERTIGO -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_50_bg_dqps.ini
do_extract tcp_vertigo
mkdir logs/tcp_vertigo_50_bg_dqps
cp results/*.out logs/tcp_vertigo_50_bg_dqps/

echo "\n\n-------------------------------------------"
echo "Running TCP_DIST_PD"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c TCP_DIST_PD -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_50_bg_dqps.ini
do_extract tcp_dist_pd
mkdir logs/tcp_dist_pd_50_bg_dqps
cp results/*.out logs/tcp_dist_pd_50_bg_dqps/

echo "\n\n-------------------------------------------"
echo "Running TCP_QUANTILE_PD"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c TCP_QUANTILE_PD -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_50_bg_dqps.ini
do_extract tcp_quantile_pd
mkdir logs/tcp_quantile_pd_pd_50_bg_dqps
cp results/*.out logs/tcp_quantile_pd_pd_50_bg_dqps/

# DCTCP RUNS
echo "\n\n-------------------------------------------"
echo "Running DCTCP_ECMP"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c DCTCP_ECMP -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_50_bg_dqps.ini
do_extract dctcp_ecmp
mkdir logs/dctcp_ecmp_50_bg_dqps
cp results/*.out logs/dctcp_ecmp_50_bg_dqps/

echo "\n\n-------------------------------------------"
echo "Running DCTCP_DIBS"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c DCTCP_DIBS -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_50_bg_dqps.ini
do_extract dctcp_dibs
mkdir logs/dctcp_dctcp_dibs_50_bg_dqps
cp results/*.out logs/dctcp_dibs_50_bg_dqps/

echo "\n\n-------------------------------------------"
echo "Running DCTCP_SD"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c DCTCP_SD -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_50_bg_dqps.ini
do_extract dctcp_sd
mkdir logs/dctcp_sd_50_bg_dqps
cp results/*.out logs/dctcp_sd_50_bg_dqps/

echo "\n\n-------------------------------------------"
echo "Running DCTCP_VERTIGO"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c DCTCP_VERTIGO -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_50_bg_dqps.ini
do_extract dctcp_vertigo
mkdir logs/dctcp_vertigo_50_bg_dqps
cp results/*.out logs/dctcp_vertigo_50_bg_dqps/

echo "\n\n-------------------------------------------"
echo "Running DCTCP_DIST_PD"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c DCTCP_DIST_PD -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_50_bg_dqps.ini
do_extract dctcp_dist_pd
mkdir logs/dctcp_dist_pd_50_bg_dqps
cp results/*.out logs/dctcp_dist_pd_50_bg_dqps/

echo "\n\n-------------------------------------------"
echo "Running DCTCP_QUANTILE_PD"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c DCTCP_QUANTILE_PD -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_50_bg_dqps.ini
do_extract dctcp_quantile_pd
mkdir logs/dctcp_quantile_pd_50_bg_dqps
cp results/*.out logs/dctcp_quantile_pd_50_bg_dqps/

echo "\n\n-------------------------------------------"
echo "Running DCTCP_AIFO"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c DCTCP_AIFO -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_50_bg_dqps.ini
do_extract dctcp_aifo
mkdir logs/dctcp_aifo_50_bg_dqps
cp results/*.out logs/dctcp_aifo_50_bg_dqps/

echo "\n\n-------------------------------------------"
echo "Running DCTCP_VERTIGO_LAS"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c DCTCP_VERTIGO_LAS -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_50_bg_dqps.ini
do_extract dctcp_vertigo_las
mkdir logs/dctcp_vertigo_las_50_bg_dqps
cp results/*.out logs/dctcp_vertigo_las_50_bg_dqps/

echo "\n\n-------------------------------------------"
echo "Running DCTCP_DIST_PD_LAS"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c DCTCP_DIST_PD_LAS -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_50_bg_dqps.ini
do_extract dctcp_dist_pd_las
mkdir logs/dctcp_dist_pd_las_50_bg_dqps
cp results/*.out logs/dctcp_dist_pd_las_50_bg_dqps/

echo "\n\n-------------------------------------------"
echo "Running DCTCP_QUANTILE_PD_LAS"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c DCTCP_QUANTILE_PD_LAS -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_50_bg_dqps.ini
do_extract dctcp_quantile_pd_las
mkdir logs/dctcp_quantile_las_pd_50_bg_dqps
cp results/*.out logs/dctcp_quantile_las_pd_50_bg_dqps/

echo "\n\n-------------------------------------------"
echo "Running DCTCP_AIFO_LAS"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c DCTCP_AIFO_LAS -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_50_bg_dqps.ini
do_extract dctcp_aifo_las
mkdir logs/dctcp_aifo_las_50_bg_dqps
cp results/*.out logs/dctcp_aifo_las_50_bg_dqps/


# SWIFT RUNS
echo "\n\n-------------------------------------------"
echo "Running Swift_ECMP"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c Swift_ECMP -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_50_bg_dqps.ini
do_extract swfit_ecmp
mkdir logs/swfit_ecmp_50_bg_dqps
cp results/*.out logs/swfit_ecmp_50_bg_dqps/

echo "\n\n-------------------------------------------"
echo "Running Swift_DIBS"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c Swift_DIBS -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_50_bg_dqps.ini
do_extract swfit_dibs
mkdir logs/swfit_dibs_50_bg_dqps
cp results/*.out logs/swfit_dibs_50_bg_dqps/

echo "\n\n-------------------------------------------"
echo "Running Swift_SD"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c Swift_SD -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_50_bg_dqps.ini
do_extract swfit_sd
mkdir logs/swfit_sd_50_bg_dqps
cp results/*.out logs/swfit_sd_50_bg_dqps/

echo "\n\n-------------------------------------------"
echo "Running Swift_VERTIGO"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c Swift_Vertigo -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_50_bg_dqps.ini
do_extract swfit_vertigo
mkdir logs/swfit_vertigo_50_bg_dqps
cp results/*.out logs/swfit_vertigo_50_bg_dqps/

echo "\n\n-------------------------------------------"
echo "Running Swift_DIST_PD"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c Swift_DIST_PD -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_50_bg_dqps.ini
do_extract swfit_dist_pd
mkdir logs/swfit_dist_pd_50_bg_dqps
cp results/*.out logs/swfit_dist_pd_50_bg_dqps/

echo "\n\n-------------------------------------------"
echo "Running Swift_QUANTILE_PD"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c Swift_QUANTILE_PD -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_50_bg_dqps.ini
do_extract swfit_quantile_pd
mkdir logs/swfit_quantile_pd_50_bg_dqps
cp results/*.out logs/swfit_quantile_pd_50_bg_dqps/


# BOLT RUNS
echo "\n\n-------------------------------------------"
echo "Running Bolt_ECMP"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c Bolt_ECMP -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_50_bg_dqps.ini
do_extract bolt_ecmp
mkdir logs/bolt_ecmp_50_bg_dqps
cp results/*.out logs/bolt_ecmp_50_bg_dqps/

echo "\n\n-------------------------------------------"
echo "Running Bolt_DIBS"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c Bolt_DIBS -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_50_bg_dqps.ini
do_extract bolt_dibs
mkdir logs/bolt_dibs_50_bg_dqps
cp results/*.out logs/bolt_dibs_50_bg_dqps/

echo "\n\n-------------------------------------------"
echo "Running Bolt_SD"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c Bolt_SD -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_50_bg_dqps.ini
do_extract bolt_sd
mkdir logs/bolt_sd_50_bg_dqps
cp results/*.out logs/bolt_sd_50_bg_dqps/

echo "\n\n-------------------------------------------"
echo "Running Bolt_VERTIGO"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c Bolt_Vertigo -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_50_bg_dqps.ini
do_extract bolt_vertigo
mkdir logs/bolt_vertigo_50_bg_dqps
cp results/*.out logs/bolt_vertigo_50_bg_dqps/

echo "\n\n-------------------------------------------"
echo "Running Bolt_DIST_PD"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c Bolt_DIST_PD -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_50_bg_dqps.ini
do_extract bolt_dist_pd
mkdir logs/bolt_dist_pd_50_bg_dqps
cp results/*.out logs/bolt_dist_pd_50_bg_dqps/

echo "\n\n-------------------------------------------"
echo "Running Bolt_QUANTILE_PD"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c Bolt_QUANTILE_PD -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_50_bg_dqps.ini
do_extract bolt_quantile_pd
mkdir logs/bolt_quantile_pd_50_bg_dqps
cp results/*.out logs/bolt_quantile_pd_50_bg_dqps/

# move the extracted results
echo "Moving the extracted results to results_50_bg_dqps"
rm -rf results_50_bg_dqps
mv extracted_results results_50_bg_dqps
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


# move the extracted results
echo "Moving the extracted results to results_50_bg_dqps_tcp"
rm -rf results_50_bg_dqps_tcp
mv extracted_results results_50_bg_dqps_tcp

python3 large_qct_tcp.py
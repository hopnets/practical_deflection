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


# move the extracted results
echo "Moving the extracted results to results_50_bg_dqps_dctcp"
rm -rf results_50_bg_dqps_dctcp
mv extracted_results results_50_bg_dqps_dctcp

python3 large_qct_dctcp.py
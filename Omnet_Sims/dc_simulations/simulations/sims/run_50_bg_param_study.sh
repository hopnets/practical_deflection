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
echo "Running DCTCP_DIST_PD_DALPHA"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c DCTCP_DIST_PD_DALPHA -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_50_bg_param_study.ini
do_extract dctcp_dist_pd_dalpha
mkdir logs/dctcp_dist_pd_50_bg_dalpha
cp results/*.out logs/dctcp_dist_pd_50_bg_dalpha/

echo "\n\n-------------------------------------------"
echo "Running DCTCP_QUANTILE_PD_DALPHA"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c DCTCP_QUANTILE_PD_DALPHA -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_50_bg_param_study.ini
do_extract dctcp_quantile_pd_dalpha
mkdir logs/dctcp_quantile_pd_50_bg_dalpha
cp results/*.out logs/dctcp_quantile_pd_50_bg_dalpha/

echo "\n\n-------------------------------------------"
echo "Running DCTCP_QUANTILE_PD_DWIND"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c DCTCP_QUANTILE_PD_DWIND -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_50_bg_param_study.ini
do_extract dctcp_quantile_pd_dwind
mkdir logs/dctcp_quantile_pd_50_bg_dwind
cp results/*.out logs/dctcp_quantile_pd_50_bg_dwind/

echo "\n\n-------------------------------------------"
echo "Running DCTCP_QUANTILE_PD_DSAMP"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c DCTCP_QUANTILE_PD_DSAMP -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_50_bg_param_study.ini
do_extract dctcp_quantile_pd_dsamp
mkdir logs/dctcp_quantile_pd_50_bg_dsamp
cp results/*.out logs/dctcp_quantile_pd_50_bg_dsamp/


# move the extracted results
echo "Moving the extracted results to results_50_bg_param_study"
rm -rf results_50_bg_dqps
mv extracted_results results_50_bg_param_study
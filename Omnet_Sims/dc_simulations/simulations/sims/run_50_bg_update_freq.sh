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
echo "Running DCTCP_DIST_PD_UPDATE_FREQ"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c DCTCP_DIST_PD_UPDATE_FREQ -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_50_bg_dupdate_freq.ini
do_extract dctcp_dist_pd_dupdate_freq
mkdir logs/dctcp_dist_pd_50_bg_dupdate_freq
cp results/*.out logs/dctcp_dist_pd_50_bg_dupdate_freq/

echo "\n\n-------------------------------------------"
echo "Running DCTCP_QUANTILE_PD_UPDATE_FREQ"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c DCTCP_QUANTILE_PD_UPDATE_FREQ -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_50_bg_dupdate_freq.ini
do_extract dctcp_quantile_pd_dupdate_freq
mkdir logs/dctcp_quantile_pd_50_bg_dupdate_freq
cp results/*.out logs/dctcp_quantile_pd_50_bg_dupdate_freq/



# move the extracted results
echo "Moving the extracted results to results_50_bg_dupdate_freq"
rm -rf results_50_bg_dupdate_freq
mv extracted_results results_50_bg_dupdate_freq
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


echo "\n\n-------------------------------------------"
echo "Running DCTCP_PROB_DIST_PD"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c DCTCP_PROB_DIST_PD -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_50_bg_dqps.ini
do_extract dctcp_prob_dist_pd
mkdir logs/dctcp_prob_dist_pd_50_bg_dqps
cp results/*.out logs/dctcp_prob_dist_pd_50_bg_dqps/

echo "\n\n-------------------------------------------"
echo "Running DCTCP_PROB_QUANTILE_PD"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c DCTCP_PROB_QUANTILE_PD -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_50_bg_dqps.ini
do_extract dctcp_prob_quantile_pd
mkdir logs/dctcp_prob_quantile_pd_50_bg_dqps
cp results/*.out logs/dctcp_prob_quantile_pd_50_bg_dqps/


# move the extracted results
echo "Moving the extracted results to results_50_bg_dqps"
rm -rf results_50_bg_dqps
mv extracted_results results_50_bg_dqps
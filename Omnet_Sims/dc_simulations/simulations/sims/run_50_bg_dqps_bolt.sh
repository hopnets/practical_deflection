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
echo "Moving the extracted results to results_50_bg_dqps_bolt"
rm -rf results_50_bg_dqps_bolt
mv extracted_results results_50_bg_dqps_bolt

python3 large_qct_bolt.py
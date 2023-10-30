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


# move the extracted results
echo "Moving the extracted results to results_50_bg_dqps_swift"
rm -rf results_50_bg_dqps_swift
mv extracted_results results_50_bg_dqps_swift

python3 large_qct_swift.py
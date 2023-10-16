#!/bin/bash


do_extract () {
    python3 ./extractor_shell_creator_fat_tree.py $1
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
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c DCTCP_ECMP_fattree -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_fattree.ini
do_extract dctcp_ecmp_fattree
mkdir logs/dctcp_ecmp_fattree
cp results/*.out logs/dctcp_ecmp_fattree/

echo "\n\n-------------------------------------------"
echo "Running DCTCP_DIBS"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c DCTCP_DIBS_fattree -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_fattree.ini
do_extract dctcp_dibs_fattree
mkdir logs/dctcp_dctcp_dibs_fattree
cp results/*.out logs/dctcp_dctcp_dibs_fattree/

echo "\n\n-------------------------------------------"
echo "Running DCTCP_SD"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c DCTCP_SD_fattree -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_fattree.ini
do_extract dctcp_sd_fattree
mkdir logs/dctcp_dctcp_sd_fattree
cp results/*.out logs/dctcp_dctcp_sd_fattree/

echo "\n\n-------------------------------------------"
echo "Running DCTCP_VERTIGO"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c DCTCP_Vertigo_fattree -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_fattree.ini
do_extract dctcp_vertigo_fattree
mkdir logs/dctcp_vertigo_fattree
cp results/*.out logs/dctcp_vertigo_fattree/

echo "\n\n-------------------------------------------"
echo "Running DCTCP_AIFO_fattree"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c DCTCP_AIFO_fattree -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_fattree.ini
do_extract dctcp_aifo_fattree
mkdir logs/dctcp_aifo_fattree
cp results/*.out logs/dctcp_aifo_fattree/

echo "\n\n-------------------------------------------"
echo "Running DCTCP_QUANTILE_PD_fattree"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c DCTCP_QUANTILE_PD_fattree -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_fattree.ini
do_extract dctcp_quantilepd_fattree
mkdir logs/dctcp_quantilepd_fattree
cp results/*.out logs/dctcp_quantilepd_fattree/

echo "\n\n-------------------------------------------"
echo "Running DCTCP_DIST_PD_fattree"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c DCTCP_DIST_PD_fattree -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_fattree.ini
do_extract dctcp_distpd_fattree
mkdir logs/dctcp_distpd_fattree
cp results/*.out logs/dctcp_distpd_fattree/

# move the extracted results
echo "Moving the extracted results to results_fattree"
rm -rf results_fattree
mv extracted_results results_fattree
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
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c DCTCP_ECMP -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_25_bg_dqps_100gbps.ini
do_extract dctcp_ecmp
mkdir logs/dctcp_ecmp_25_bg_dqps_100gbps
cp results/*.out logs/dctcp_ecmp_25_bg_dqps_100gbps/

echo "\n\n-------------------------------------------"
echo "Running DCTCP_DIBS"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c DCTCP_DIBS -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_25_bg_dqps_100gbps.ini
do_extract dctcp_dibs
mkdir logs/dctcp_dctcp_dibs_25_bg_dqps_100gbps
cp results/*.out logs/dctcp_dibs_25_bg_dqps_100gbps/

echo "\n\n-------------------------------------------"
echo "Running DCTCP_SD"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c DCTCP_SD -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_25_bg_dqps_100gbps.ini
do_extract dctcp_sd
mkdir logs/dctcp_sd_25_bg_dqps_100gbps
cp results/*.out logs/dctcp_sd_25_bg_dqps_100gbps/

echo "\n\n-------------------------------------------"
echo "Running DCTCP_VERTIGO"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c DCTCP_VERTIGO -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_25_bg_dqps_100gbps.ini
do_extract dctcp_vertigo
mkdir logs/dctcp_vertigo_25_bg_dqps_100gbps
cp results/*.out logs/dctcp_vertigo_25_bg_dqps_100gbps/

echo "\n\n-------------------------------------------"
echo "Running DCTCP_DIST_PD"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c DCTCP_DIST_PD -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_25_bg_dqps_100gbps.ini
do_extract dctcp_dist_pd
mkdir logs/dctcp_dist_pd_25_bg_dqps_100gbps
cp results/*.out logs/dctcp_dist_pd_25_bg_dqps_100gbps/

echo "\n\n-------------------------------------------"
echo "Running DCTCP_QUANTILE_PD"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c DCTCP_QUANTILE_PD -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_25_bg_dqps_100gbps.ini
do_extract dctcp_quantile_pd
mkdir logs/dctcp_quantile_pd_25_bg_dqps_100gbps
cp results/*.out logs/dctcp_quantile_pd_25_bg_dqps_100gbps/

echo "\n\n-------------------------------------------"
echo "Running DCTCP_AIFO"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c DCTCP_AIFO -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_25_bg_dqps_100gbps.ini
do_extract dctcp_aifo
mkdir logs/dctcp_aifo_25_bg_dqps_100gbps
cp results/*.out logs/dctcp_aifo_25_bg_dqps_100gbps/

echo "\n\n-------------------------------------------"
echo "Running DCTCP_VERTIGO_LAS"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c DCTCP_VERTIGO_LAS -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_25_bg_dqps_100gbps.ini
do_extract dctcp_vertigo_las
mkdir logs/dctcp_vertigo_las_25_bg_dqps_100gbps
cp results/*.out logs/dctcp_vertigo_las_25_bg_dqps_100gbps/

echo "\n\n-------------------------------------------"
echo "Running DCTCP_DIST_PD_LAS"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c DCTCP_DIST_PD_LAS -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_25_bg_dqps_100gbps.ini
do_extract dctcp_dist_pd_las
mkdir logs/dctcp_dist_pd_las_25_bg_dqps_100gbps
cp results/*.out logs/dctcp_dist_pd_las_25_bg_dqps_100gbps/

echo "\n\n-------------------------------------------"
echo "Running DCTCP_QUANTILE_PD_LAS"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c DCTCP_QUANTILE_PD_LAS -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_25_bg_dqps_100gbps.ini
do_extract dctcp_quantile_pd_las
mkdir logs/dctcp_quantile_las_pd_25_bg_dqps_100gbps
cp results/*.out logs/dctcp_quantile_las_pd_25_bg_dqps_100gbps/

echo "\n\n-------------------------------------------"
echo "Running DCTCP_AIFO_LAS"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c DCTCP_AIFO_LAS -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_25_bg_dqps_100gbps.ini
do_extract dctcp_aifo_las
mkdir logs/dctcp_aifo_las_25_bg_dqps_100gbps
cp results/*.out logs/dctcp_aifo_las_25_bg_dqps_100gbps/


# SWIFT RUNS
echo "\n\n-------------------------------------------"
echo "Running Swift_ECMP"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c Swift_ECMP -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_25_bg_dqps_100gbps.ini
do_extract swfit_ecmp
mkdir logs/swfit_ecmp_25_bg_dqps_100gbps
cp results/*.out logs/swfit_ecmp_25_bg_dqps_100gbps/

echo "\n\n-------------------------------------------"
echo "Running Swift_DIBS"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c Swift_DIBS -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_25_bg_dqps_100gbps.ini
do_extract swfit_dibs
mkdir logs/swfit_dibs_25_bg_dqps_100gbps
cp results/*.out logs/swfit_dibs_25_bg_dqps_100gbps/

echo "\n\n-------------------------------------------"
echo "Running Swift_SD"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c Swift_SD -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_25_bg_dqps_100gbps.ini
do_extract swfit_sd
mkdir logs/swfit_sd_25_bg_dqps_100gbps
cp results/*.out logs/swfit_sd_25_bg_dqps_100gbps/

echo "\n\n-------------------------------------------"
echo "Running Swift_VERTIGO"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c Swift_Vertigo -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_25_bg_dqps_100gbps.ini
do_extract swfit_vertigo
mkdir logs/swfit_vertigo_25_bg_dqps_100gbps
cp results/*.out logs/swfit_vertigo_25_bg_dqps_100gbps/

echo "\n\n-------------------------------------------"
echo "Running Swift_DIST_PD"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c Swift_DIST_PD -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_25_bg_dqps_100gbps.ini
do_extract swfit_dist_pd
mkdir logs/swfit_dist_pd_25_bg_dqps_100gbps
cp results/*.out logs/swfit_dist_pd_25_bg_dqps_100gbps/

echo "\n\n-------------------------------------------"
echo "Running Swift_QUANTILE_PD"
opp_runall -j50 ../../src/dc_simulations -m -u Cmdenv -c Swift_QUANTILE_PD -n ..:../../src:../../../inet/src:../../../inet/examples:../../../inet/tutorials:../../../inet/showcases --image-path=../../../inet/images -l ../../../inet/src/INET omnetpp_25_bg_dqps_100gbps.ini
do_extract swfit_quantile_pd
mkdir logs/swfit_quantile_pd_25_bg_dqps_100gbps
cp results/*.out logs/swfit_quantile_pd_25_bg_dqps_100gbps/



# move the extracted results
echo "Moving the extracted results to results_25_bg_dqps_100gbps"
rm -rf results_25_bg_dqps_100gbps
mv extracted_results results_25_bg_dqps_100gbps
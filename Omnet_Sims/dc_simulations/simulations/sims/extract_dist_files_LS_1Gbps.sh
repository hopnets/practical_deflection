echo "Extracting dist files!"

mkdir distributions

mv dist_files_1gbps/LS_1Gbps/* distributions/

rm -rf dist_files_1gbps

bunzip2 -vf ./distributions/*.bz2

echo "Done!"
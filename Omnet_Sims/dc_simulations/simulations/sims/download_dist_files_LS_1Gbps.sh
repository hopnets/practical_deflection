echo "Downloading dist files..."

mkdir distributions

git clone https://sepehrabdous96@bitbucket.org/dc_group/dist_files_1gbps.git

echo "Extracting dist files!"

mv dist_files_1gbps/LS_1Gbps/* distributions/

rm -rf dist_files_1gbps

bunzip2 -vf ./distributions/*.bz2

echo "Done!"
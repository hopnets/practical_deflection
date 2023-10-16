echo "Downloading dist files..."

mkdir distributions

git clone https://sepehrabdous96@bitbucket.org/dc_group/dist_files_40_10gbps.git
git clone https://sepehrabdous96@bitbucket.org/dc_group/dist_files_100gbps.git

echo "Extracting dist files!"

mv dist_files_40_10gbps/files/* distributions/
mv dist_files_100gbps/LS_100G_OR_10CORE/* distributions/

rm -rf dist_files_40_10gbps
rm -rf dist_files_100gbps

bunzip2 -vf ./distributions/*.bz2

echo "Done!"
repeat=$1
cd kernel
chmod +x uninstall.sh install.sh
./uninstall.sh && ./install.sh
cd ..
cd load_filter
chmod +x run.sh
DATE_STR=$(date +%Y%m%d%H%M)
OUT_FILE="${DATE_STR}.txt"
./run.sh "$repeat" "$OUT_FILE"
python3 filter.py "$OUT_FILE"
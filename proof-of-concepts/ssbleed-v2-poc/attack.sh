OUT_FILE=$1
OFFSET=$2
cd poc && make clean && make
./single_target "$OUT_FILE" "$OFFSET"
python3 plot.py "$OUT_FILE"
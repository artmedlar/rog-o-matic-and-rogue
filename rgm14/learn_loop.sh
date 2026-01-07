#!/bin/bash
# Continuous learning loop for Rog-O-Matic
# Usage: ./learn_loop.sh [num_games] [moves_per_sec]
#
# Example: ./learn_loop.sh 100 10
#   Run 100 games at 10 moves/sec

NUM_GAMES=${1:-10}
MOVE_RATE=${2:-0}

cd "$(dirname "$0")"

echo "Starting Rog-O-Matic learning loop"
echo "Games to play: $NUM_GAMES"
echo "Move rate: ${MOVE_RATE:-unlimited}"
echo ""

for i in $(seq 1 $NUM_GAMES); do
    echo "=== Game $i of $NUM_GAMES ==="
    
    if [ "$MOVE_RATE" -gt 0 ] 2>/dev/null; then
        ./rogomatic -w -m "$MOVE_RATE"
    else
        ./rogomatic -w
    fi
    
    EXIT_CODE=$?
    echo "Game $i finished with exit code $EXIT_CODE"
    
    # Brief pause between games
    sleep 1
done

echo ""
echo "=== Learning session complete ==="
echo "Check lib/ltm* and lib/GenePool* for learned data"


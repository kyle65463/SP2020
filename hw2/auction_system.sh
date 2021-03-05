#! /bin/sh

# Error handling
function interrupted() {
    rm *.tmp
    killall host
    exit
}
trap "interrupted" SIGINT
trap "interrupted" ERR

# Parse arguments
n_host=${1}
n_player=${2}
if [ ${n_host} -gt 10 ] || [ ${n_host} -lt 1 ]; then
    echo "Error: 1<=m<=10"
    exit
fi
if [ ${n_player} -gt 12 ] || [ ${n_player} -lt 8 ]; then
    echo "Error: 8<=n<=12"
    exit
fi

# Get all combinations
combination=()
n_comb=1
for ((a = 1; a <= ${n_player}; a++)); do
    for ((b = $((a + 1)); b <= ${n_player}; b++)); do
        for ((c = $((b + 1)); c <= ${n_player}; c++)); do
            for ((d = $((c + 1)); d <= ${n_player}; d++)); do
                for ((e = $((d + 1)); e <= ${n_player}; e++)); do
                    for ((f = $((e + 1)); f <= ${n_player}; f++)); do
                        for ((g = $((f + 1)); g <= ${n_player}; g++)); do
                            for ((h = $((g + 1)); h <= ${n_player}; h++)); do
                                combination[${n_comb}]="${a} ${b} ${c} ${d} ${e} ${f} ${g} ${h}"
                                ((n_comb++))
                            done
                        done
                    done
                done
            done
        done
    done
done
n_comb=${#combination[@]}
[ ${n_host} -gt ${n_comb} ] && n_host=${n_comb}

# Init final scores
final_score=()
for i in $(seq 1 ${n_player}); do
    final_score[${i}]=0
done

# Init fifos
for i in $(seq 0 10); do
    mkfifo fifo_${i}.tmp
done
exec 4<>fifo_1.tmp
exec 5<>fifo_2.tmp
exec 6<>fifo_3.tmp
exec 7<>fifo_4.tmp
exec 8<>fifo_5.tmp
exec 9<>fifo_6.tmp
exec 10<>fifo_7.tmp
exec 11<>fifo_8.tmp
exec 12<>fifo_9.tmp
exec 13<>fifo_10.tmp

# Init every host with a combination from the back of combination list
for i in $(seq 1 ${n_host}); do
    ./host ${i} ${i} 0 &
    echo ${combination[$((${n_comb} - ${i} + 1))]} >fifo_${i}.tmp
done

# Read and write fifos
exec <fifo_0.tmp
for i in $(seq 1 ${n_comb}); do
    # Get key and id
    read key
    id=${key}
    
    # Get and update score
    for j in $(seq 1 8); do
        read player rank
        score=$((8 - ${rank}))
        final_score[${player}]=$((${final_score[${player}]} + ${score}))
    done
    
    # Pass combination to host when i < n_comb - n_host
    [ ${i} -le $((${n_comb} - ${n_host})) ] && echo ${combination[${i}]} >fifo_${id}.tmp
done

# Close all hosts
for i in $(seq 1 ${n_host}); do
    echo "-1 -1 -1 -1 -1 -1 -1 -1" >fifo_${i}.tmp
done

# Print final scores
for i in $(seq 1 ${n_player}); do
    echo ${i} ${final_score[${i}]}
done

# Clean up
wait
rm *.tmp
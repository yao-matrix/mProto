# taskset -c 1 ./dp

# cpupower frequency-set -u 2.4GHZ
# cpupower frequency-set -d 2.4GHZ
# cpupower frequency-set -g performance

source /opt/intel/vtune_amplifier/amplxe-vars.sh
# amplxe-cl -collect uarch-exploration  -- ./dp
amplxe-cl -collect uarch-exploration -knob sampling-interval=1 -- ./dp
# amplxe-cl -collect memory-access -knob sampling-interval=1 -- ./dp

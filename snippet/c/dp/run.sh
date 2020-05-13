# taskset -c 1 ./dp


source /opt/intel/vtune_amplifier/amplxe-vars.sh
amplxe-cl -collect uarch-exploration -knob sampling-interval=1 -- ./dp

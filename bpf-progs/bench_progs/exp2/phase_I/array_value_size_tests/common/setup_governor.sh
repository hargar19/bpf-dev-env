#!/usr/bin/env bash
# Setup script for CPU frequency scaling (run as root)
# Sets all CPUs to performance governor and disables turbo boost

if [ "$(id -u)" -ne 0 ]; then
    echo "Error: This script must be run as root"
    echo "Usage: sudo ./setup_governor.sh"
    exit 1
fi

echo "Setting CPU governor to performance..."
for gov in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do
    if [ -f "$gov" ]; then
        current=$(cat "$gov")
        echo performance > "$gov"
        echo "  $(basename $(dirname $(dirname "$gov"))): $current -> performance"
    fi
done

echo ""
echo "Disabling turbo boost..."
# Intel
if [ -f /sys/devices/system/cpu/intel_pstate/no_turbo ]; then
    echo 1 > /sys/devices/system/cpu/intel_pstate/no_turbo
    echo "  Intel turbo boost disabled"
fi
# AMD
if [ -f /sys/devices/system/cpu/cpufreq/boost ]; then
    echo 0 > /sys/devices/system/cpu/cpufreq/boost
    echo "  AMD boost disabled"
fi

echo ""
echo "Current CPU configuration:"
echo "  Governor: $(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor)"
if [ -f /sys/devices/system/cpu/intel_pstate/no_turbo ]; then
    turbo_status=$(cat /sys/devices/system/cpu/intel_pstate/no_turbo)
    if [ "$turbo_status" = "1" ]; then
        echo "  Turbo: disabled"
    else
        echo "  Turbo: enabled"
    fi
fi

echo ""
echo "CPU frequencies:"
for cpu in /sys/devices/system/cpu/cpu*/cpufreq/scaling_cur_freq; do
    if [ -f "$cpu" ]; then
        freq=$(cat "$cpu")
        freq_mhz=$((freq / 1000))
        echo "  $(basename $(dirname $(dirname "$cpu"))): ${freq_mhz} MHz"
    fi
done

echo ""
echo "Setup complete. Run './run_all.sh' to start benchmarks."

#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 || $# -gt 2 || ( "$1" != before && "$1" != after ) ]]; then
    printf 'Usage: bash scripts/collect_u02.sh {before|after} [evidence-directory]\n' >&2
    exit 2
fi
if [[ ${ROS_DISTRO:-} != jazzy ]]; then
    printf 'Run source /opt/ros/jazzy/setup.bash first.\n' >&2
    exit 2
fi

phase="$1"
repo="$(cd -- "$(dirname -- "$0")/.." && pwd)"
out="${2:-$repo/evidence/d06}"
controller="${UR_TRAJECTORY_CONTROLLER:-scaled_joint_trajectory_controller}"
mkdir -p "$out"

if [[ "$phase" == before ]]; then
    ros2 control list_controllers |
        sed -E $'s/\033\\[[0-9;]*m//g' | tee "$out/controllers.txt"
    if ! awk -v name="$controller" '$1 == name && $NF == "active" { active = 1 } END { exit !active }' "$out/controllers.txt"; then
        printf 'Controller %s must be active before the motion test.\n' "$controller" >&2
        exit 1
    fi
    ros2 action list -t | tee "$out/actions.txt"
    ros2 action info "/$controller/follow_joint_trajectory" | tee "$out/action-info.txt"
    if ! grep -Eq '^Action servers: [1-9][0-9]*[[:space:]]*$' "$out/action-info.txt"; then
        printf 'No trajectory action server found for %s.\n' "$controller" >&2
        exit 1
    fi
    ros2 param get /move_group use_sim_time | tee "$out/move-group-sim-time.txt"
    timeout 10s ros2 topic echo /clock --once | tee "$out/clock.txt"
elif [[ ! -s "$out/joints-before.yaml" ]]; then
    printf 'Capture joints-before.yaml before the motion test.\n' >&2
    exit 1
fi

tmp="$(mktemp "$out/joints-$phase.yaml.XXXXXX")"
trap 'rm -f -- "$tmp"' EXIT
timeout 10s ros2 topic echo /joint_states --once > "$tmp"
test -s "$tmp"
mv -- "$tmp" "$out/joints-$phase.yaml"
printf 'Saved %s\n' "$out/joints-$phase.yaml"

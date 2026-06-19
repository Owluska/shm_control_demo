#!/usr/bin/env python3

from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd


def main() -> None:
    log_path = Path("shm_log.csv")
    if not log_path.exists():
        raise FileNotFoundError(
            "shm_log.csv not found. Run ./build/monitor first."
        )
    
    output_dir = Path("docs/images")
    output_dir.mkdir(parents=True, exist_ok=True)
    df = pd.read_csv(log_path)

    if df.empty:
        raise RuntimeError("shm_log.csv is empty.")

    t0 = df["time_ns"].iloc[0]
    df["time_sec"] = (df["time_ns"] - t0) / 1e9

    plt.figure()
    plt.plot(df["time_sec"], df["target_speed"], label="target speed")
    plt.plot(df["time_sec"], df["state_speed"], label="state speed")
    plt.xlabel("Time, s")
    plt.ylabel("Speed, m/s")
    plt.title("Target speed vs vehicle speed")
    plt.grid(True)
    plt.legend()
    plt.tight_layout()
    plt.savefig(output_dir / "speed_plot.png", dpi=150)

    plt.figure()
    plt.plot(df["time_sec"], df["throttle"], label="throttle")
    plt.plot(df["time_sec"], df["brake"], label="brake")
    plt.plot(df["time_sec"], df["steering"], label="steering")
    plt.xlabel("Time, s")
    plt.ylabel("Command")
    plt.title("Actuator command")
    plt.grid(True)
    plt.legend()
    plt.tight_layout()
    plt.savefig(output_dir / "command_plot.png", dpi=150)

    plt.figure()
    plt.plot(df["time_sec"], df["target_age_ms"], label="target age")
    plt.plot(df["time_sec"], df["state_age_ms"], label="state age")
    plt.plot(df["time_sec"], df["command_age_ms"], label="command age")
    plt.xlabel("Time, s")
    plt.ylabel("Age, ms")
    plt.title("Shared memory data age")
    plt.grid(True)
    plt.legend()
    plt.tight_layout()
    plt.savefig(output_dir / "age_plot.png", dpi=150)

    plt.show()


if __name__ == "__main__":
    main()
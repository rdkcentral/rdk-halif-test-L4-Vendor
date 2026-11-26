# GstValidate scenario files

Each AV test runs one scenario file. The scenario decides what happens on the pipeline: play, pause, seek, wait, and stop.
The C++ test runner reads these files on the device. They are not compiled into the binary.

Which scenario a test uses is set in the AV test configuration:
- `AVModule.L4.<Suite>.scenario` sets the default for every test of the suite.
- `AVModule.L4.<Suite>.<Test>.scenario` overrides the default for one test.
- `AVModule.scenario_dir` sets the folder, relative to the install base. The default is `scenarios`.

## Files

| File | Status | What it does |
|---|---|---|
| `NormalPlayback.scenario` | Placeholder | Plays to the end. |
| `PauseResume.scenario` | Written | Plays, pauses at 5 s for 5 s, plays again at 10 s, stops at 15 s. |
| `TrickModes.scenario` | Placeholder | Plays to the end. The fast-forward and rewind steps are still to be written. |
| `SeekForward.scenario` | Written | Plays, seeks forward to 30 s at 5 s, stops at 10 s. |
| `SeekBackward.scenario` | Written | Plays, seeks back to 2 s at 5 s, stops at 10 s. |
| `AdaptiveBitrate.scenario` | Placeholder | Plays to the end. The stream switching steps are still to be written. |
| `MetadataPropagation.scenario` | Placeholder | Plays to the end. The metadata checks are not implemented. |

Placeholders play the stream to the end, so the result shows only that the stream plays. The operation in the test case is not tested by a placeholder.

## Format

These are GstValidate scenario files. Each line is one action with `key=value` parameters. The actions used here are
`description`, `play`, `pause`, `seek`, `wait`, and `stop`. Seek positions are absolute times in seconds.

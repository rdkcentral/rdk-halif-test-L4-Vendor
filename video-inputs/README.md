# Stream files

Place every stream file named in `test_cases/l4_av_test_cases.yaml` (the `file:` field under `streams:`) in this directory.
The Python test host copies the streams that the selected test cases use to the device.
The bundle (`arm-deps/make-device-bundle.py`) fails if a stream of an enabled test case is missing from this directory.
The stream files are not stored in this repository snapshot.

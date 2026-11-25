#!/usr/bin/env python3
#** *****************************************************************************
# *
# * If not stated otherwise in this file or this component's LICENSE file the
# * following copyright and licenses apply:
# *
# * Copyright 2025 RDK Management
# *
# * Licensed under the Apache License, Version 2.0 (the "License");
# * you may not use this file except in compliance with the License.
# * You may obtain a copy of the License at
# *
# * http://www.apache.org/licenses/LICENSE-2.0
# *
# * Unless required by applicable law or agreed to in writing, software
# * distributed under the License is distributed on an "AS IS" BASIS,
# * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# * See the License for the specific language governing permissions and
# * limitations under the License.
# *
#** ******************************************************************************
"""
AV L4 pipeline test, run by the RAFT framework.

Flow:
  1. Read the AV test configuration (av_test_configurations/vts_l4_av_test_config.yaml) and its stream
     configuration. The device has the same files; the host reads them to know which tests to run and how
     long each one can take.
  2. Deploy the bundle archive to the device.
  3. For each suite listed in the setup file, run its tests in configuration order through the UT Core menu:
     suite <NormalPlayback | PauseResume | TrickModes | Seek | AdaptiveBitrate | MetadataPropagation>,
     test <Test1 .. Test32>.
  4. Collect each result and return the overall verdict.
"""

import os
import re
import sys

import yaml

dir_path = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(dir_path, "../"))

from raft.framework.core.logModule import logModule

from vts_l4_helper import WAIT_MARGIN_S, VTSL4Helper

REPO_ROOT = os.path.normpath(os.path.join(dir_path, "../../.."))
TEST_CONFIG_FILE = os.path.join(REPO_ROOT, "av_test_configurations", "vts_l4_av_test_config.yaml")
SETUP_FILE = "vts_l4_test_setup.yaml"

# UT Core suites registered by src/vts_audio_video.cpp (one per use case of docs/L4_Test_Cases.md).
KNOWN_SUITES = ("NormalPlayback", "PauseResume", "TrickModes", "Seek", "AdaptiveBitrate", "MetadataPropagation")
# Test names registered in each suite by src/vts_audio_video.cpp (Test1 to Test32).
TEST_NAME_RE = re.compile(r"^Test(?:[1-9]|[12][0-9]|3[0-2])$")
# Actions the device binary understands in trick_modes.
KNOWN_ACTIONS = ("play", "pause", "stop", "seek", "trick_mode")
FULL_LENGTH = 2147483647
# Seconds allowed per stream for the DUT to download it (the binary fetches it inside the test).
DOWNLOAD_MARGIN_S = 60


class TestConfigError(Exception):
    """Raised when the AV test configuration is malformed or names an unknown suite, test, or stream."""


def load_test_config(path: str) -> dict:
    """
    Reads the AV test configuration and its include_<n> stream configurations.

    Returns:
        dict with keys:
          tests:   list of {"suite", "name", "delay_ms", "steps", "streams"} in file order
          streams: {stream id: {"duration": int}} from AVStreams
    """
    with open(path, "r", encoding="utf-8") as f:
        data = yaml.safe_load(f) or {}

    module = data.get("AVModule")
    if not isinstance(module, dict):
        raise TestConfigError("AVModule must be a mapping")

    streams = {}
    for key, value in data.items():
        if key.startswith("include_"):
            with open(os.path.join(REPO_ROOT, value), "r", encoding="utf-8") as f:
                included = yaml.safe_load(f) or {}
            streams.update(included.get("AVStreams", {}))
    streams.update(data.get("AVStreams", {}))

    tests = []
    for suite, suite_tests in (module.get("L4") or {}).items():
        if suite not in KNOWN_SUITES:
            raise TestConfigError(f"suite '{suite}' is not one of {', '.join(KNOWN_SUITES)}")
        for name, entry in (suite_tests or {}).items():
            where = f"AVModule.L4.{suite}.{name}"
            if not TEST_NAME_RE.match(name):
                raise TestConfigError(f"{where}: test name must be Test1 to Test32")
            entry = entry or {}
            controls = entry.get("controls") or {}
            steps = controls.get("trick_modes") or []
            for step in steps:
                if step.get("action") not in KNOWN_ACTIONS:
                    raise TestConfigError(f"{where}: unknown action {step.get('action')!r}")
            used = entry.get("streams") or []
            for stream_id in used:
                if stream_id not in streams:
                    raise TestConfigError(f"{where}: stream {stream_id!r} is not in AVStreams")
            tests.append({
                "suite": suite,
                "name": name,
                "delay_ms": int(controls.get("delay_between_play", 0)),
                "steps": steps,
                "streams": used,
            })
    stream_download_path = module.get("stream_download_path")
    if not stream_download_path:
        raise TestConfigError("AVModule.stream_download_path is missing")
    input_streams_path = module.get("input_streams_path")
    if not input_streams_path:
        raise TestConfigError("AVModule.input_streams_path is missing")
    return {"tests": tests, "streams": streams,
            "stream_download_path": stream_download_path, "input_streams_path": input_streams_path}


def estimate_wait(test: dict, streams: dict) -> int:
    """Device timeout for one test: its finite steps, the stream length for each play to end, and a margin."""
    total = WAIT_MARGIN_S
    for step in test["steps"]:
        duration = int(step.get("duration", 0))
        if duration == FULL_LENGTH:
            total += 10
        else:
            total += max(duration, 0)
    for stream_id in test["streams"]:
        # The DUT downloads each stream inside the test, so the wait includes the download time.
        total += int(streams[stream_id]["duration"]) + DOWNLOAD_MARGIN_S
    return total


class VTSL4TestClass(VTSL4Helper):
    """RAFT test class for the AV L4 pipeline suites."""

    def __init__(self, log: logModule = None):
        super().__init__(test_name="L4_AVModule", qc_id="L4_001", setup_file_name=SETUP_FILE, log=log)
        with open(os.path.join(dir_path, SETUP_FILE), "r", encoding="utf-8") as f:
            self.suites = list((yaml.safe_load(f) or {}).get("VTS", {}).get("suites", []))

    def testFunction(self) -> bool:
        """Runs every configured test of the suites in the setup file. Returns True when all of them pass."""
        try:
            config = load_test_config(TEST_CONFIG_FILE)
        except (TestConfigError, OSError, yaml.YAMLError, KeyError) as error:
            self.log.error(f"AV test configuration error: {error}")
            return False

        unknown = [suite for suite in self.suites if suite not in KNOWN_SUITES]
        if unknown:
            self.log.error(f"Setup lists unknown suites: {', '.join(unknown)}")
            return False

        plan = [test for test in config["tests"] if test["suite"] in self.suites]
        if not plan:
            self.log.error(f"No configured test in suites {', '.join(self.suites)}")
            return False

        self.log.testStart(self.test_name, self.qc_id)

        if not self.deploy_artifact():
            self.log.error("Failed to deploy the bundle to the device")
            return False

        self.enter_run_path()
        self.start_menu()

        stream_dir = "/".join([self.run_path, config["input_streams_path"].strip("/")])
        final_result = True
        try:
            for test in plan:
                step = f"Suite: {test['suite']} Test: {test['name']} Streams: {', '.join(test['streams'])}"
                self.log.stepStart(step)
                passed = False
                try:
                    if self.download_streams(test["streams"], config["streams"],
                                             config["stream_download_path"], stream_dir):
                        result = self.run_test(test["suite"], test["name"], estimate_wait(test, config["streams"]))
                        passed = result is True
                finally:
                    self.remove_streams()
                final_result = final_result and passed
                self.log.stepResult(passed, step)
        finally:
            self.stop_menu()

        self.log.testResult(f"{self.test_name} finished")
        return final_result


if __name__ == "__main__":
    summaryLog = logModule("VTSL4Test", level=logModule.INFO)
    test = VTSL4TestClass(summaryLog)
    test.run(False)

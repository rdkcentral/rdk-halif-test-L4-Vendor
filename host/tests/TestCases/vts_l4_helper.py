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
Device helper for the AV L4 test (RAFT).

Deploys the bundle archive and runs the UT Core tests of the device binary through the UT Core menu, the same
way the VTS suites do. Results are read from the console output ("Run Summary:") by the RAFT UT helper.
Only AV L4 behaviour is here.
"""

import os
import sys

import yaml

dir_path = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(dir_path, "../"))

from raft.framework.core.logModule import logModule
from raft.framework.plugins.ut_raft import utHelperClass
from raft.framework.plugins.ut_raft.configRead import ConfigRead
from raft.framework.plugins.ut_raft.utBaseUtils import utBaseUtils
from raft.framework.plugins.ut_raft.utSuiteNavigator import UTSuiteNavigatorClass
from raft.framework.plugins.ut_raft.utUserResponse import utUserResponse

# Top-level key of the setup YAML and of the device configuration section.
MODULE_KEY = "VTS"
# Directory of the bundle on the device: the archive extracts to <target_directory>/l4-rdk-audio-video.
BUNDLE_DIR_NAME = "l4-rdk-audio-video"
# Console used for the L4 run (defined in the rack configuration).
CONSOLE_NAME = "ssh_hal_test_0"
# Question printed by the binary after a stream when userChoice is set. Must match src/vts_audio_video.cpp.
MANUAL_PROMPT = "Is the stream playback good (Y/N)?"
# Seconds added to the expected run length of a test to get the device wait timeout.
WAIT_MARGIN_S = 60


class VTSL4Helper(utHelperClass):
    """Deploys the AV L4 bundle to the device and runs UT Core tests through the UT Core menu."""

    def __init__(self, test_name: str, qc_id: str, setup_file_name: str, log: logModule = None):
        super().__init__(test_name, qc_id, log)
        self.log = log or logModule(self.__class__.__name__, level=logModule.INFO)
        self.test_name = test_name
        self.qc_id = qc_id

        self.test_setup_path = os.path.join(dir_path, setup_file_name)
        self.test_config = ConfigRead(self.test_setup_path, MODULE_KEY)
        self.test_streams = []

        self.prompt = self.cpe.get("prompt")
        self.wait_time = self.cpe.get("wait_time")
        self.target_directory = self.cpe.get("target_directory").rstrip("/")
        self.run_path = "/".join([self.target_directory, BUNDLE_DIR_NAME])

        self.rack_device = "dut"
        self.hal_session = self.dut.getConsoleSession(CONSOLE_NAME)
        self.hal_session.prompt = self.prompt

        self.ut_menu = UTSuiteNavigatorClass(self.test_config, None, self.hal_session)
        self.utilities = utBaseUtils()
        self.testUserResponse = utUserResponse()

    def deploy_artifact(self) -> bool:
        """Copies the bundle archive to the target directory and extracts it there."""
        try:
            for artifact in self.test_config.test.artifacts:
                local_path = os.path.normpath(os.path.join(dir_path, artifact))
                if not os.path.exists(local_path):
                    self.log.error(f"Artifact not found: {local_path}. Build the bundle first (build-arm.sh).")
                    return False
                name = os.path.basename(local_path)
                self.utilities.sftpCopy(self.hal_session, local_path, self.target_directory)
                self.utilities.change_directory(self.hal_session, self.target_directory)
                self.utilities.untar(self.hal_session, name, self.target_directory)
                self.log.info(f"Extracted {name} in {self.target_directory}")
            return True
        except Exception as e:
            self.log.error(f"Failed to deploy the bundle: {e}")
            return False

    def download_streams(self, stream_ids: list, streams_info: dict, base_url: str, stream_dir: str) -> bool:
        """
        Downloads the stream files of one test from base_url to the DUT, as the existing VTS suites do.

        The DUT fetches each file itself (RAFT downloadToDevice). Each file lands in stream_dir under its file name.

        Args:
            stream_ids (list): Stream ids of the test (keys of AVStreams).
            streams_info (dict): Stream id -> {"input": relative path on the server, "duration": seconds}.
            base_url (str): AVModule.stream_download_path.
            stream_dir (str): Absolute directory on the DUT (AVModule.input_streams_path).
        """
        if not base_url:
            self.log.error("AVModule.stream_download_path is not set in the AV test configuration")
            return False
        try:
            inputs = [streams_info[stream_id]["input"] for stream_id in stream_ids]
            urls = ["/".join([base_url.rstrip("/"), name]) for name in inputs]
            dest = stream_dir.rstrip("/") + "/"
            self.test_streams = [dest + os.path.basename(name) for name in inputs]
            self.writeCommands(f"mkdir -p {dest}", self.hal_session)
            self.downloadToDevice(urls, dest, self.rack_device, True)
            self.log.info(f"Downloaded {', '.join(inputs)} to {dest}")
            return True
        except Exception as e:
            self.log.error(f"Failed to download streams: {e}")
            return False

    def remove_streams(self) -> None:
        """Removes the stream files of the last test from the DUT."""
        if not self.test_streams:
            return
        try:
            self.deleteFromDevice(self.test_streams, self.rack_device, False)
        except Exception as e:
            self.log.error(f"Failed to remove streams: {e}")
        self.test_streams = []

    def enter_run_path(self) -> None:
        self.utilities.change_directory(self.hal_session, self.run_path)

    def start_menu(self) -> None:
        self.ut_menu.start()

    def stop_menu(self) -> None:
        self.ut_menu.stop()

    def run_test(self, suite: str, test_name: str, wait_time: int):
        """
        Runs one UT Core test and returns its result as the RAFT UT helper reports it.

        The binary prints the operator question when userChoice is set. The answer is written back on the same
        console. A missing 'Run Summary:' means the binary crashed: the menu is restarted and the test fails.
        """
        output = self.ut_menu.select(suite, test_name, None, wait_time, False)

        if MANUAL_PROMPT in output:
            verdict = bool(self.testUserResponse.getUserYN(MANUAL_PROMPT))
            self.hal_session.write("Y\n" if verdict else "N\n")
            self.log.info(f"Operator verdict for {suite}.{test_name}: {'pass' if verdict else 'fail'}")

        if "Run Summary:" not in output:
            output += self.hal_session.read_until("Run Summary:", timeout=60)
        if "Run Summary:" in output and "Elapsed time =" not in output:
            output += self.hal_session.read_until("Elapsed time =", timeout=60)

        if "Run Summary:" not in output or "Segmentation fault" in output:
            self.log.error(f"{suite}.{test_name}: Run Summary not found, the binary likely crashed. Marking FAILED.")
            self.ut_menu.stop()
            self.ut_menu.start()
            return False

        return self.ut_menu.collect_results(output, True)
